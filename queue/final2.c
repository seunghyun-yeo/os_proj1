/* signal test */
/* sigaction */
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <unistd.h> 

#include "queues.h"

#define time_quantum 10
#define maxproc 10
#define maxcpuburst 1000

void signal_handler(int signo);
void child_handler(int signo);
void tiktok(int, int);
void reduceall();
void mymovequeue(queue*,queue*,int);


pid_t pid;
int tq;
int cpu_time;
int io_time;
int ret;
int key;

int msgq;
struct sigaction old_sa;
struct sigaction new_sa;
struct itimerval new_itimer, old_itimer;

queue* rqueue;
queue* ioqueue;

typedef struct{
	int mtype;
	pid_t pid;
	int io_time;
} msgbuf;

typedef struct{
	pid_t pid;
	int io_time;
	int cpu_time;
	int tq;
} pcb;

msgbuf msg;
pcb* pcbdata;
int main()
{
	memset(&new_sa, 0, sizeof(new_sa));
	new_sa.sa_handler = &signal_handler;
	sigaction(SIGALRM, &new_sa, &old_sa);
	//need to make SIGALRM every 1sec

	rqueue = createqueue();
	ioqueue = createqueue();

	for(int i=0; i<maxproc; i++)
	{
		pid=fork();
		if(pid==0)
		{
			key=0x142735;
			msgq = msgget(key,IPC_CREAT|0666);
			msg.mtype=0;
			msg.pid=getpid();
			msg.io_time=2;
			new_sa.sa_handler = &child_handler;
			sigaction(SIGUSR1,&new_sa,&old_sa);
			while(1);
		}
		else if(pid>0)
		{
			pcbdata = (pcb*)malloc(sizeof(pcb));
			pcbdata->pid=pid;
			pcbdata->io_time=-1;
			pcbdata->cpu_time=0;
			pcbdata->tq=time_quantum;
			enqueue(rqueue,pcbdata);
		}
		else printf("fork error\n");
	}

	tiktok(0,1);
	
	key=0x142735;
	msgq = msgget(key,IPC_CREAT|0666);
	
	while (1)
	{
		while((ret=msgrcv(msgq, &msg, sizeof(msgbuf),0,IPC_NOWAIT))==-1);
		queuefront(rqueue,(void**)&pcbdata);
		if(pcbdata->pid==msg.pid)
		{
			mymovqueue(rqueue,ioqueue,msg.io_time);
		}
		else
		{
			printf("recieved pid is different from rqueue's front pid\n");
			exit(0);
		}
	}
	return 0;
}

void signal_handler(int signo)
{
	pcb *pcbptr = NULL;
	if((emptyqueue(rqueue))&&(emptyqueue(ioqueue)))
	{
		printf("kernel did all jobs\n");
		exit(0);
	}

	if(!emptyqueue(ioqueue))
	{
		reduceall();
		while(1)
		{
			queuefront(ioqueue,(void**)&pcbptr);
			if(pcbptr->io_time==0)
			{
				dequeue(ioqueue,(void**)&pcbptr);
				enqueue(rqueue,(void*)pcbptr);
			}
			else
			{
				break;
			}
		}
	}

	if(!emptyqueue(rqueue))
	{
		queuefront(rqueue,(void**)&pcbptr);
		if(pcbptr->io_time==0)
		{
			kill(pcbptr->pid,SIGKILL);
			free(pcbptr);
			return;
		}
		pcbptr->tq--;
		pcbptr->cpu_time++;
		kill(pcbptr->pid,SIGUSR1);
		if(pcbptr->tq==0)
		{
			pcbptr->tq=time_quantum;
			if(queuecount(rqueue)>1)requeue(rqueue);
		}


	}
}

void searchqueue(queue* targetqueue, queuenode **ppre ,queuenode **ploc, int iotime)
{
	pcb * pcbptr;
	for(*ppre=NULL,*ploc=targetqueue->front;*ploc!=NULL;*ppre=*ploc,*ploc=(*ploc)->next)
	{
		pcbptr=(*ploc)->dataptr;
		if(pcbptr->io_time<iotime)
			break;
	}
}

void insertqueue(queue* targetqueue, queuenode *ppre,pcb* pcbptr)
{
	queuenode *insert=(queuenode*)malloc(sizeof(queuenode));
	insert->dataptr=pcbptr;
	if(ppre=NULL)
	{
		insert->next=targetqueue->front;
		targetqueue->front=insert;
	}
	else
	{
		insert->next=ppre->next;
		ppre->next=insert;
	}
	targetqueue->count++;
}

void mymovqueue(queue* sourceq, queue* destq, int iotime)
{
	queuenode *ppre=NULL;
	queuenode *ploc=NULL;
	pcb* pcbptr;
	dequeue(sourceq,(void**)&pcbptr);
	searchqueue(destq,&ppre,&ploc,iotime);
	insertqueue(destq,ppre,pcbptr);
}
/*
void mymovqueue(queue* sourceq, queue* destq, int iotime)
{
	pcb* pcbptr,pcbptr1,pcbptr2;
	queuenode *traverse *traverse1, *traverse2;
	dequeue(sourceq,(void**)&pcbptr);
	for(traverse = ioqueue->front;traverse!=NULL,traverse=traverse2)
	{
		if(traverse!=NULL)
		{
			traverse2=traverse->next;
		}
		else 
		{//traverse==NULL : nothing in the queue
			enqueue(ioqueue,pcbptr);
			break;
		}

		if(traverse2==NULL) 
		{//traverse2==NULL : traverse is rear
			enqueue(ioqueue,pcbptr);
			break;
		}

		pcbptr1=traverse1->dataptr;
		pcbptr2=traverse2->dataptr;
		if((pcbptr1->io_time<pcbptr->io_time)&&(pcbptr->io_time<=pcbptr2->io_time))
		{

	}
}*/

void child_handler(int signo)
{
	cpu_time--;
	if(cpu_time==0)
	{
		ret = msgsnd(msgq, &msg,sizeof(msg),IPC_NOWAIT);
		return;
	}
}

void reduceall()
{
	pcb* pcbptr;
	queuenode *traverse;
	for(traverse = ioqueue->front;traverse!=NULL;traverse=traverse->next)
	{
		pcbptr=traverse->dataptr;
		pcbptr->io_time--;
	}
}

void tiktok(int a, int b)
{
	new_itimer.it_interval.tv_sec = a;
	new_itimer.it_interval.tv_usec = b;
	new_itimer.it_value.tv_sec = a;
	new_itimer.it_value.tv_usec = b;
	setitimer(ITIMER_REAL, &new_itimer, &old_itimer);
}












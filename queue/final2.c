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

#define time_quantum 2
#define maxproc 4
#define maxcpuburst 4

void signal_handler(int signo);
void child_handler(int signo);
void tiktok(int, int);
void reduceall();
void mymovqueue(queue*,queue*,int,int);

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

			printf("msgq : %d\n",msgq);
			io_time=2;
			cpu_time=maxcpuburst;
			msg.mtype=0;
			msg.pid=getpid();
			msg.io_time=io_time;
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

	tiktok(1,0);

	key=0x142735;
	msgq = msgget(key,IPC_CREAT|0666);
	printf("msgq : %d\n",msgq);

	while (1)
	{
		while((ret=msgrcv(msgq, &msg, sizeof(msgbuf),0,IPC_NOWAIT))==-1);
		//queuerear(rqueue,(void**)&pcbdata);
		//if(pcbdata->pid==msg.pid)
		//{
		//	printf("%d == %d\n",pcbdata->pid, msg.pid);
			mymovqueue(rqueue,ioqueue,msg.pid,msg.io_time);
		//}
		/*else
		{
			printf("recieved pid is different from rqueue's front pid\n");
			printf("%d != %d\n",pcbdata->pid,msg.pid);
			exit(0);
		}*/
	}
	return 0;
}

void signal_handler(int signo)
{
	printf("tik\n");
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
		printf("pid : %d\n remain tq : %d\n",pcbptr->pid,pcbptr->tq);
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

void insertqueue(queue* targetqueue, queuenode *ppre, queuenode *ploc, queuenode* pploc)
{
	//queuenode *insert;
	//insert->dataptr=pcbptr;
	//insert->dataptr = ploc->dataptr;	

	if(ppre=NULL)
	{
		insert->next=targetqueue->front;
		targetqueue->front=insert;
	}
	else
	{
		ploc->next=ppre->next;
		ppre->next= ploc;
		if(pploc == NULL){
			targetqueue->rear = ploc;
		}
	}
	targetqueue->count++;
}

void mymovqueue(queue* sourceq, queue* destq, int pid, int iotime)
{
	printf("mymovqueue called\n");
	queuenode *ppre=NULL;
	queuenode *ploc=NULL;
	queuenode *pploc= NULL;
	pcb* pcbptr;

	for(ppre=NULL,ploc=sourceq->front; ploc!=NULL; ppre=ploc,ploc=ploc->next){
		pcbptr = ploc->dataptr;
		if(pcbptr->pid == pid){
			ppre->next = ploc->next;
			//free(ploc);
			sourceq->count--;
			pcbptr->iotime = iotime;
			break;
		}

	}
	//dequeue가 아니고 traverse를 이용해서 pid가 같은애가 나가고 봉합하도록
	//dequeue(sourceq,(void**)&pcbptr);
	printf("dequeue  called\n");
	searchqueue(destq,&ppre,&pploc,iotime);
	printf("search queue  called\n");
	insertqueue(destq,ppre,ploc,pploc);
	printf("insert queue called\n");
}

void child_handler(int signo)
{
	cpu_time--;

	printf("remain cpu time : %d\n",cpu_time);
	if(cpu_time==0)
	{
		printf("send message\n");
		ret = msgsnd(msgq, &msg,sizeof(msg),NULL);
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












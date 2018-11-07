#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include <ctype.h>
#include "queues.h"

void signal_handler(int signo);
void child_counter(int signo);
void readyout(int signo);
void reready(int signo);
int total =0;
int count = 0;
queue* readyqueue;

int main(){

	pid_t *pid;
	readyqueue = createqueue();
	struct sigaction old_sa;
	struct sigaction new_sa;
	memset(&new_sa, 0, sizeof(new_sa));
	new_sa.sa_handler = &signal_handler;
	sigaction(SIGALRM, &new_sa, &old_sa);

	struct itimerval new_itimer, old_itimer;
	new_itimer.it_interval.tv_sec = 1;
	new_itimer.it_interval.tv_usec = 0;
	new_itimer.it_value.tv_sec = 1;
	new_itimer.it_value.tv_usec = 0;
	setitimer(ITIMER_REAL, &new_itimer, &old_itimer);

	for(int i = 0; i < 2; i++)
	{
		pid = (pid_t*)malloc(sizeof(pid_t));

		*pid = fork();
		if(*pid == 0)
		{
			count=0;
			struct sigaction child_handler;
			child_handler.sa_handler = &child_counter;
			sigaction(SIGUSR1, &child_handler, &old_sa);
			while(1);	
		}
		else if(*pid > 0)
		{
			enqueue(readyqueue, pid);
		}
	}

	struct sigaction sigusr1;
	memset(&sigusr1, 0, sizeof(sigusr1));
	sigusr1.sa_handler = &readyout;
	sigaction(SIGUSR1, &sigusr1, &old_sa);

	struct sigaction sigusr2;
	memset(&sigusr2, 0, sizeof(sigusr2));
	sigusr2.sa_handler = &reready;
	sigaction(SIGUSR2, &sigusr2, &old_sa);


	while(1);
	return 0;
}
void signal_handler(int signo){//time
	pid_t *pid;
	queuefront(readyqueue, (void**)&pid);
	kill(*pid, SIGUSR1);
}
void child_counter(int signo)
{//child proc -> SIGUSR1
	int end_proc = 10;
	int timeq = 2;

	count++;
	total++;
	printf("%d signaled! : %d\n",getpid(),count);
	if((count == 2)&&(total != 4)){
		kill(getppid(),SIGUSR2);
		count=0;
	}

	if(total == 4){
		kill(getppid(), SIGUSR1);
		exit(0);
	}
	else return;
}

void readyout(int signo){

	pid_t *pid;
	dequeue(readyqueue, (void**)&pid);
	if(emptyqueue(readyqueue)) 
	{
		free(pid);
		exit(0);
	}
	else 
	{
		free(pid);
		return;
	}
}

void reready(int signo){
	requeue(readyqueue);
}

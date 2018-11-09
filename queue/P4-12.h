bool movqueue(queue* squeue, queue* dqueue)
{
	void *dataptr;
	dequeue(squeue,&dataptr);
	enqueue(dqueue,dataptr);
	/*if(squeue->count==0)
	{
		return false;
	}
	if(squeue->count==1)
	{
		squeue->count--;
		dqueue->count++;
		if(dqueue->count==0)
		{
			dqueue->front=squeue->front;
			dqueue->rear=dqueue->front;
		}
		else
		{
			dqueue->rear->next=squeue->front;
			dqueue->rear=dqueue->rear->next;
		}
		squeue->front=NULL;
		squeue->rear=NULL;
	}
	else
	{

		squeue->count--;
		dqueue->count++;
		if(dqueue->count==0)
		{
			dqueue->front=squeue->front;
			dqueue->rear=dqueue->front;
		}
		else
		{
			dqueue->rear->next=squeue->front;
			dqueue->rear=dqueue->rear->next;
		}
		squeue->front=squeue->front->next;
		dqueue->rear->next=NULL;
	}*/
}

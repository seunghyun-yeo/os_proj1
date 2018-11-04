bool requeue(queue* pqueue){

	queuenode* newptr;
	
	if(!(newptr = 
		(queuenode*)malloc(sizeof(queuenode))))
		return false;
	
	void* tempdata;
	tempdata = pqueue->front->dataptr;
	
	newptr->dataptr = tempdata;
	newptr->next = NULL;

	if(pqueue->count == 0)
		pqueue->front = newptr;
	else{
		
		pqueue->rear->next = newptr;
		pqueue->front = pqueue->front->next;
		//pqueue->front->next = NULL;
		free(pqueue->front->next);
		pqueue->front->next = NULL;
		pqueue->rear = newptr;
	}	
	pqueue->rear = newptr;
	return true;
}

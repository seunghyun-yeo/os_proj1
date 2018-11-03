#include<stdio.h>
#include"queues.h"

int main()
{
	queue* test_queue = createqueue();

        enqueue(test_queue, "1");
        enqueue(test_queue, "2");

        printf("%s\n",(char*)test_queue->front->dataptr);
	
	requeue(test_queue);

	printf("%s\n",(char*)test_queue->front->dataptr);

return 0;
}

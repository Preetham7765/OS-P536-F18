#include <fqueue.h>
/*
 * Queue Implementation for futures set queue and get queue
 *
 */


/*
 *Description: Create an empty queue
 *Return: pointer to the newly created queue
 */
queue* create_queue() {
   queue* q = (queue*)getmem(sizeof(queue));
   if((int32)q == SYSERR){
	return NULL;
   }

  q->front = NULL;
  q->rear = NULL;

   return q;
}



/*
 *Description: Put the value at the end of the queue
 *Input: pointer to the queue and the value to be added
 *
 */
void push_back(queue *q, pid32 value){

   node* new_node = (node *)getmem(sizeof(node));
  
   if((int32)new_node == SYSERR){

	fprintf(stderr, "Memory allocation failed\n");
	return ;
   }

   new_node->value = value;
   new_node->next = NULL;

  // check if there no nodes in the queue
   if(q->front == NULL &&  q->rear == NULL){
	q->front = q->rear = new_node;
	return ;
   }

   // add the value to the end
   q->rear->next = new_node;
   q->rear = new_node;

}

/*Description: Remove the element from the front end of the queue
 *Input: pointer to the queue
 *Return: value at the front end of the queue
 */

pid32 pop_front(queue *q) { 

  pid32 value;

  if(q->front == NULL){
       return -1;
  }

  // copy the value of the last node
  value = q->front->value;
  node *n = q->front;
  
  q->front = q->front->next;

  if(q->front == NULL)
	q->rear = NULL;

   //delete the last node
   freemem((char*)n, sizeof(node));;
   
  return value;
}
/*
 *Description: get the number of elements in the queue
 *Input: pointer to the queue
 *Return: size of the queue
 */

uint32 size(queue* q) {
    uint32 count =0;
   
    node *cur = q->front;
    while(cur != NULL){
	count++;
    }

    return count;
}

/*
 *Description: release the memory allocated from the queue 
 *Input: pointer to queue
 *Return : status SYSERR/OK
 */

syscall free_queue(queue* q){

    if(q != NULL){

        //free all the exisitng nodes 
	node* cur = q->front;
	while(cur != NULL){
	  if(freemem((char *)q, sizeof(node)) != OK){
		return SYSERR;

	  }
	  cur = cur->next;
	}
	q->front = NULL;
	q->rear = NULL;
	// free the queue
	return freemem((char*)q, sizeof(queue));
    }
   
    return SYSERR;
}

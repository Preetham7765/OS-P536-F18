#ifndef _FQUEUE_H
#define _FQUEUE_H

#include <xinu.h>
//structure to store every node in the queue
typedef struct qnode {

  int32 value;
  struct qnode *next;

}node;

//structure for all the queue
typedef struct queue {
  node* front;
  node* rear;

}queue;

// queue manupilaiton functions
queue* create_queue();
void push_back (queue *q, int32 value);
int32 pop_front(queue *q);
uint32 size(queue* q);
syscall free_queue(queue* q);
#endif

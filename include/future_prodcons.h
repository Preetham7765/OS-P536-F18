#ifndef _FUTURE_PRODCONS_H
#define FUTURE_PRODCONS_H

#include <xinu.h>
#include <future.h>

// functions to set and get future values
uint future_prod(future_t* fut,int n);
uint future_cons(future_t* fut);


#endif

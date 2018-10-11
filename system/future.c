/* future.c - futures */

#include <xinu.h>
#include <stdio.h>
#include <future.h>


/*
 * functions to 
 * 1. create a future.
 * 2. free a future.
 * 3. get the value of the future.
 * 4. set the value of the future.
 *
 */


/*
 *Description: function to create a new future
 *Input: mode of the future
 *Return: pointer to the newly created future
 */

future_t *future_alloc(future_mode_t mode) {

    //1. create memory for the future.
    future_t *new_future = (future_t *)getmem(sizeof(future_t));;
    if((int32)new_future == SYSERR){
        fprintf(stderr, "Allocating memory failed\n");
	return NULL;

    }
    
   //2. load default values into new future.
    new_future->value = 0;
    new_future->state = FUTURE_EMPTY;
    new_future->mode = mode;
    new_future->pid = -1;
    new_future->set_queue = create_queue();
    new_future->get_queue = create_queue();


    return new_future;

}

/*
 *Description: free the allocated future
 *Input: pointer to the future to be free
 *Return: OK or SYSERR
 */

syscall future_free(future_t *f) {

	if(f != NULL) {
	
	    if(free_queue(f->get_queue) != OK){

		return SYSERR;
	    }
	
	    if(free_queue(f->set_queue) != OK){

		return SYSERR;
	   }

	    return freemem((char*)f, sizeof(future_t));
        }

	return SYSERR;
}

/*
 *Description: get the value of the future.
 *Input: future and a pointer to the value which needs to be put 
 *Return: OK or Return
 */


syscall future_get(future_t *f, int *value) {

  future_mode_t mode = f->mode;
  
  switch(mode) {
        //mode is exclusive
	case FUTURE_EXCLUSIVE: {
	   // if the state is empty suspen the calling process
           if(f->state == FUTURE_EMPTY){
		f->state = FUTURE_WAITING;
                f->pid = currpid;
                suspend(currpid); 
		
           }
           // cannot wait on already waiting future
           if(f->state == FUTURE_WAITING){
		fprintf(stderr, "Future already in waiting state\n");
		return SYSERR;
           }
           // if the state is ready get the value and change it to empty
	   if(f->state == FUTURE_READY){
		*value = f->value;
		f->state = FUTURE_EMPTY;
                return OK;
           }
           break;
	}

	// mode is shared
	case FUTURE_SHARED: {
           // if it is in empty or waiting push it to the get queue and suspend the current process
          if(f->state == FUTURE_EMPTY || f->state == FUTURE_WAITING) {
		f->state = FUTURE_WAITING;
                push_back(f->get_queue,(int32)currpid);
		suspend(currpid);
          }
          // if the future value is ready then return that value
          if(f->state == FUTURE_READY){
              *value = f->value;
	      return OK;
         }
	 break;

        }
   
        // mode is queue
        case FUTURE_QUEUE : {
	   // when the state is empty or waiting(no process has set the value) keep pushing it to the get queue
	    if(f->state == FUTURE_EMPTY || f->state == FUTURE_WAITING) {
              f->state = FUTURE_WAITING;
              push_back(f->get_queue, (int32)currpid);
              suspend(currpid);
            }
            // if there is any process in the set queue then pop from the front of the set queue and return its value
            if(f->state == FUTURE_READY){
                int32 value_from_prod;
                if(size(f->set_queue) != 0){
       		    value_from_prod = pop_front(f->set_queue);
                    *value = value_from_prod;
                }
                if(size(f->set_queue) ==0){
			*value = f->value;	
			f->state = FUTURE_WAITING;
		}
               /*
                if(size(f->set_queue) == 0){
                  if(size(f->get_queue) == 0)
                  	f->state = FUTURE_EMPTY;
	          else
                      f->state = FUTURE_WAITING;
                }*/
                return OK;      
           
            }

        }
	default: {

	    return SYSERR;

       }

       return SYSERR;

  }

  return SYSERR;

}
/*
 *Description: set the value to the future
 *Input: pointer to the future and the value to set
 *Return: status OK or SYSERR
 */

syscall future_set(future_t *f, int value) {

    future_mode_t mode = f->mode;

     switch(mode) {
	// mode is exclusive
        case FUTURE_EXCLUSIVE: {
	   // if the future state is empty then set the future value and change the state to ready
           if(f->state == FUTURE_EMPTY){
                f->value = value;
                f->state = FUTURE_READY;
		return OK;
                                
           }
           // if the future state is wating then set get the pid from the future and call resume
           else if(f->state == FUTURE_WAITING){
                pid32 pid;
                f->value = value;
		f->state = FUTURE_READY;
                pid = f->pid;
		resume(pid);
                return OK;
           }
           // if the future is in ready state and trying to set again. 
           else if(f->state == FUTURE_READY){
		fprintf(stderr, "Error: Old value not read\n");
                return SYSERR;
           }
           break;

        }
	// mode is shared
        case FUTURE_SHARED: {
	       // if the future value is empty then save the value and change the state to ready
               if(f->state == FUTURE_EMPTY){

                    f->value = value;
                    f->state = FUTURE_READY;
                    return OK;

               }
              // if the future is in waiting state remove all the processes from the getqueue and resume each of them
               else if(f->state == FUTURE_WAITING) {
                   // remove all pids 
                   pid32 pid;
                   f->value =value;
                   f->state= FUTURE_READY;  
                   while((pid =(pid32)pop_front(f->get_queue)) != -1){
			resume(pid);
                  }
                 

              }
              // error when multiple sets are called
              else if(f->state == FUTURE_READY){
                 fprintf(stderr, "Error: Multiple set noti allowed\n");
                 return SYSERR;

              }
              break;

        }
	// mode is  queue
        case FUTURE_QUEUE : {
		// if the future is empty push the set request to the queue
                if(f->state == FUTURE_EMPTY){
		   push_back(f->set_queue, value);
                   f->state = FUTURE_READY;
                   return OK;
                }
                // if the future is in waiting then set the value and resume the first pid in the queue
                else if(f->state == FUTURE_WAITING) {
                   f->value = value;
                   f->state = FUTURE_READY;
		   pid32 pid = (pid32)pop_front(f->get_queue);
                   resume(pid);
                   return OK;
                
                }
                // keep pushing the set request to the end
		else if(f-> state == FUTURE_READY) {
		  push_back(f->set_queue,value);
		  return OK;
                }
               break;
                

        }
        default: {

            return SYSERR;

       }

       return SYSERR;

  }


  return SYSERR;

}


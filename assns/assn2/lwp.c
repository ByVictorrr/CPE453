#include "lwp.h"
#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>

#define NULL (void*)0



/**********************Shared variables***************************/
thread current;
// if we were to think of this process current state as a thread
context process;
process.id=NO_THREAD;
// TODO COME BACK HERE
/***************************************************************/


//********************Scheduler**********************************//

/* rr_init=NULL - this is to be called before any threads are 
			 admitted to the scheduler. It setups up the 
			 scheduler.

		void rr_init();
 	(rr_shutdown = NULL) - 	this is to be called when the lwp lib is
									done with a scheduler to allow it to clean up.
									void rr_shutdown();
 rr_admit(thread new) - this is to admit a new thread to the 
						  schedulers pool
*/


thread sch_tail, sch_head; // used to keep track of queue
scheduler RoundRobin = {NULL, NULL, rr_admit, rr_remove, rr_next};


void rr_admit(thread new){
	/* new - newly created thread*/
	/* Verifys - the thread has a validated thread_id*/
	if(new->tid > NO_THREAD){
		/* Case - nothing is in schedulers linked list*/
		if(!sch_tail && !sch_head){
			sch_tail=sch_head=new;
			sch_tail->lib_one=NULL;
			sch_tail->lib_two=NULL;
			return;
		}
		//tail
	}else{
		return;
	}
}


void rr_remove(thread victim);
thread rr_next();






//**************************************************************//
//************************LWP************************************//

thread lib_head;
/* lwp_create: 
			This function creates a new thread with function lwpfun
			It returns the Thread ID
*/
thread newThread(lwpfn, void *arg, int size){	
	static size_t thread_count = 0;
	thread new;
	if(!(new = calloc(1, sizeof(thread)) && (new->stack = calloc(1, sizeof(unsigned long))))){
		return NULL;
	}else{
		/*Set the new thread*/
		new->tid=thread_count++;
		new->stacksize = stack_size;
		new->lib_one=NULL;

		/* Stack stuff */
		new->state.rdi = arg;
		new->state.rsp=(new->stack+size); // is the top os stack
		new->state.rsp=new->stack; // Bottom of stack
		new->state.fxsave=FPU_INIT;

		return new;
	}

}
/*Description: creates a new lwp which executes the given 
				function with given arguments. The new proces
				stack will be stacksize
returns: lwp_id of the new thread OR -1 if cant be made
				*/
thread getTail(){
	thread temp=lib_head;
	while(temp->lib_one){		
		temp=temp->lib_one;
	}
	return temp;
}
tid_t lwp_create(lwpfun, void *arg, size_t size){
	thread new = newThread(lwpfun, arg, size), tail;
	/* Case 1 - is to check to see if linked list is empty*/
	if(!lib_head){
		lib_head=new;
		lib_head->lib_one=NULL;
    /* Case 2 - general case if theres at least one in list*/
	}else{
		/* TODO : add passsed context to scheulers pool 
		sched.admit(new);
		*/
		tail=getTail(); // case only happens if tail != head
		tail->lib_one=new;
	}
	return new->tid;
}
/*Description: Starts the LWP system. Saves the org context,
				Picks a LWP and starts it running.
				If no other threads it returns to main process
				*/
void lwp_start(){
	/* Saving contents of Orginal state before LWP*/
	swap_rfiles(&process.state, NULL);
	/* Make a note to future self when removing check 
		to if list empty if so point head to NULL*/
	if(!lib_head){
		/*lwp_exit() - TODO put back orginal contents in register(i.e. swap_files)*/
	}else{
		thread next;
		/*next = Sch.next()*/
		swap_rfiles(&current->state, &next);
	}
} 
/*Description: Stops the lwp sys, resptres the org stack pointer 
				and returns to that context
				*/
	
void lwp_stop(){
	thread temp; 
	/* Get current state then write org process*/
	swap_rfiles(&current->state, &process);
}


/*Description: Terminates the current process and frees 
				its resource(i.e. its stack). Calls sched->next()
				to get next thread, if no other threads,
				restores the orginal system thread
				*/
void lwp_exit(){
	/* if a current exists then*/
	if(current){
		/* There has to be a thread left */
		free(current->stack);
		free(current);
		/* sched.remove(current)*/
		thread next;
		/* next = sched->next()*/
		/* restore the org system thread */
		if(!next){
			swap_rfiles(NULL, &process);
			current=NULL; // no more in linked list
		/* Set next to the current thread*/
		}else{
			/* We dont care what was previous in address*/
			current=next;
			swap_rfiles(NULL, &current);
		}
	}
}
//**************************************************************//


int main(){

	return 0;
}

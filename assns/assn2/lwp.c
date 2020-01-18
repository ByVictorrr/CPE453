#include "lwp.h"
#include <stdio.h>
#include <stdlib.h>


#define THREAD_INFO_SIZE sizeof(struct threadinfo_st)

/**********************Shared variables***************************/
thread current;
// if we were to think of this process current state as a thread
context process;
//process.ID=
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
scheduler rr_sch = {NULL, NULL, rr_admit, rr_remove, rr_next};

void rr_admit(thread new){
	/* new - newly created thread*/
	thread last;
	/* Verifys - the thread has a validated thread_id*/
	if(new->tid > NO_THREAD){
		/* Case - nothing is in schedulers linked list*/
		if(!sch_tail && !sch_head){
			sch_tail=sch_head=new;
			sch_tail->lib_one=NULL;
			sch_tail->lib_two=NULL;
			return;
		/* Case - only one node in scheduler*/
		}else if(sch_head == sch_tail){
			sch_head->lib_two=new;
			sch_tail=new;
			sch_tail->lib_one = new;
			sch_tail->lib_two = sch_head;
		/* Case - at least two in schedular*/
		}else if(sch_head != sch_tail){
			/* Just have to use the tail*/
			last=sch_tail;
			last->lib_two=new;
			new->lib_one=last;
			new->lib_two=sch_head;
		}
	}
}
/* Description: Removes the passwed context from the scheduler's pool*/
void rr_remove(thread victim){
	thread right, left;
	/* Case - which isnt found*/
	if(!isInPool(victim))
		return;

	/* Case - when one or more in list*/
	/* Case - where one is in the list*/
	if(sch_head==sch_tail && sch_tail && sch_head){
		sch_head=sch_tail=NULL;
	/* Case - where at least two in list*/
	}else{
		/* Case - where victims the head*/
		if(victim==sch_head){
			right = sch_head->sched_two; // right one
			sch_tail->lib_two=right;
			right->sched_one=NULL;
			sch_head=right;
		/* Case - where victims the tail*/
		}else if( victim == sch_tail){
			left= sch_tail->sched_one;//left one
			left->sched_two=sch_head;
		/* Case -general case in the middle*/
		}else{
			left=victim->sched_one;
			right=victim->sched_two;	
			left->sched_two=right;
			right->sched_one=left;
		}
	}

}
/* Description: helper function to remove*/
int isInPool(thread victim){
	thread temp = sch_head;
	while(temp){
		if(temp==victim){
			return TRUE;
		}
		temp=temp->lib_two;
	}
	return FALSE;
}

/* Description: Retursn the next thread to be run or 
				NULL if there isnt one*/
thread rr_next(){
	thread temp;
	/* Case - empty pool*/
	if(!sch_head && !sch_tail){
		return NULL;
	}

	/*Case - at least one in pool*/

	/*Case - where current hasnt been assigned*/
	if(!current){
		current=sch_head;
	/*Case - where currrent has been assign*/
	}else{
		temp=current;		
		current=temp->lib_two;
	}
	return current;
}

//**************************************************************//
//************************LWP************************************//

thread lib_head;
/* lwp_create: 
			This function creates a new thread with function lwpfun
			It returns the Thread ID
*/
thread newThread(lwpfun fn, void *arg, size_t size){	

	static size_t thread_count = 0;
	thread new;
	if(!(new=new = calloc(1, sizeof(THREAD_INFO_SIZE))) && 
	   !(new->stack=calloc(size, sizeof(unsigned long)))){
		return NULL;
	}else{
		/*Set the new thread*/
		new->tid=thread_count++;
		new->stacksize = size;
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
tid_t lwp_create(lwpfun fn, void *arg, size_t size){
	thread new = newThread(fn, arg, size), tail;
	/* Case 1 - is to check to see if linked list is empty*/
	if(!lib_head){
		lib_head=new;
		lib_head->lib_one=NULL;
    /* Case 2 - general case if theres at least one in list*/
	}else{
		/* TODO : add passsed context to scheulers pool */
		rr_sch->admit(new);
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

	thread next;
	/* Saving contents of Orginal state before LWP*/
	/* Make a note to future self when removing check 
		to if list empty if so point head to NULL*/
	if(!(next=rr_sch->next())){
		lwp_exit();
	}else{
		swap_rfiles(&process.state, &next);
		current=next;
	}
} 

/*Description: Yields control to another lwp. WHich one depends on the schuler. 
				Savesthe current lwp's context, pick the next one, restoring that 
				threads context
*/
void lwp_yeild(){
	thread curr = current, next;
	/*next = sch.next()*/
	/* What is we dont have anymore thread in here*/
	if(next){
		swap_rfiles(&curr->state, &next->state);
	}else{
		lwp_stop();
	}
}

/*Description: Stops the lwp sys, resptres the org stack pointer 
				and returns to that context
				*/
	
void lwp_stop(){
	thread temp; 
	/* Get current state then write org process*/
	if(current){
		swap_rfiles(&current->state, &process);
	}else{
		swap_rfiles(NULL, &process);
	}
}


/*Description: Terminates the current process and frees 
				its resource(i.e. its stack). Calls sched->next()
				to get next thread, if no other threads,
				restores the orginal system thread
				*/
void lwp_exit(){
	/* if a current exists then*/
	thread next;
	if(current){
		/* There has to be a thread left */
		rr_sch->remove(current);
		free(current->stack);
		free(current);
		/* restore the org system thread */
		if(!(next=rr_sch->next())){
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



thread tid2thread(tid_t id){
	thread id_temp = lib_head;
	if(id > 0 && lib_head){
		while(id_temp){
			if(id_temp->tid == id){
				return id_temp;
			}
			id_temp=id_temp->lib_one;
		}
	}
	return NULL;
}

//**************************************************************//


int main(){

	#define STACK_SIZE 1000

	int i;
	for(i=0; i<2; i++)
		lwp_create(say_hi, NULL, STACK_SIZE);


	return 0;
}

// Prints out its tid
void say_hi(void *arg){
	int i;
	for(i<0; i<4; i++){
		printf("%d", current->tid);
	}
}
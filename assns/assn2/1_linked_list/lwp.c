#include "lwp.h"
#include <stdio.h>
#include <stdlib.h>


#define THREAD_INFO_SIZE sizeof(struct threadinfo_st)

/**********************Shared variables***************************/
thread current = NULL;
// if we were to think of this process current state as a thread

rfile orginal;
/***************************************************************/


//********************Scheduler**********************************//
thread tail = NULL, head = NULL; // used to keep track of queue
/** NULL<-[head]-><-[arbitrary]-><-[tail]->(points to head) 
*		(1) 		(2)
*/

void rr_admit(thread new){
	/* new - newly created thread*/
	thread last;
	/* Verifys - the thread has a validated thread_id*/
	if(new->tid > NO_THREAD){
		/* Case - nothing is in schedulers linked list*/
		if(!tail && !head){
			tail=head=new;
			tail->sched_one=NULL;
			tail->sched_two=new;
			return;
		/* Case - only one node in scheduler*/
		}else if(head == tail){
			head->sched_two=new;
			tail=new;
			tail->sched_one = head;
			tail->sched_two=head;
		/* Case - at least two in schedular*/
		}else if(head != tail){
			/* Just have to use the tail*/
			last=tail;
			last->sched_two=new;
			new->sched_one=last;
			new->sched_two=head;
			tail=new;
		}
	}
}
/* Description: helper function to remove*/
int isInPool(thread victim){
	if(head==victim){
		return TRUE;
	}else{
		thread temp = head->sched_two;
		while(temp != head){
			if(temp==victim){
				return TRUE;
			}
			temp=temp->sched_two;
		}
	}
	return FALSE;
}
/* Description: Removes the passwed context from the scheduler's pool*/
void rr_remove(thread victim){
	thread right, left;
	/* Case - which isnt found*/
	if(!isInPool(victim))
		return;

	/* Case - when one or more in list*/
	/* Case - where one is in the list*/
	if(head==tail && tail && head){
		//current=NULL;
		head=tail=NULL;
	/* Case - where at least two in list*/
	}else{
		/* Case - where victims the head*/
		if(victim==head){
			right = head->sched_two; // right one
			tail->sched_two=right;
			right->sched_one=NULL;
			head=right;
		/* Case - where victims the tail*/
		}else if( victim == tail){
			left= tail->sched_one;//left one
			left->sched_two=head;
			tail=left;
		/* Case -general case in the middle*/
		}else{
			left=victim->sched_one;
			right=victim->sched_two;	
			left->sched_two=right;
			right->sched_one=left;
		}
	}

}


/* Description: Retursn the next thread to be run or 
				NULL if there isnt one(used to assign curent)*/
thread rr_next(){
	thread temp, next;
	/* Case - empty pool*/
	if(!head && !tail){
		return NULL;
	}

	/*Case - at least one in pool*/

	/*Case - where current hasnt been assigned*/
	if(!current){
		next=head;
	/*Case - where currrent has been assign*/
	}else{
		temp=current;
		next=temp->sched_two;
	}
	return next;
}
struct scheduler rr_sched = {NULL, NULL, rr_admit, rr_remove, rr_next};
scheduler sched = &rr_sched;

//**************************************************************//
//************************LWP************************************//

/* lwp_create: 
			This function creates a new thread with function lwpfun
			It returns the Thread ID
*/
thread newThread(lwpfun fn, void *arg, size_t size){	

	static size_t thread_count = 1;
	unsigned long *temp_stack;
	thread new;
	if(!(new=calloc(1, sizeof(struct threadinfo_st)))){
	   	return NULL;
	}else if(!(new->stack=calloc(size, sizeof(unsigned long)))){
		(new);
		return NULL;
	}else{
		/*Set the new thread*/
		new->tid=thread_count++;
		new->stacksize = size;
		new->lib_one=NULL;
		new->lib_two=NULL;


		/* Init stack */
		temp_stack=((unsigned long*)(new->stack))+new->stacksize;
		temp_stack--;
		*temp_stack = lwp_exit;
		temp_stack--;
		*temp_stack = fn; 		
		temp_stack--;


		/* Register stuff */
		new->state.rdi = arg;
		new->state.rbp=temp_stack;
		new->state.fxsave=FPU_INIT;

		return new;
	}

}
/*Description: creates a new lwp which executes the given 
				function with given arguments. The new proces
				stack will be stacksize
returns: lwp_id of the new thread OR -1 if cant be made
				*/
tid_t lwp_create(lwpfun fn, void *arg, size_t size){
	thread new;
	if((new= newThread(fn, arg, size))){
		sched->admit(new);
	}else{
		return -1;
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
	if(!(next=sched->next())){
		lwp_exit();
	}else{
		current=next;
		swap_rfiles(&orginal, &(next->state));
	}
} 

/*Description: Yields control to another lwp. 
 *				Which one depends on the schuler. 
 *				Saves the current lwp's context, 
 *				pick the next one, restoring that 
 *				threads context
*/
void lwp_yield(){
	thread curr = current, next;
	/* What is we dont have anymore thread in here*/
	if((next=sched->next())){
		current=next;
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
		swap_rfiles(&current->state, &orginal);
	}else{
		//swap_rfiles(NULL, &process.state);
		return;
	}
}


/*Description: Terminates the current process and s 
				its resource(i.e. its stack). 
				Calls sched->next()
				to get next thread, if no other threads,
				restores the orginal system thread
				*/
void lwp_exit(){
	/* if a current exists then*/
	thread next, curr;
	unsigned long *stack;
	if(current){
		sched->remove(current);
		stack=current->stack;
		curr=current;
		/* remove from sch list */
	
	/* restore the org system thread */
		if(!(next=sched->next())){
			current=NULL;
			(stack);
			(curr);
			swap_rfiles(NULL, &orginal);
		/* Set next to the current thread*/
		}else{
			/* We dont care what was previous in address*/
			current=next;
			//(_stack);
			//(_curr);
			swap_rfiles(NULL, &next->state);
		}

	}
}

thread tid2thread(tid_t id){
	thread id_temp = head;
	if(id <= 0 && !id_temp){
		return NULL;
	}

	if(head == id){
		return head;
	}else{
		id_temp=head->sched_two;
		while(id_temp != head){
		if(id_temp->tid == id){
			return id_temp;
		}
		id_temp=id_temp->lib_two;	
	}
	return NULL;
	}
}

tid_t lwp_gettid(){
	if(current){
		return current->tid;
	}else{
		return NO_THREAD;
	}
}
/* Description: Casues the LWP package to use the given
				scheduler to choose the next process to run
				Transfers all threads from the old scheduler
				to the new one in next() order. If scheduler 
				is NULL the library should return to 
				round-robin scheduling*/
void lwp_set_scheduler(scheduler sch){
	thread next;
	if(sch!=NULL){
		/* Transfer control*/
		if(sch->init){
			sch->init();
		}
		while((next=sched->next())){
			sched->remove(next);
			sch->admit(next);
		}
		if(sched->shutdown){
			sched->shutdown();
		}
			sched=sch;
	}else{
		sched=&rr_sched;
	}	
}

/* Description: Returns the pointer to the current scheduler*/
scheduler lwp_get_scheduler(){
	return sched;
}
//**************************************************************//


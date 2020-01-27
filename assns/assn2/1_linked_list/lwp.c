#include "lwp.h"
#include "scheduler.h"
#include <stdio.h>
#include <stdlib.h>


static rfile orginal;

#define lt_next lib_two
#define lt_prev lib_one
static thread head = NULL, tail = NULL, current = NULL;

extern void rr_admit(thread new);
extern void rr_remove(thread victim);
extern thread rr_next();


static struct scheduler rr_sched = {NULL, NULL, rr_admit, rr_remove, rr_next};
static scheduler sched = &rr_sched;
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
		free(new);
		return NULL;
	}else{
		/*Set the new thread*/
		new->tid=thread_count++;
		new->stacksize = size;
		new->lt_prev=NULL;
		new->lt_next=NULL;


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
		new->state.rsp=temp_stack;
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
		/* Case - head and tail are empty(zero on list)*/
		if(!head && !tail){
			head=tail=new;
			new->lt_prev=tail;
			new->lt_next=tail;
		/* Case - head == fail(only one is list)*/
		}else if(head==tail){
			tail=new;	
			new->lt_prev=head;
			new->lt_next=head;
			head->lt_prev=tail;
			head->lt_next=tail;
		/* Case - 2 or more in list*/
		}else{
			tail->lt_next=new;
			new->lt_prev=tail;
			head->lt_prev=new;
			new->lt_next=head;
			tail=new;
		}
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
	/* Saving contents of Orginal state before LWP*/
	/* Make a note to future self when removing check 
		to if list empty if so point head to NULL*/
	if(!(current=sched->next())){
		lwp_exit();
	}else{
		swap_rfiles(&orginal, &current->state);
	}
} 

/*Description: Yields control to another lwp. 
 *				Which one depends on the schuler. 
 *				Saves the current lwp's context, 
 *				pick the next one, restoring that 
 *				threads context
*/
void lwp_yield(){
	thread prev = current;
	/* What is we dont have anymore thread in here*/
	if((current=sched->next())){
		swap_rfiles(&prev->state, &current->state);
	}else{
		lwp_stop();
	}
}

/*Description: Stops the lwp sys, resptres the org stack pointer 
				and returns to that context
				*/
	
void lwp_stop(){
	/* Get current state then write org process*/
	if(current){
		swap_rfiles(&current->state, &orginal);
	}else{
		swap_rfiles(NULL, &orginal); // TODO : DISABLE
	}
}

void remove_lt_thread(thread victim){
	thread right, left;
	/* Case - where one in list*/
	if(head==tail && tail && head){
		current=NULL;
		head=tail=NULL;
	/* Case - where at least two in list*/
	}else{
		/* Case - where victims the head*/
		if(victim==head){
			right = head->lt_next; // right one
			tail->lt_next=right;
			right->lt_prev=tail;
			head=right;
		/* Case - where victims the tail*/
		}else if( victim == tail){
			left= tail->lt_prev;//left one
			left->lt_next=head;
			tail=left;
		/* Case -general case in the middle*/
		}else{
			left=victim->lt_prev;
			right=victim->lt_next;	
			left->lt_next=right;
			right->lt_prev=left;
		}
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
		remove_lt_thread(current);
	/* restore the org system thread */
		if(!(current=sched->next())){
			(stack);
			(curr);
			swap_rfiles(NULL, &orginal);
		/* Set next to the current thread*/
		}else{
			/* We dont care what was previous in address*/
			//(_stack);
			//(_curr);
			swap_rfiles(NULL, &current->state);
		}

	}
}

thread tid2thread(tid_t id){
	thread id_temp = current;
	if(id <= 0 && !id_temp){
		return NULL;
	}else{
		if(id_temp->tid == id){
			return id_temp;
		}else{
			id_temp=id_temp->lt_next;
			while(id_temp != current){
				if(id_temp->tid == id){
					return id_temp;
				}
			id_temp=id_temp->lt_next;	
			}
		}
	}
	return NULL;
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
		while((current=sched->next())){
			sched->remove(current);
			sch->admit(current);
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


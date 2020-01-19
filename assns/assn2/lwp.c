#include "lwp.h"
#include <stdio.h>
#include <stdlib.h>


#define THREAD_INFO_SIZE sizeof(struct threadinfo_st)

/**********************Shared variables***************************/
thread current = NULL;
// if we were to think of this process current state as a thread
context process ;

//process.ID=
// TODO COME BACK HERE
/***************************************************************/


//********************Scheduler**********************************//

thread sch_tail = NULL, sch_head = NULL; // used to keep track of queue
/** NULL<-[sch_head]-><-[arbitrary]-><-[sch_tail]->(points to head) 
* 
*		(1) 		(2)
*/


void rr_admit(thread new){
	/* new - newly created thread*/
	thread last;
	/* Verifys - the thread has a validated thread_id*/
	if(new->tid > NO_THREAD){
		/* Case - nothing is in schedulers linked list*/
		if(!sch_tail && !sch_head){
			sch_tail=sch_head=new;
			sch_tail->sched_one=NULL;
			sch_tail->sched_two=new;
			return;
		/* Case - only one node in scheduler*/
		}else if(sch_head == sch_tail){
			sch_head->sched_two=new;
			sch_tail=new;
			sch_tail->sched_one = sch_head;
			sch_tail->sched_two=sch_head;
		/* Case - at least two in schedular*/
		}else if(sch_head != sch_tail){
			/* Just have to use the tail*/
			last=sch_tail;
			last->sched_two=new;
			new->sched_one=last;
			new->sched_two=sch_head;
			sch_tail=new;
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
		temp=temp->sched_two;
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
	if(sch_head==sch_tail && sch_tail && sch_head){
		sch_head=sch_tail=NULL;
	/* Case - where at least two in list*/
	}else{
		/* Case - where victims the head*/
		if(victim==sch_head){
			right = sch_head->sched_two; // right one
			sch_tail->sched_two=right;
			right->sched_one=NULL;
			sch_head=right;
		/* Case - where victims the tail*/
		}else if( victim == sch_tail){
			left= sch_tail->sched_one;//left one
			left->sched_two=sch_head;
			sch_tail=left;
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
				NULL if there isnt one*/
thread rr_next(){
	thread temp;
	/* Case - empty pool*/
	if(!sch_head && !sch_tail){
		current=NULL;
	}

	/*Case - at least one in pool*/

	/*Case - where current hasnt been assigned*/
	if(!current){
		current=sch_head;
	/*Case - where currrent has been assign*/
	}else{
		temp=current;		
		current=temp->sched_two;
	}
	return current;
}
struct scheduler rr_sched = {NULL, NULL, rr_admit, rr_remove, rr_next};
scheduler sched = &rr_sched;

//**************************************************************//
//************************LWP************************************//

/***NULL<-[lib_head]-><-[arbitrary]-><-[lib_tail]->(NULL)
 *		(1) 		(2)
 * 
*/
thread lib_head = NULL, lib_tail = NULL;
/* lwp_create: 
			This function creates a new thread with function lwpfun
			It returns the Thread ID
*/
thread newThread(lwpfun fn, void *arg, size_t size){	

	static size_t thread_count = 1;
	unsigned long *temp_stack;
	thread new;
	if(!(new=calloc(1, THREAD_INFO_SIZE))){
	   	return NULL;
	}else if(!(new->stack=calloc(size, sizeof(unsigned long)))){
		return NULL;
	}else{
		/*Set the new thread*/
		new->tid=thread_count++;
		new->stacksize = size;
		new->lib_one=NULL;
		new->lib_two=NULL;


		/* Init stack */
		temp_stack=(new->stack+new->stacksize -1);

		*temp_stack = lwp_exit;
		temp_stack--;
		*temp_stack = fn;
		temp_stack--;

		/* After Leave: sp takes rbp 
		 * and adds 8 (i.e go next space in stack)*/
		/* After ret: sp is put into */
		

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
	thread new = newThread(fn, arg, size), temp;
	/* Case 1 - is to check to see if linked list is empty*/
	if(!lib_head){
		lib_head=new;
		lib_tail=lib_head;
		lib_head->lib_one=NULL;
		lib_head->lib_two=NULL;
	/* Case 2 - if one in the list*/
	}else if(lib_head==lib_tail){
		lib_head->lib_two=new;
		new->lib_one=lib_head;
		lib_tail=new;
    /* Case 3 - general case if theres at least two in list*/
	}else{
		temp=lib_tail;
		temp->lib_two=new;
		new->lib_one=temp;
		lib_tail=new;
	}
	sched->admit(new);
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
		swap_rfiles(&process.state, &next->state);
		//current=next;
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
		swap_rfiles(&current->state, &process.state);
	}else{
		swap_rfiles(NULL, &process.state);
	}
}
/*Description: helper function for lwp_exit;
				This assumes current as not null*/
void remove_from_lib_list(thread current){
	thread right, left;
	/* Case 1 - only one in the list*/
	if( lib_head==lib_tail && current==lib_tail){
		/* Implicitly saying current == head*/
		lib_tail = lib_head=NULL;
	/* Case 2 - if more than one in list*/
	}else{
		/* Case 2.1 - if current is lib_head*/
		if(current==lib_head){
			right=lib_head->lib_two;
			right->lib_one=NULL;
			lib_head=right;
        /* Case 2.2 - if current is tail of lib*/
		}else if(current==lib_tail){
			left=lib_tail->lib_one;
			left->lib_two=NULL;	
			lib_tail=left;
        /* Case 2.3 - if current is in the middle of list*/
		}else{
			right=current->lib_two;
			left=current->lib_one;
			right->lib_one=left;
			left->lib_two=right;
		}
	}

}



/*Description: Terminates the current process and frees 
				its resource(i.e. its stack). 
				Calls sched->next()
				to get next thread, if no other threads,
				restores the orginal system thread
				*/
void lwp_exit(){
	/* if a current exists then*/
	thread next;
	if(current){
		/* remove from lib list */
		remove_from_lib_list(current);
		/* remove from sch list */
		sched->remove(current);
		free(current->stack);
		free(current);
		/* restore the org system thread */
		if(!(next=sched->next())){
			swap_rfiles(NULL, &process.state);
			current=NULL; // no more in linked list
		/* Set next to the current thread*/
		}else{
			/* We dont care what was previous in address*/
			current=next;
			swap_rfiles(NULL, &current->state);
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
			id_temp=id_temp->lib_two;
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
	if(sch){
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

void say_hi(void *arg);

static void indentnum(uintptr_t num);

/*
int main(){

	#define STACK_SIZE 1000
	process.state.fxsave=FPU_INIT;

	int i;

	int j=4;

	lwp_create(say_hi, &j, STACK_SIZE);
	for(i=0; i<4; i++)
		lwp_create(say_hi, &j, STACK_SIZE);

	lwp_start();

	return 0;
}
*/

void say_hi(void * arg){

  int howfar,i;
	printf("%s", "Greetings from Thread ");
	printf("%ld\n", current->tid);
	lwp_yield();
	printf("%s","IM still avlive" );
	lwp_exit();
}
static void indentnum(uintptr_t num) {
  /* print the number num num times, indented by 5*num spaces
   * Not terribly interesting, but it is instructive.
   */
  int howfar,i;

  howfar=(int)num;              /* interpret num as an integer */
  for(i=0;i<howfar;i++){
    printf("%*d\n",howfar*5,howfar);
    lwp_yield();                /* let another have a turn */
  }
  lwp_exit();                   /* bail when done.  This should
                                 * be unnecessary if the stack has
                                 * been properly prepared
                                 */
}

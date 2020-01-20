#include "lwp.h"
#include <stdio.h>

thread current;
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
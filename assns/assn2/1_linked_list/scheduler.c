#include "scheduler.h"
#include <stdio.h>

static thread tail = NULL, head = NULL; 

void rr_admit(thread new){
	/* new - newly created thread*/
	thread last;
	/* Verifys - the thread has a validated thread_id*/
	if(new->tid > NO_THREAD){
		/* Case - nothing is in schedulers linked list*/
		if(!tail && !head){
			tail=head=new;
			tail->st_prev=NULL;
			tail->st_next=new;
			return;
		/* Case - only one node in scheduler*/
		}else if(head == tail){
			head->st_next=new;
			tail=new;
			tail->st_prev = head;
			tail->st_next=head;
		/* Case - at least two in schedular*/
		}else if(head != tail){
			/* Just have to use the tail*/
			last=tail;
			last->st_next=new;
			new->st_prev=last;
			new->st_next=head;
			tail=new;
		}
	}
}
/* Description: helper function to remove*/
int isInPool(thread victim){
	if(head==victim){
		return TRUE;
	}else{
		thread temp = head->st_next;
		while(temp != head){
			if(temp==victim){
				return TRUE;
			}
			temp=temp->st_next;
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
			right = head->st_next; // right one
			tail->st_next=right;
			right->st_prev=NULL;
			head=right;
		/* Case - where victims the tail*/
		}else if( victim == tail){
			left= tail->st_prev;//left one
			left->st_next=head;
			tail=left;
		/* Case -general case in the middle*/
		}else{
			left=victim->st_prev;
			right=victim->st_next;	
			left->st_next=right;
			right->st_prev=left;
		}
	}

}


/* Description: Retursn the next thread to be run or 
				NULL if there isnt one(used to assign curent)*/
thread rr_next(){
    static thread next;
	/* Case - empty pool*/
	if(!head && !tail){
		return NULL;
	}

	/*Case - at least one in pool*/

	/*Case - where current hasnt been assigned*/
	if(!next){
		next=head;
	/*Case - where currrent has been assign*/
	}else{
		next=next->st_next;
	}
	return next;
}




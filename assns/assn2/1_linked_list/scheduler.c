#include "scheduler.h"
#include "lwp.h"
#include <stdio.h>


static thread st_tail = NULL, st_head = NULL, st_current=NULL;
static int removed = FALSE;


void rr_admit(thread new){
	/* new - newly created thread*/
	thread last;
	/* Verifys - the thread has a validated thread_id*/
	if(new->tid > NO_THREAD){
		/* Case - nothing is in schedulers linked list*/
		if(!st_tail && !st_head){
			st_tail=st_head=new;
			st_tail->st_prev=new;
			st_tail->st_next=new;
			return;
		/* Case - only one node in scheduler*/
		}else if(st_head == st_tail){
			st_tail=new;
			st_head->st_next=new;
            st_head->st_prev=st_tail;
			st_tail->st_prev = st_head;
			st_tail->st_next=st_head;
		/* Case - at least two in schedular*/
		}else if(st_head != st_tail){
			/* Just have to use the st_tail*/
			last=st_tail;
			last->st_next=new;
			new->st_prev=last;
			new->st_next=st_head;
            st_head->st_prev=new;
			st_tail=new;
		}
	}
}
/* Description: helper function to remove*/
int isInPool(thread victim){
	if(st_head==victim){
		return TRUE;
	}else{
		thread temp = st_head->st_next;
		while(temp != st_head){
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
	if(st_head==st_tail && st_tail && st_head){
		st_head=st_tail=NULL;
        st_current=NULL;
	/* Case - where at least two in list*/
	}else{
		/* Case - where victims the st_head*/
		if(victim==st_head){
			right = st_head->st_next; // right one
			st_tail->st_next=right;
			right->st_prev=st_tail;
			st_head=right;
            st_current=st_head;
		/* Case - where victims the st_tail*/
		}else if( victim == st_tail){
			left= st_tail->st_prev;//left one
			left->st_next=st_head;
			st_tail=left;
            st_current=st_head;
		/* Case -general case in the middle*/
		}else{
			left=victim->st_prev;
			right=victim->st_next;	
			left->st_next=right;
			right->st_prev=left;
            st_current=right;
		}
        removed=TRUE;
	}
}


/* Description: Retursn the next thread to be run or 
				NULL if there isnt one(used to assign curent)*/
thread rr_next(){
   	/* Case - empty pool*/

	if(!st_head && !st_tail){
		return NULL;
    }
    if(!st_current){
        st_current=st_head;
    /*Case - not removed (set in remove function)*/
    }else if (!(removed)){
        st_current = st_current->st_next;
    }
    removed=FALSE;
	/*Case - at least one in pool*/
        return st_current;
}



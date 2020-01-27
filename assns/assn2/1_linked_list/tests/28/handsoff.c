/* This is a round robin scheduler for up to THREAD_MAX threads that
 * wipes out sched_one and sched_two to make sure that the library
 * isn't using those pointers.  Since the most likely place for the
 * library to do that is tid2thread(), we check each thread's tid
 * each time next() is called.
 */

#include "../../lwp.h"
#include <stdlib.h>
#include <stdio.h>
#include "handsoff.h"
#define THREAD_MAX 1000
static thread qhead=NULL;

#define tnext sched_one
#define tprev sched_two

static int count=0;
static int current_v=0;
static thread table[THREAD_MAX];
static tid_t  id[THREAD_MAX];

/* linux supports only 47 bits of VM so this is guaranteed to be a
 * bogus pointer
 */
#define BOGUS (void*)((0xFFl<<47)|0xFF)

static void s_admit(thread new) {
  if ( count >= THREAD_MAX ) {
    /* print an error message and return */
    fprintf(stderr,"%s: thread table full\n",__FUNCTION__);
    return;
  }
  id[count] = new->tid;
  table[count] = new;
  count++;
  /* wipe out sched_one and sched_two with hopefully bogus non-null values */
  new->sched_one = BOGUS;
  new->sched_two = BOGUS;
}

static void s_remove(thread victim) {
  /* remove from the tables and advance the tables */
  int i;
  for(i=0;i<count && victim != table[i];i++)
    /* whee */ ;
  for (;i<count-1;i++) {
    table[i]=table[i+1];
    id[i]=id[i+1];
  }
  count--;
}

static thread s_next() {
  thread next = NULL;
  if (count) {
    /* Test that tid2thread() is working */
    if ( table[current_v] != tid2thread(id[current_v]) ) {
      fprintf(stderr,"tid2thread() is returning bogus values\n");
    }
    /* the scheduling part */
    next = table[current_v];
    current_v = (current_v+1)%count;
  }
  return next;
}

static struct scheduler publish = {NULL,NULL,s_admit,s_remove,s_next};
scheduler Handsoff=&publish;

/*********************************************************/
__attribute__ ((unused))
void az_dp() {
  thread l;
  if ( !qhead )
    fprintf(stderr,"  AZ qhead is NULL\n");
  else {
    fprintf(stderr,"  AZ queue:\n");
    l = qhead;
    do {
      fprintf(stderr,"  (tid=%lu)\n", l->tid);
      l=l->tnext;
    } while ( l != qhead ) ;
    fprintf(stderr,"\n");
  }
}


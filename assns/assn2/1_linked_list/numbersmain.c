/*
 * Author: Dr. Phillip Nico
 *         Department of Computer Science
 *         California Polytechnic State University
 *         One Grand Avenue.
 *         San Luis Obispo, CA  93407  USA
 *
 * Email:  pnico@csc.calpoly.edu
 *
 * Revision History:
 *         $Log: main.c,v $
 *         Revision 1.2  2004-04-13 12:31:50-07  pnico
 *         checkpointing with listener
 *
 *         Revision 1.1  2004-04-13 09:53:55-07  pnico
 *         Initial revision
 *
 *         Revision 1.1  2004-04-13 09:52:46-07  pnico
 *         Initial revision
 *
 */

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "lwp.h"
#include <math.h>

#define MAXSNAKES  100
#define INITIALSTACK 2048

extern thread current;
typedef void (*sigfun)(int signum);
static void indentnum(uintptr_t num);

void test14(uintptr_t ptr);

void test13_call();

void test13(void *ptr){
  static int i=0;


  printf("Greetings from Thread %ld Yielding\n", current->tid);
  lwp_yield();
  printf("I (%ld)am still alive\n", current->tid);
  lwp_exit();
}

/*
void test13_call(){
  int j,i;
  for(j=0; j<100; j++){
    for(i=0;i<10;i++){
      lwp_create((lwpfun)test13,(void*)i,10000000000000);
    }
    lwp_start();
    lwp_exit();
}
*/




void test14(uintptr_t ptr){
  printf("Greetings from ThreadYielding\n");
  lwp_yield();
  //printf("I  still alive. Goodbye.\n");
  lwp_exit();
}
static void indentnum(uintptr_t num) {
  /* print the number num num times, indented by 5*num spaces
   * Not terribly interesting, but it is instructive.
   */
  int id;
  id = (int)num;
  printf("Greetings from Thread %d.  Yielding...\n",id);
  lwp_yield();
  printf("I (%d) am still alive.  Goodbye.\n",id);
  lwp_exit();
}


int main(int argc, char *argv[]){
  long i;

  /* spawn a number of individual LWPs */
  for(i=0;i<5;i++)
    lwp_create((lwpfun)indentnum,(void*)i,INITIALSTACK);

  lwp_start();

  printf("LWPs have ended.\n");
  return 0;
}


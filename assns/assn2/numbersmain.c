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

typedef void (*sigfun)(int signum);
static void indentnum(uintptr_t num);

void test14(uintptr_t ptr);
int main(int argc, char *argv[]){
  long i;

  for (i=1;i<argc;i++) {                /* check options */
    fprintf(stderr,"%s: unknown option\n",argv[i]);
    exit(-1);
  }

  printf("Launching LWPS\n");

  for(i=1;i<6;i++) {
    lwp_create((lwpfun)test14,(void*)i,INITIALSTACK);
  }
  lwp_start();                     /* returns when the last lwp exits */

  //printf("First round complete. Restarting with NO q")

  int j=1;
  for(;j<6;j++){
    lwp_create((lwpfun)test14,(void*)j,INITIALSTACK);
  }

  lwp_start();                     /* returns when the last lwp exits */
  

  printf("Back from LWPS.\n");
  return 0;
}

void test14(uintptr_t ptr){
  printf("Greetings from ThreadYielding\n");
  lwp_yield();
  printf("I  still alive. Goodbye.\n");
  lwp_exit();
}
static void indentnum(uintptr_t num) {
  /* print the number num num times, indented by 5*num spaces
   * Not terribly interesting, but it is instructive.
   */
  int howfar,i, abs_val;

  howfar=(int)num;              /* interpret num as an integer */

/*
  abs_val=abs(howfar);
  for(i=-6;i<howfar;i++){
    printf("%*d\n",abs_val*5,abs_val);
    lwp_yield();                
  abs_val=abs(howfar);
  for(i=0;i<howfar;i++){
    printf("%*d\n",abs_val*5,abs_val);
  }
  */
}


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <limits.h>
#include <float.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <libgen.h>

#include "types.h"
#include "parser.h"

#define NO_EXIT EXIT_SUCCESS

void usage(int doExit)
{
   printf("usage: ./minls  [ -v ] [ -p num [ -s num ] ] imagefile [ path ]\n");

   if(doExit == EXIT_FAILURE)
      exit(EXIT_FAILURE);
}

void help()
{
   usage(NO_EXIT);

   printf("Options:\n");
   printf("%7s %-8s %-7s %s\n", "", "-p", "part",
         "--- select partition for filesystem (default: none)" );
   printf("%7s %-8s %-7s %s\n", "", "-s", "sub",
         "--- select subpartition for filesystem (default: none)" );
   printf("%7s %-8s %-7s %s\n", "", "-h", "help",
         "--- print usage information and exit" );
   printf("%7s %-8s %-7s %s\n", "", "-v", "verbose",
         "--- increase verbosity level" );

   exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {

   printf("Hello World\n");
   return 0;

}

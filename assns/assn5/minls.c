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
#include <getopt.h>
#include <ctype.h>
#include <errno.h>

#include "types.h"
#include "parser.h"
#include "shared.h"

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


int main(int argc, char *argv[])
{
   minix_t minix;
   initOpt(&minix.opt);
   getArgs(argc, argv, &minix.opt);

   set_minix_types(&minix);

   char default_path[2]  = {'/', 0};
   if(minix.opt.srcpath == NULL){
      minix.opt.srcpath = default_path;
   }

   // step 1 - find file output it
   print_all(&minix);

   free(minix.inodes);
   fclose(minix.image);
   return EXIT_SUCCESS;
}


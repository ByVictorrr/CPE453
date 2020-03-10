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


void usage(int doExit, char * argv[])
{
   printf("usage: %s  [ -v ] [ -p num [ -s num ] ] imagefile [ path ]\n", argv[0]);

   if(doExit == EXIT_FAILURE)
      exit(EXIT_FAILURE);
}

void help(char *argv[])
{
   usage(NO_EXIT, argv);

   fprintf(stderr, "Options:\n");
   fprintf(stderr, "%7s %-8s %-7s %s\n", "", "-p", "part",
         "--- select partition for filesystem (default: none)" );
   fprintf(stderr, "%7s %-8s %-7s %s\n", "", "-s", "sub",
         "--- select subpartition for filesystem (default: none)" );
   fprintf(stderr, "%7s %-8s %-7s %s\n", "", "-h", "help",
         "--- print usage information and exit" );
   fprintf(stderr, "%7s %-8s %-7s %s\n", "", "-v", "verbose",
         "--- increase verbosity level" );

   exit(EXIT_FAILURE);
}


void handleLeftOverArgs(int argc, char *argv[], options_t * opt)
{
   int i;

   /* handle non-option args */
   for(i = 0 ; optind < argc; i++, optind++){
      if(i == 0)
         opt->imagefile = argv[optind];
      if(i == 1)
         opt->srcpath = argv[optind];
   }
   if(i > 2 || i < 1)
      help(argv);
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


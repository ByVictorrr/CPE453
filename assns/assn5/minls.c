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

#include "minfs.h"

#define NO_EXIT EXIT_SUCCESS

/* I might make this NOT global */
struct
{
   int   part;
   int   subpart;
   char  *imagefile;
   char  *srcpath;
   char  *path;
   int   verbose;

} argInfo;


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

long getValue(char *input)
{
   char *ptr = input;
   long val = -1;

   while (*ptr) {
      if (isdigit(*ptr) || *ptr == '-') { /* allow for negative numbers */
         val = strtol(ptr, &ptr, 10);
      }
      else {
         fprintf(stderr, "%s: badly formed integer.\n", input);
         help();
         ptr++;
      }
   }
   return val;
}


void getArgs(int argc, char *argv[])
{
   int opt_index = 0;
   int i = 0;

   argInfo.part = -1;
   argInfo.subpart = -1;
   argInfo.imagefile = NULL;
   argInfo.srcpath = NULL;
   argInfo.path = NULL;
   argInfo.verbose = 0;

   /*funky: to indicate that case requires an optarg write: "s:" */

   while((opt_index = getopt(argc, argv, ":p:s:vh")) != -1)
      switch(opt_index)
         {
            case 'h':
               help();
               break;

            case 'v':
               argInfo.verbose = 1;
               break;

            case 'p':
               argInfo.part = getValue(optarg); /* getValue handles type error
                                                   (badly formed Integer)*/
               if(argInfo.part < 0 || argInfo.part > 3) {
                  fprintf(stderr, "Partition %d out of range.  %s\n",
                        argInfo.part, "Must be 0..3.");
                  help();
               }
               printf("part: %s\n", optarg);
               break;

            /* NEEDED */
            case 's':
               argInfo.subpart = getValue(optarg); /* getValue handles type error
                                                   (badly formed Integer)*/
               if(argInfo.subpart < 0 || argInfo.subpart > 3) {
                  fprintf(stderr, "Subpartition %d out of range.  %s\n",
                        argInfo.subpart, "Must be 0..3.");
                  help();
               }
               printf("subpart: %s\n", optarg);
               break;

            case ':':
               printf("option needs a value\n");
               break;

            case '?':
               printf("unknown option: %c\n", optopt);
               usage(EXIT_FAILURE);
               break;
         }

   if(argInfo.subpart != -1 && argInfo.part == -1)
      fprintf(stderr, "Cannot have a subpartition without a partition.");


   /* handle non-option args */
   for(i = 0 ; optind < argc; i++, optind++){
      if(i == 0)
         argInfo.imagefile = argv[optind];
      if(i == 1)
         argInfo.srcpath = argv[optind];
      if(i == 2)
         argInfo.path = argv[optind];
      printf("extra arguments: %s\n", argv[optind]);
   }
}


int main(int argc, char *argv[])
{
   getArgs(argc, argv);
   return EXIT_SUCCESS;
}


void newfunction(int i) {


}

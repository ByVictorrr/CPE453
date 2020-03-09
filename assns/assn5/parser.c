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

#define NO_EXIT EXIT_SUCCESS

void usage(int doExit);
void help();

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

void initOpt(options_t * opt)
{
   opt->part = -1;
   opt->subpart = -1;
   opt->imagefile = NULL;
   opt->srcpath = NULL;
   opt->dstpath = NULL;
   opt->verbosity = 0;
}


void handleLeftOverArgs(int argc, char *argv[], options_t * opt)
{
   int i;

   if(opt->subpart != -1 && opt->part == -1)
      fprintf(stderr, "Cannot have a subpartition without a partition.");

   /* handle non-option args */
   for(i = 0 ; optind < argc; i++, optind++){
      if(i == 0)
         opt->imagefile = argv[optind];
      if(i == 1)
         opt->srcpath = argv[optind];
      if(i == 2)
         opt->dstpath = argv[optind];
      printf("extra arguments: %s\n", argv[optind]);
   }
}

void getArgs(int argc, char *argv[], options_t * opt)
{
   int opt_index = 0;


   while((opt_index = getopt(argc, argv, ":p:s:vh")) != -1)
      switch(opt_index)
         {
            case 'h':
               help();
               break;

            case 'v':
               opt->verbosity++;
               break;

            case 'p':
               opt->part = getValue(optarg); /* getValue handles type error
                                                   (badly formed Integer)*/
               if(opt->part < 0 || opt->part > 3) {
                  fprintf(stderr, "Partition %d out of range.  %s\n",
                        opt->part, "Must be 0..3.");
                  help();
               }
               break;

            /* NEEDED */
            case 's':
               opt->subpart = getValue(optarg); /* getValue handles type error
                                                   (badly formed Integer)*/
               if(opt->subpart < 0 || opt->subpart > 3) {
                  fprintf(stderr, "Subpartition %d out of range.  %s\n",
                        opt->subpart, "Must be 0..3.");
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

         handleLeftOverArgs(argc, argv, opt);


         /* REMOVE: This is for debuging */
         minix_t minix;
         minix.opt = opt;
         print_options(minix);
}


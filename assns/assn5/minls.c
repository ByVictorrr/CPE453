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
   int   verbose_flag;

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
   printf("%*s%*s%*s\n", 15, "-p", 20, "part", 30, "--- select partition for filesystem (default: none)" );
   printf("%8s %6s %20s \n", "-s", "sub", "--- select partition for filesystem (default: none)" );
   printf("%8s%7s%3s\n", "-h", "help",
         "--- select partition for filesystem (default: none)" );
   printf("%6s%6s%3s\n", "-v", "verbose",
         "--- select partition for filesystem (default: none)" );

   exit(EXIT_FAILURE);
}


void getArgs(int argc, char *argv[])
{
   int opt_index = 0;
   int i = 0;

   while((opt_index = getopt(argc, argv, ":if:lrxh")) != -1)
      switch(opt_index)
         {
            case 'h':
                 help();
                 break;

            case 'v':
            case 'l':
            case 'r':
                printf("option: %c\n", opt_index);
                break;

            case 'f':
                printf("filename: %s\n", optarg);
                break;

            case 'p': /* needed */
                if(-1 == (argInfo.part = atoi(optarg)))
                   usage(EXIT_FAILURE);
                printf("part: %s\n", optarg);
                break;

            case 's': /* needed */
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

   for(i = 0 ; optind < argc; optind++){
      if(i == 0){

      }

      printf("extra arguments: %s\n", argv[optind]);
   }

}


int main(int argc, char *argv[])
{
   printf("Hello World\n");
   getArgs(argc, argv);

   return EXIT_SUCCESS;
}


void newfunction(int i) {


}

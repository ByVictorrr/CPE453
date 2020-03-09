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

int main(int argc, char *argv[]) {

   FILE *output;
   minix_t minix;
   // TODO: ethan parse
   minix.opt.part=0;
   minix.opt.subpart=2;
   minix.opt.imagefile="Images/HardDisk";
   minix.opt.dstpath="test_get/file1";
   minix.opt.srcpath="/home/pnico/Message";

   if(minix.opt.dstpath == NULL){
         output=stdout;
   }else{
         output=safe_fopen(minix.opt.dstpath, "w");
   }

      set_minix_types(&minix);
      write_file(&minix, output);
      fclose(output);
      fclose(minix.image);
      free(minix.inodes);





   return 0;

}

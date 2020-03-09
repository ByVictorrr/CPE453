#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <libgen.h>
#include <string.h>
#include <time.h>

#include "types.h"


// Wrapper function - so we dont have to worry about mess
void safe_fseek(FILE *fp, long int offset, int pos){
    if(fseek(fp, offset,  pos) != 0){
        printf("seek error");
        exit(EXIT_FAILURE);
    }
}
// Wrapper function - so we dont have to worry about mess
void safe_fread(void *ptr, size_t size, size_t nmemb, FILE *stream){
    if(fread(ptr, size,nmemb, stream) < 0){
        printf("read error");
        exit(EXIT_FAILURE);
    }
}
// Wrapper function - so we dont have to worry about mess
void *safe_malloc(size_t size){
    void *ptr;
    if((ptr=malloc(size))==NULL){
        printf("malloc error");
        exit(EXIT_FAILURE);
    }
    return ptr;
}
void * safe_calloc(size_t nitems, size_t size){
    void *ptr;
    if((ptr=calloc(nitems, size))==NULL){
        printf("malloc error");
        exit(EXIT_FAILURE);
    }
    return ptr;
}


/*******************************PARITION FUNCTIONS*******************************************/
/*
* This function checks to see if the partion table is valid
*/
bool_t is_part_table_valid(FILE *image, uint32_t table_addr){
    const int VALID_SIG[2][2]={{510, 0x55},{511, 0xAA}};
    uint8_t sig;
    // step 1 - check at byte 510
    safe_fseek(image, VALID_SIG[0][0]+table_addr, SEEK_SET);
    safe_fread(&sig, sizeof(uint8_t), 1, image);
    if(sig != VALID_SIG[0][1]){
        printf("location %d doesnt have signature of %d", VALID_SIG[0][0], VALID_SIG[0][1]);
        return FALSE;
    }
    // step 2 - check at byte 511
    safe_fseek(image, VALID_SIG[1][0]+table_addr, SEEK_SET);
    safe_fread(&sig, sizeof(uint8_t), 1, image);
    if(sig != VALID_SIG[1][1]){
        printf("location %d doesnt have signature of %d", VALID_SIG[1][0], VALID_SIG[1][1]);
        return FALSE;
    }
    return TRUE;
}

/*
* Given and image turn the data at location addr in partition_t
*/
partition_t get_partition(FILE *image, uint32_t addr){
    partition_t part;
    safe_fseek(image, addr, SEEK_SET);
    safe_fread(&part, sizeof(part),1, image);
    return part;
}
/* Used to get if the table is */
partition_t read_partition(FILE * image, uint32_t part_table, uint32_t im_addr){
    partition_t part;
    // step 1 - make sure its valid
    if(!is_part_table_valid(image, part_table)){
        printf("Not a valid partition table");
        exit(EXIT_FAILURE);
    }
    if((part = get_partition(image, im_addr)).type != MINIX_PART){
        printf("not a minix partition");
        exit(EXIT_FAILURE);
    }
    return part;
}

/*
 Main function for this section, it returns a valid partition or exits (throws msg)
*/
partition_t find_minix_partion(FILE *image, int prim_part, int sub_part){
    partition_t part;
    // step 1 : put @parm2 offset from @parm3 st start of prim part
    part=read_partition(image,
        0,
        ADDR_PARTITION_TABLE +  sizeof(partition_t) * prim_part
    );

    // step 2 : do a similar thing for the sub_partition
    if(sub_part != -1){
        // @parm2 is how far away if it away from its start addr(the partition)
        part=read_partition(image
            , part.lFirst*SECTOR_SIZE
            , (part.lFirst*SECTOR_SIZE) + ADDR_PARTITION_TABLE
                                    + sizeof(partition_t)*sub_part
          );
    }

    return part;
}





/******************************SUPER BLOCK*******************************************/

/* Given an image at @parm2 start reading SB from there */
superblock_t get_SB(FILE *image, const uint32_t first_sector){
    uint32_t LOC = first_sector+BOOT_SIZE;
    superblock_t sb;
    safe_fseek(image, LOC, SEEK_SET);
    safe_fread(&sb, sizeof(superblock_t), 1, image);
    return sb;
}

/***************************O GET INODE STRUCTS*******************************************/
/**
 * @parm image file pointer object
 * @parma first_sector absolute value where first sector is
 * @parma sum_blk_maps - sum of sizes of zone and inode maps
 */
inode_t *get_inodes(FILE *image,const uint32_t first_sector, superblock_t sb){
    uint64_t LOC = first_sector
                    +(2+sb.z_blocks+sb.i_blocks)*sb.blocksize;
    inode_t *inodes = safe_malloc(sizeof(inode_t)*sb.ninodes);
    safe_fseek(image, LOC, SEEK_SET);
    safe_fread(inodes, sizeof(inode_t), sb.ninodes, image);
    return inodes;
}

/**************************TO GET MINIX STRUCT*****************************************/
minix_t get_minix(FILE *image, int prim_part, int sub_part){
    minix_t minix;
    // step 1 - get minix part
    minix.part = find_minix_partion(image, prim_part, sub_part);
    // step 2 - locate the relative address of superblock
    minix.sb=get_SB(image, minix.part.lFirst*SECTOR_SIZE);
    // step 3 - skip over inode bit map and zone bit map to get inodes
    minix.inodes = get_inodes(image, minix.part.lFirst*SECTOR_SIZE, minix.sb);
    return minix;
}

/**************************************************************************/

void print_partition(superblock_t sb, inode_t * inodes)
{

}

/*
File inode:
  unsigned short mode         0x41ff    (drwxrwxrwx)
  unsigned short links             3
  unsigned short uid               2
  unsigned short gid               2
  uint32_t  size            384
  uint32_t  atime    1141098157 --- Mon Feb 27 19:42:37 2006
  uint32_t  mtime    1141098157 --- Mon Feb 27 19:42:37 2006
  uint32_t  ctime    1141098157 --- Mon Feb 27 19:42:37 2006


  Direct zones:
              zone[0]   =         16
              zone[1]   =          0
              zone[2]   =          0
              zone[3]   =          0
              zone[4]   =          0
              zone[5]   =          0
              zone[6]   =          0
   uint32_t  indirect   =          0
   uint32_t  double     =          0
/:
drwxrwxrwx       384 .
drwxrwxrwx       384 ..
-rw-r--r--     73991 Other
drwxr-xr-x      3200 src
-rw-r--r--        11 Hello

*/


/************************PRINT FUNCTIONS ************************************************/
char *get_mode(uint16_t mode)
{
   char* permissions = (char *) malloc(sizeof(char) * 11);
   permissions[0] = GET_PERM(mode, MASK_DIR, 'd');
   permissions[1] = GET_PERM(mode, MASK_O_R, 'r');
   permissions[2] = GET_PERM(mode, MASK_O_W, 'w');
   permissions[3] = GET_PERM(mode, MASK_O_X, 'x');
   permissions[4] = GET_PERM(mode, MASK_G_R, 'r');
   permissions[5] = GET_PERM(mode, MASK_G_W, 'w');
   permissions[6] = GET_PERM(mode, MASK_G_X, 'x');
   permissions[7] = GET_PERM(mode, MASK_OT_R, 'r');
   permissions[8] = GET_PERM(mode, MASK_OT_W, 'w');
   permissions[9] = GET_PERM(mode, MASK_OT_X, 'x');

   permissions[10] = '\0';
   return permissions;
}

void printReadableTime(uint32_t time)
{
    time_t raw_time = (time_t) time;
    struct tm *timeinfo = localtime (&raw_time);
    printf ("%s", asctime(timeinfo));
}

void print_inode(minix_t minix, inode_t inode)
{
   int i;
   /* inode_t *inode = minix.inodes; */
   char * modeText;

   printf("File inode:\n");

   printf("  unsigned short mode %14x    (%s)", inode.mode,
         modeText = get_mode(inode.mode));

   free(modeText);

   printf("  unsigned short links %14d", inode.links);
   printf("  unsigned short uid %14d", inode.uid);
   printf("  unsigned short gid %14d", inode.gid);

   printf("  uint32_t  size %9d", inode.size);

   printf("  uint32_t  atime %9d --- ", inode.atime);
   printReadableTime(inode.atime);

   printf("  uint32_t  mtime %9d --- ", inode.mtime);
   printReadableTime(inode.mtime);

   printf("  uint32_t  ctime %9d --- ", inode.ctime);
   printReadableTime(inode.ctime);

   printf("Direct zones:\n");

   for(i = 0; i < DIRECT_ZONES; i++)
      printf("%18s[%d]   = %10d\n", "zone", i, inode.zone[i]);

   printf("   uint32_t  %11s = %10d", "indirect", inode.indirect);
   printf("   uint32_t  %11s = %10d", "double", inode.two_indirect);
}

/*

Superblock Contents:
Stored Fields:
  ninodes          768
  i_blocks           1
  z_blocks           1
  firstdata         16
  log_zone_size      0 (zone size: 4096)
  max_file  4294967295
  magic         0x4d5a
  zones            360
  blocksize       4096
  subversion         0
Computed Fields:
  version            3
  firstImap          2
  firstZmap          3
  firstIblock        4
  zonesize        4096
  ptrs_per_zone   1024
  ino_per_block     64
  wrongended         0
  fileent_size      64
  max_filename      60
  ent_per_zone      64

*/
void print_superBlock(minix_t minix)
{

   superblock_t sb = minix.sb;

   printf("Superblock Contents:\n");


   printf("Stored Fields:\n");

   printf("  %s%20d\n", "ninodes", sb.ninodes);
   printf("  %s%20d\n", "i_blocks", sb.i_blocks);
   printf("  %s%20d\n", "z_blocks", sb.z_blocks);
   printf("  %s%20d\n", "firstdata", sb.firstdata);
   printf("  %s%20d\n", "log_zone_size", sb.log_zone_size);
   printf("  %s%20d\n", "max_file", sb.max_file);
   printf("  %s%20d\n", "magic", sb.magic);
   printf("  %s%20d\n", "zones", sb.zones);
   printf("  %s%20d\n", "blocksize", sb.blocksize);
   printf("  %s%20d\n", "subversion", sb.subversion);

   printf("Computed Fields:\n");
   /* printf("  %s%20d\n", "version", sb->version);               */
   /* printf("  %s%20d\n", "firstImap", sb->firstImap);           */
   /* printf("  %s%20d\n", "firstZmap", sb->firstZmap);           */
   /* printf("  %s%20d\n", "firstIblock", sb->firstIblock);       */
   /* printf("  %s%20d\n", "zonesize", sb->zonesize);             */
   /* printf("  %s%20d\n", "ptrs_per_zone", sb->ptrs_per_zone);   */
   /* printf("  %s%20d\n", "ino_per_block", sb->ino_per_block);   */
   /* printf("  %s%20d\n", "wrongended", sb->wrongended);         */
   /* printf("  %s%20d\n", "fileent_size", sb->fileent_size);     */
   /* printf("  %s%20d\n", "max_filename", sb->max_filename);     */
   /* printf("  %s%20d\n", "ent_per_zone", sb->ent_per_zone);     */

}

/*
Options:
  opt->part      -1
  opt->subpart   -1
  opt->imagefile Images/TestImage
  opt->srcpath   (null)
  opt->dstpath   (null)


  This is only called if:
      (opt->verbosity > 2)
*/
void print_Options(minix_t minix)
{

   options_t opt = minix.opt;
   printf("Options:\n");

   printf("  %s%20d\n", "otp->part", opt->part);
   printf("  %s%20d\n", "otp->subpart", opt->subpart);
   printf("  %s%20d\n", "otp->imagefile", opt->imagefile);
   printf("  %s%20d\n", "otp->srcpath", opt->srcpath);
   printf("  %s%20d\n", "otp->dstpath", opt->dstpath);


}

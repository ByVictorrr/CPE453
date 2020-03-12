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
        fprintf(stderr, "seek error\n");
        exit(EXIT_FAILURE);
    }
}
// Wrapper function - so we dont have to worry about mess
void safe_fread(void *ptr, size_t size, size_t nmemb, FILE *stream){
    if(fread(ptr, size,nmemb, stream) < 0){
        fprintf(stderr, "read error\n");
        exit(EXIT_FAILURE);
    }
}
// Wrapper function - so we dont have to worry about mess
void *safe_malloc(size_t size){
    void *ptr = NULL;
    if((ptr=malloc(size))==NULL){
        fprintf(stderr, "malloc error\n");
        exit(EXIT_FAILURE);
    }
    return ptr;
}
void * safe_calloc(size_t nitems, size_t size){
    void *ptr = NULL;
    if((ptr=calloc(nitems, size))==NULL){
        fprintf(stderr, "calloc error");
        exit(EXIT_FAILURE);
    }
    return ptr;
}
FILE *safe_fopen(char *path, char *RW){
    FILE *image = NULL;
    if((image=fopen(path,RW))== NULL){
        fprintf(stderr, "No such image exists");
        exit(EXIT_FAILURE);
    }
    return image;
}
size_t safe_fwrite(const void *ptr, size_t size, size_t nitems, FILE *stream){
    size_t read;
    if((read=fwrite(ptr, size, nitems, stream)) < nitems){
        fprintf(stderr, "fwrite failed\n");
        exit(EXIT_FAILURE);
    }
    return read;
}

/******* SETTER FUNCTIONS *************************/
/*******************PARITION FUNCTIONS**************/
bool_t is_part_table_valid(FILE *image, uint32_t table_addr){
    const int VALID_SIG[2][2]={{510, 0x55},{511, 0xAA}};
    uint8_t sig;
    // step 1 - check at byte 510
    safe_fseek(image, VALID_SIG[0][0]+table_addr, SEEK_SET);
    safe_fread(&sig, sizeof(uint8_t), 1, image);
    if(sig != VALID_SIG[0][1]){
        fprintf(stderr, "location %d doesnt have signature of %d", 
                VALID_SIG[0][0], VALID_SIG[0][1]);
        return FALSE;
    }
    // step 2 - check at byte 511
    safe_fseek(image, VALID_SIG[1][0]+table_addr, SEEK_SET);
    safe_fread(&sig, sizeof(uint8_t), 1, image);
    if(sig != VALID_SIG[1][1]){
        fprintf(stderr, "location %d doesnt have signature of %d", 
                VALID_SIG[1][0], VALID_SIG[1][1]);
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
        fprintf(stderr, "Not a valid partition table");
        exit(EXIT_FAILURE);
    }
    if((part = get_partition(image, im_addr)).type != MINIX_PART){
        fprintf(stderr, "not a minix partition");
        exit(EXIT_FAILURE);
    }
    return part;
}

void set_partition(minix_t *minix){
    // step 1 : put @parm2 offset from @parm3 st start of prim part
    if(minix->opt.part != UNPARITIONED){
        minix->part=read_partition(minix->image,
            0,
            ADDR_PARTITION_TABLE +  sizeof(partition_t) * minix->opt.part
        );
    }
    // step 2 : do a similar thing for the sub_partition
    if(minix->opt.subpart != UNPARITIONED){
        // @parm2 is how far away if it away from its start addr(the partition)
        minix->part=read_partition(minix->image
            , minix->part.lFirst*SECTOR_SIZE
            , (minix->part.lFirst*SECTOR_SIZE) + ADDR_PARTITION_TABLE
                                    + sizeof(partition_t)*minix->opt.subpart
          );
    }
}

/*************SUPER BLOCK*********************/

/* Given an image at @parm2 start reading SB from there */
 void set_SB(minix_t *minix){
    uint32_t LOC;
     // case no -p and -s
     if(minix->opt.part == UNPARITIONED){
         LOC=BOOT_SIZE;
     }else{
        LOC = minix->part.lFirst*SECTOR_SIZE+BOOT_SIZE;
     }
    safe_fseek(minix->image, LOC, SEEK_SET);
    safe_fread(&minix->sb, sizeof(superblock_t), 1, minix->image);
    if(minix->sb.magic != MINIX_MAGIC && minix->sb.magic != REV_MINIX_MAGIC){
        fprintf(stderr, "Bad magic number. (0x0000)\n");
        fprintf(stderr, "This doesn't look like a MINIX filesystem.\n");
        exit(EXIT_FAILURE);
    }
}

/***************************INODE*******************************************/
/**
 * @parm image file pointer object
 * @parma first_sector absolute value where first sector is
 * @parma sum_blk_maps - sum of sizes of zone and inode maps
 */
void set_inodes(minix_t *minix){
     uint64_t LOC;
     // case no -p and -s
    if(minix->opt.part == UNPARITIONED){
         LOC=(2+minix->sb.z_blocks+minix->sb.i_blocks)*minix->sb.blocksize;
     }else{
          LOC = minix->part.lFirst*SECTOR_SIZE
              +(2+minix->sb.z_blocks+minix->sb.i_blocks)*minix->sb.blocksize;
     }
     // add one because inode=0 is nothing
    minix->inodes = safe_malloc(sizeof(inode_t)*(minix->sb.ninodes+1));
    safe_fseek(minix->image, LOC, SEEK_SET);
    safe_fread(minix->inodes+1, sizeof(inode_t), 
                minix->sb.ninodes, minix->image);

}
/*************BIT MAPS************************************/

void set_zmap(minix_t *minix){
    minix->z_map = safe_calloc(1, minix->sb.blocksize);
    safe_fseek(minix->image, 3*minix->sb.blocksize, SEEK_SET);
    safe_fread(minix->z_map, minix->sb.blocksize, 1, minix->image);

}
void set_imap(minix_t *minix){
    minix->i_map = safe_calloc(1, minix->sb.blocksize);
    safe_fseek(minix->image, 2*minix->sb.blocksize, SEEK_SET);
    safe_fread(minix->i_map, minix->sb.blocksize, 1, minix->image);
}

/***************TO GET MINIX STRUCT******************/
void set_minix_types(minix_t *minix){
    minix->image=safe_fopen(minix->opt.imagefile, "r");
    set_partition(minix);
    set_SB(minix);
    set_imap(minix);
    set_zmap(minix);
    set_inodes(minix);
}

/*******************END OF SETTER FUNCTION******/



/************PRINT FUNCTIONS *********************/
char *get_mode(uint16_t mode)
{
   char* perms = safe_calloc(11, sizeof(char));
   perms[0] = GET_PERM(mode, MASK_DIR, 'd');
   perms[1] = GET_PERM(mode, MASK_O_R, 'r');
   perms[2] = GET_PERM(mode, MASK_O_W, 'w');
   perms[3] = GET_PERM(mode, MASK_O_X, 'x');
   perms[4] = GET_PERM(mode, MASK_G_R, 'r');
   perms[5] = GET_PERM(mode, MASK_G_W, 'w');
   perms[6] = GET_PERM(mode, MASK_G_X, 'x');
   perms[7] = GET_PERM(mode, MASK_OT_R, 'r');
   perms[8] = GET_PERM(mode, MASK_OT_W, 'w');
   perms[9] = GET_PERM(mode, MASK_OT_X, 'x');
   return perms;
}

void printReadableTime(uint32_t time)
{
    time_t raw_time = (time_t) time;
    struct tm *timeinfo = localtime (&raw_time);
    fprintf(stderr, "%s", asctime(timeinfo));
}

void print_inode_metadata(minix_t *minix, inode_t inode)
{
   int i;
   /* inode_t *inode = minix.inodes; */
   char * modeText;

   fprintf(stderr, "File inode:\n");

   fprintf(stderr, "  unsigned short mode %14x    (%s)", inode.mode,
         modeText = get_mode(inode.mode));

   free(modeText);

   fprintf(stderr, "  unsigned short links %14d", inode.links);
   fprintf(stderr, "  unsigned short uid %14d", inode.uid);
   fprintf(stderr, "  unsigned short gid %14d", inode.gid);

   fprintf(stderr, "  uint32_t  size %9d", inode.size);

   fprintf(stderr, "  uint32_t  atime %9d --- ", inode.atime);
   printReadableTime(inode.atime);

   fprintf(stderr, "  uint32_t  mtime %9d --- ", inode.mtime);
   printReadableTime(inode.mtime);

   fprintf(stderr, "  uint32_t  ctime %9d --- ", inode.ctime);
   printReadableTime(inode.ctime);

   fprintf(stderr, "Direct zones:\n");

   for(i = 0; i < DIRECT_ZONES; i++)
      fprintf(stderr, "%18s[%d]   = %10d\n", "zone", i, inode.zone[i]);

   fprintf(stderr, "   uint32_t  %11s = %10d", "indirect", inode.indirect);
   fprintf(stderr, "   uint32_t  %11s = %10d", "double", inode.two_indirect);
}

void print_superBlock(minix_t *minix)
{

   superblock_t sb = minix->sb;

   fprintf(stderr, "Superblock Contents:\n");

   fprintf(stderr, "Stored Fields:\n");

   fprintf(stderr, "  %-20s%-d\n", "ninodes", sb.ninodes);
   fprintf(stderr, "  %-20s%-d\n", "i_blocks", sb.i_blocks);
   fprintf(stderr, "  %-20s%-d\n", "z_blocks", sb.z_blocks);
   fprintf(stderr, "  %-20s%-d\n", "firstdata", sb.firstdata);
   /*ADD (zone size: %d)*/
   fprintf(stderr, "  %-20s%-d\n", "log_zone_size", sb.log_zone_size);
   fprintf(stderr, "  %-20s%-d\n", "max_file", sb.max_file);
   fprintf(stderr, "  %-20s%-d\n", "magic", sb.magic);
   fprintf(stderr, "  %-20s%-d\n", "zones", sb.zones);
   fprintf(stderr, "  %-20s%-d\n", "blocksize", sb.blocksize);
   fprintf(stderr, "  %-20s%-d\n", "subversion", sb.subversion);

   fprintf(stderr, "Computed Fields:\n");
   /* fprintf(stderr, "  %s%20d\n", "version", sb->version);               */
   /* fprintf(stderr, "  %s%20d\n", "firstImap", sb->firstImap);           */
   /* fprintf(stderr, "  %s%20d\n", "firstZmap", sb->firstZmap);           */
   /* fprintf(stderr, "  %s%20d\n", "firstIblock", sb->firstIblock);       */
   /* fprintf(stderr, "  %s%20d\n", "zonesize", sb->zonesize);             */
   /* fprintf(stderr, "  %s%20d\n", "ptrs_per_zone", sb->ptrs_per_zone);   */
   /* fprintf(stderr, "  %s%20d\n", "ino_per_block", sb->ino_per_block);   */
   /* fprintf(stderr, "  %s%20d\n", "wrongended", sb->wrongended);         */
   /* fprintf(stderr, "  %s%20d\n", "fileent_size", sb->fileent_size);     */
   /* fprintf(stderr, "  %s%20d\n", "max_filename", sb->max_filename);     */
   /* fprintf(stderr, "  %s%20d\n", "ent_per_zone", sb->ent_per_zone);     */

}

/*
 * This is only called if:
 *     (opt->verbosity > 2)
 */
void print_options(minix_t *minix)
{
   options_t *opt = &(minix->opt);
   fprintf(stderr, "Options:\n");

   fprintf(stderr, "  %-15s%-d\n", "otp->part", opt->part);
   fprintf(stderr, "  %-15s%-d\n", "otp->subpart", opt->subpart);
   fprintf(stderr, "  %-15s%-s\n", "otp->imagefile", opt->imagefile);
   fprintf(stderr, "  %-15s%-s\n", "otp->srcpath", opt->srcpath);
   fprintf(stderr, "  %-15s%-s\n", "otp->dstpath", opt->dstpath);
}


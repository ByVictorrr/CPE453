#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <libgen.h>
#include <string.h>

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

/*****************************EO PARTITION*************************************************/




/******************************SUPER BLOCK*******************************************/

/* Given an image at @parm2 start reading SB from there */
superblock_t get_SB(FILE *image, const uint32_t first_sector){
    uint32_t LOC = first_sector+BOOT_SIZE;
    superblock_t sb;
    safe_fseek(image, LOC, SEEK_SET);
    safe_fread(&sb, sizeof(superblock_t), 1, image);
    return sb;
}

/******************************INODE BLOCK*******************************************/
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
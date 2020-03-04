
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <libgen.h>
#include <string.h>


#include "super.h"
#include "partition.h"
#include "dirent.h"
#include "inode.h"

#define LENGTH 1000
// In bytes
#define SECTOR_SIZE 512
#define BLOCK_SIZE SECTOR * 2
// Location of partition table
#define ADDR_PARTITION_TABLE  0x1BE
// indicates a valid minix partition
#define MINIX_PART 0x81
// minix 3 magic number (little endian)
#define MINIX_MAGIC 0x4D5A
#define BOOT_SIZE 1024
typedef enum BOOL{FALSE,TRUE}bool_t;

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

/*******************************PARITION FUNCTIONS*******************************************/
/* 
* This function checks to see if the partion table is valid
* @parm image - file pointer to image
* @return - if the part table is valid true else false 
*/
bool_t is_part_table_valid(FILE *image, uint32_t offset){
    const int VALID_SIG[2][2]={{510, 0x55},{511, 0xAA}};
    uint8_t sig;
    // step 1 - check at byte 510
    safe_fseek(image, VALID_SIG[0][0]+offset, SEEK_SET);
    safe_fread(&sig, sizeof(uint8_t), 1, image);
    if(sig != VALID_SIG[0][1]){
        printf("location %d doesnt have signature of %d", VALID_SIG[0][0], VALID_SIG[0][1]);
        return FALSE;
    }    
    // step 2 - check at byte 511
    safe_fseek(image, VALID_SIG[1][0]+offset, SEEK_SET);
    safe_fread(&sig, sizeof(uint8_t), 1, image);
    if(sig != VALID_SIG[1][1]){
        printf("location %d doesnt have signature of %d", VALID_SIG[1][0], VALID_SIG[1][1]);
        return FALSE;
    } 
    return TRUE;
}

/* 
* Checks to see if the partion is bootable
* @parm - part_num 
*/
partition_t get_partition(FILE *image, const int part_num){
    partition_t part;
    safe_fseek(image, ADDR_PARTITION_TABLE+part_num*SECTOR_SIZE, SEEK_SET);
    safe_fread(&part, sizeof(part),1, image);

    return part;
}

/* 
* This function return a valid minix partition object
* TODO: make it recursive for sub-partitions
* @return minix partition or null if table not valid
*/
partition_t find_minix_partion(FILE *image){
    partition_t part;
    int part_num=0;
    int last_sector;
    // step 0 - check to see if valid parition table
    if(!is_part_table_valid(image)){
        printf("Not a valid partition table");
        return (partition_t){0};
    }
    // step 1 - go through every partition until find a minix partition
    while(part.type != MINIX_PART){
        part = get_partition(image, part_num);
        // next partion
        last_sector = part.lFirst+part.size-1; // gives last sector of part number
        part_num = last_sector+1; // gives begging of next partition(in sectors)
    }
    // step 2 - once got a minix valid partition return it
    return part;
}

/**********************************************************************************/
/******************************SUPER BLOCK*******************************************/


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
    uint32_t LOC = first_sector+2*SECTOR_SIZE+(sb.z_blocks+sb.i_blocks);
    inode_t *inodes = safe_malloc(sizeof(inode_t)*sb.ninodes);
    safe_fseek(image, LOC, SEEK_SET);
    safe_fread(inodes, sizeof(inode_t), sb.ninodes, image);
    return inodes; 
}


/**********************************************************************************/



/**
 * recursive function that goes through directies until it finds the src_path
 * returns the inode corresponding to the file_path
 */
/*
inode_t find_file(inode_t *inodes, int inode_num, const char *file_path, FILE * image){
    
    dirent_t dir;
    // base case stop when we find the base name 
    if(strcmp(file_path, basename(file_path)) == 0){
        return inodes[inode_num];
    }
    // general case
    //safe_fseek(image, inodes[inode_num], )
}
*/
void read_file(FILE *image, const char * src_path){
    partition_t minix_part;
    superblock_t sb;
    inode_t *inodes; // need to malloc doesnt know how many inodes
    uint32_t addr_first_sector;
    // step 1 - get minix part
    minix_part = find_minix_partion(image);
    // absolute first sector of given partition in bytes
    addr_first_sector=minix_part.lFirst*SECTOR_SIZE;
    // step 2 - locate the relative address of superblock
    sb=get_SB(image, addr_first_sector);
    // step 3 - skip over inode bit map and zone bit map to get inodes 
    inodes = get_inodes(image, addr_first_sector, sb);
    // step 4 - determine if the src_path is a file or directory
    // data_block_addr = find_file(inodes, 0, src_path, image);
    


}
/**********************************************************************************/

int main(){
    FILE * image;
    const char * IMAGE = "Images/HardDisk";
    const char * SRC_FILE = "/minix";
    if((image=fopen(IMAGE,"r"))== NULL){
        printf("No such image exists");
        exit(EXIT_FAILURE);
    }
    read_file(image, SRC_FILE);

    return 0;
}




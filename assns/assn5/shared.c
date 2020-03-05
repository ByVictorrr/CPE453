
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
* @parm start_par - absolute address of start of current partition
* @return - if the part table is valid true else false 
*/
bool_t is_part_table_valid(FILE *image, uint32_t start_addr){
    const int VALID_SIG[2][2]={{510, 0x55},{511, 0xAA}};
    uint8_t sig;
    // step 1 - check at byte 510
    safe_fseek(image, VALID_SIG[0][0]+start_addr, SEEK_SET);
    safe_fread(&sig, sizeof(uint8_t), 1, image);
    if(sig != VALID_SIG[0][1]){
        printf("location %d doesnt have signature of %d", VALID_SIG[0][0], VALID_SIG[0][1]);
        return FALSE;
    }    
    // step 2 - check at byte 511
    safe_fseek(image, VALID_SIG[1][0]+start_addr, SEEK_SET);
    safe_fread(&sig, sizeof(uint8_t), 1, image);
    if(sig != VALID_SIG[1][1]){
        printf("location %d doesnt have signature of %d", VALID_SIG[1][0], VALID_SIG[1][1]);
        return FALSE;
    } 
    return TRUE;
}

/* 
* Checks to see if the partion is bootable
* @parm - addr absolute address to get parition
*/
partition_t get_partition(FILE *image, uint32_t addr){
    partition_t part;
    safe_fseek(image, addr, SEEK_SET);
    safe_fread(&part, sizeof(part),1, image);
    return part;
}

partition_t read_partition(FILE * image, uint32_t abs_addr, uint32_t abs_start_addr){
    partition_t part;
    // step 1 - make sure its valid
    if(!is_part_table_valid(image, abs_start_addr)){
        printf("Not a valid partition table");
        exit(EXIT_FAILURE);
    }
    if((part = get_partition(image, abs_addr)).type != MINIX_PART){
        printf("not a minix partition");
        exit(EXIT_FAILURE);
    }
    return part;
}

/* 
* This function return a valid minix partition object
* @param prim_part - what primary partition to read
* @param sub_part - what subpartion with a prim_part to read (-1 means no subpartion)
* @return minix partition or null if table not valid

*/
partition_t find_minix_partion(FILE *image, int prim_part, int sub_part){
    partition_t part;
    const uint32_t prim_part_addr = ADDR_PARTITION_TABLE + 
                            sizeof(partition_t) * prim_part;
    uint32_t sub_part_addr;
    // step 1 - get primary parition
    part=read_partition(image, prim_part_addr, 0);
    // step 2 - get sub partition is asked for
    if(sub_part != -1){
        sub_part_addr = (part.lFirst*SECTOR_SIZE) // this quanity is starting of partition table
                + ADDR_PARTITION_TABLE + sizeof(partition_t)*sub_part;
        part=read_partition(image, sub_part_addr, part.lFirst*SECTOR_SIZE);
    }

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
    uint32_t LOC = first_sector+BOOT_SIZE+(sb.z_blocks+sb.i_blocks)*sb.blocksize;
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
inode_t find_file(inode_t *inodes, int inode_num, const char *file_path, FILE * image){
    
    dirent_t dir;
    // base case stop when we find the base name 
    if(strcmp(file_path, basename(file_path)) == 0){
        return inodes[inode_num];
    }
    // general case
    //safe_fseek(image, inodes[inode_num], )
}

void read_file(FILE *image, const char * src_path, int prim_part, int sub_part){
    partition_t minix_part;
    superblock_t sb;
    inode_t *inodes; // need to malloc doesnt know how many inodes
    uint32_t addr_first_sector;
    // step 1 - get minix part
    minix_part = find_minix_partion(image, prim_part, sub_part);
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
    const int prim_part = 0;
    const int sub_part = 0;
    if((image=fopen(IMAGE,"r"))== NULL){
        printf("No such image exists");
        exit(EXIT_FAILURE);
    }
    read_file(image, SRC_FILE, prim_part, sub_part);

    return 0;
}




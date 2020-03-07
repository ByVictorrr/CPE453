
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <libgen.h>
#include <string.h>


#include "types.h"



/************************USING ALL ABOVE ************************************************/

void print_data(unsigned char * arr, int size){
    int i;
    for (i=0; i < size; i++){
        //printf("%c", (char)arr[i]);
    }
}
uint8_t * read_data( const minix_t minix,
                     const inode_t from,
                     FILE * image
                     ){
    // how many bytes a zone takes up
    int ZONE_SIZE = minix.sb.blocksize << minix.sb.log_zone_size;     
    uint8_t i, data_index, b_left;
    uint8_t *data = safe_malloc(sizeof(uint8_t)*from.size);
    // go through direct each zone 
    b_left = from.size;
    for (i=0; i< DIRECT_ZONES && b_left; i++){
        // index where we are in reading data
        data_index = from.size - b_left;
        safe_fseek(image,
            minix.part.lFirst*SECTOR_SIZE +
            from.zone[i]*ZONE_SIZE,
            SEEK_SET
        );
        safe_fread(data + data_index, sizeof(uint8_t), ZONE_SIZE, image);
        b_left = b_left-ZONE_SIZE;
    }
    print_data(data, data_index+1);

}


/**
 * recursive function that goes through directies until it finds the src_path
 * returns the inode corresponding to the file_path
 */
inode_t Find_File( const minix_t minix,
                    int inode_num, 
                    const char *file_path,
                    FILE * image
                    ){
    
    #define MAX 60
    inode_t curr;
    uint8_t *data;
    // base case stop when we find the base name 
    if(strcmp(file_path, basename(file_path)) == 0){
        return minix.inodes[inode_num];
    }else{
        // step 1 - from inode
        curr = minix.inodes[inode_num];
        // step 2 - get the directory from that inode
        data = read_data(minix, curr, image);
        // step 3 - memset it into the dir entry

        
    }
    //safe_fseek(image, inodes[inode_num], )
}


// wrapper function of Find file_file
inode_t find_file(minix_t minix, FILE *image, const char *file_path){
    inode_t found = Find_File(minix, 0, file_path, image);
    return found;
}


void read_file(FILE *image, const char * src_path, int prim_part, int sub_part){
    uint32_t start_addr; // in bytes
    inode_t dest; // destination data files
    minix_t minix;
    // step 1 - get minix part
    minix.part = find_minix_partion(image, prim_part, sub_part);
    // absolute first sector of given partition in bytes
    start_addr=minix.part.lFirst*SECTOR_SIZE;
    // step 2 - locate the relative address of superblock
    minix.sb=get_SB(image, start_addr);
    // step 3 - skip over inode bit map and zone bit map to get inodes 
    minix.inodes = get_inodes(image, start_addr, minix.sb);
    // step 5 - read file of src_path
    dest = find_file(minix, image, src_path);


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




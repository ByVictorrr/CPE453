
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <libgen.h>
#include <string.h>


#include "types.h"
#include "path.h"
#define MIN(a,b) ((a) < (b) ? (a):(b))



/************************USING ALL ABOVE ************************************************/


// can pass a dir / char type 
dirent_t * read_dir( const minix_t minix,
                     const inode_t from,
                     FILE * image
                     ){
    // how many bytes a zone takes up
    int ZONE_SIZE = minix.sb.blocksize << minix.sb.log_zone_size;     
    uint8_t i, data_index, b_left;
    int type_size;
    dirent_t *data = safe_calloc(from.size, sizeof(uint8_t));
    // go through direct each zone 

    b_left = from.size;
    for (i=0; i< DIRECT_ZONES && b_left; i++){
        // to determine what size to read
        int read_size = MIN(b_left, ZONE_SIZE);
        // index where we are in reading data
        data_index = from.size - b_left;
        if(from.zone[i] == 0){
            continue;
        }
        safe_fseek(image,
            minix.part.lFirst*SECTOR_SIZE + // start of part
            from.zone[i]*ZONE_SIZE, // zone
            SEEK_SET
        );
        safe_fread(data + data_index, sizeof(dirent_t), read_size, image);
        b_left = b_left-read_size;
    }
    return data;
}

dirent_t *get_dir_entry(dirent_t *entrys, int size, char *name){
    int i;
    for(i=0; i< size; i++){
        if(strcmp(entrys[i].name, name)==0)
            return &entrys[i];
    }
    return NULL;
}

/**
 * recursive function that goes through directies until it finds the src_path
 * returns the inode corresponding to the file_path
 */
inode_t Find_File( const minix_t minix,
                    int inode_num, 
                    char *file_path,
                    FILE * image
                    ){
    
    inode_t curr;
    dirent_t *entrys;
    dirent_t *next_dir;
    // base case stop when we find the base name 
    if(strcmp(file_path, basename(file_path)) == 0){
        return minix.inodes[inode_num];
    }else{
        // step 1 - from inode
        curr = minix.inodes[inode_num];
        // step 2 - get the directory from that inode
        entrys = read_dir(minix, curr, image);
        // step 3 - find that next dir in data
        char next_path[1000] = {0}, *next_folder_base;
        // step 4 - get the next directories base name
        basename_path(file_path, next_path);
        // step 5 - get the inode for that next entry 
        next_folder_base=dirname(next_path);
        if(strcmp(next_folder_base,".")==0){
            next_folder_base = next_path;
        }
        // step 6 - get exact wanted entry
        if((next_dir = get_dir_entry(entrys, curr.size, next_folder_base)) == NULL){
            printf("no such entry found");
            exit(EXIT_FAILURE);
        }
        // step 4 - recurse
        // free the entrys
        free(entrys);
        return Find_File(minix, next_dir->inode, next_path, image);
    }
}


// wrapper function of Find file_file
inode_t find_file(minix_t minix, FILE *image, char *file_path){
    inode_t found = Find_File(minix, 0, file_path, image);
    return found;
}


void read_file(FILE *image, char * src_path, int prim_part, int sub_part){
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
    // step 6 - print out contents of that inode

}
/**********************************************************************************/


int main(){
    FILE * image;
    const char * IMAGE = "Images/HardDisk";
    char *SRC_FILE = "/bin";
    const int prim_part = 0;
    const int sub_part = 0;
    if((image=fopen(IMAGE,"r"))== NULL){
        printf("No such image exists");
        exit(EXIT_FAILURE);
    }
    
    read_file(image, SRC_FILE, prim_part, sub_part);

    return 0;
}




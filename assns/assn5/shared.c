
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <libgen.h>
#include <string.h>
#include <math.h>


#include "types.h"
#include "path.h"
#define MIN(a,b) ((a) < (b) ? (a):(b))
typedef enum{REGULAR, DIRECTORY} file_t;


/************************PRINT FUNCTION ************************************************/

/*
* A general function that allows us to read from an inode
*/
uint8_t *read_inode(
                  const minix_t *minix,
                  const inode_t inode,
                  file_t type
                     ){
                    
    int ZONE_SIZE = minix->sb.blocksize << minix->sb.log_zone_size;
    uint8_t i, data_index, b_left;
    uint8_t *data;
    // to keep track how many block we need to read
    int type_size;
    // step 1 - determine if inode is dirent or reg file
    if(type==DIRECTORY){
       type_size = sizeof(dirent_t);
    }else{
        type_size = sizeof(uint8_t);
    }
    data=safe_calloc(inode.size, type_size);
    b_left = inode.size;
    for (i=0; i< DIRECT_ZONES && b_left; i++){
        // to determine what size to read
        int read_size = MIN(b_left, ZONE_SIZE);
        // index where we are in reading data
        data_index = inode.size - b_left;
        if(inode.zone[i] == 0){
            continue;
        }
        safe_fseek(minix->image,
            minix->part.lFirst*SECTOR_SIZE + // start of part
            inode.zone[i]*ZONE_SIZE, // zone in bytes
            SEEK_SET
        );
        safe_fread(data + data_index, type_size, read_size, minix->image);
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
inode_t Find_File( const minix_t *minix,
                    int inode_num,
                    char *file_path
                    ){

    inode_t curr;
    dirent_t *entrys;
    dirent_t *next_dir;
    char next_path[1000] = {0}, *next_folder_base;

    // base case stop when we find the base name
    if(strcmp(file_path, basename(file_path)) == 0){
        return minix->inodes[inode_num];
    }else{
        // step 1 - from inode
        curr = minix->inodes[inode_num];
        // step 2 - get the directory from that inode
        entrys = read_inode(minix, curr, DIRECTORY);
        // step 3 - find that next dir in data
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
        return Find_File(minix, next_dir->inode, next_path);
    }
}



// wrapper function of Find file_file
inode_t find_file(minix_t *minix){
    inode_t found = Find_File(minix, 1, minix->opt.srcpath);
    return found;
}


/**********************************************************************************/

/**
 * This function is used for ls minix
 */
void read_file(minix_t *minix){
    uint32_t start_addr; // in bytes
    inode_t dest; // destination data files
    // Step 2 - get inode for src_path
    dest = find_file(minix);
    // step 3 - print out contents of
    print_inode(minix, dest);
}

/*
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

    read_file();

    return 0;
}

*/



#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <libgen.h>
#include <string.h>


#include "types.h"
#include "path.h"
#define MIN(a,b) ((a) < (b) ? (a):(b))

#define MASK_DIR  0040000
#define MASK_O_R  0000400
#define MASK_O_W  0000200
#define MASK_O_X  0000100
#define MASK_G_R  0000040
#define MASK_G_W  0000020
#define MASK_G_X  0000010
#define MASK_OT_R 0000004
#define MASK_OT_W 0000002
#define MASK_OT_X 0000001
#define GET_PERM(mode, mask, c) ( (((mode)&(mask)) == mask) ? c : '-' )

/************************PRINT FUNCTION ************************************************/
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

void print_dir(dirent_t *dir, minix_t minix, int size){
    int i, inode_num;

    for(i=0; i< size; i++){
        minix.inodes[dir[i].inode];
    }
    return NULL;

}


/************************PRINT FUNCTION ************************************************/


typedef enum{REGULAR, DIRECTORY} file_t;
// returns the data blocks associated with that inode (from)
void * read_inode(const minix_t minix,
                  const inode_t from,
                  file_t type,
                  FILE * image
                     ){
    int ZONE_SIZE = minix.sb.blocksize << minix.sb.log_zone_size;
    uint8_t i, data_index, b_left;
    void *data = safe_calloc(from.size, sizeof(uint8_t));
    int type_size;

    // step 1 - determine if inode is dirent or reg file
    if(type==DIRECTORY){
       type_size = sizeof(dirent_t);
    }else{
        type_size = sizeof(uint8_t);
    }

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
            from.zone[i]*ZONE_SIZE, // zone in bytes
            SEEK_SET
        );
        safe_fread(data + data_index, type_size, read_size, image);
        b_left = b_left-read_size;
    }

    // TODO : the other zones
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
        entrys = read_inode(minix, curr, DIRECTORY, image);
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


/**
 * This function is used for ls minix
 */
void read_file(FILE *image, char * src_path, int prim_part, int sub_part){
    uint32_t start_addr; // in bytes
    inode_t dest; // destination data files
    uint8_t *data;
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
    // step 6 - print out contents of that inode (this is for reading for the get)
    print_inode();
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




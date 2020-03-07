
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <libgen.h>
#include <string.h>
#include <math.h>


#include "types.h"
#include "path.h"
#define MIN(a,b) ((a) < (b) ? (a):(b))

void print_dir(dirent_t *dir, minix_t minix, int size){
    int i, inode_num;

    for(i=0; i< size; i++){
        minix.inodes[dir[i].inode];
    }
    return NULL;

}


/************************PRINT FUNCTION ************************************************/

typedef enum{REGULAR, DIRECTORY} file_t;
/*
* A general function that where we can read data from zones given by @parm2
* @parm1 minix structure to give us info we need
* @parm2 zones[DIRECT_ZONES] giving us the zones to read from
* @parm3 type of file we are reading from
* @parm4 image were we read from
* @parm5 were we are putting all the data
* 
* @return the amount of size left to read in bytes
*/
int read_direct_inode(
                  const minix_t minix,
                  uint32_t zones[DIRECT_ZONES],
                  int size,
                  file_t type,
                  FILE * image,
                  uint8_t *data
                     ){

    int ZONE_SIZE = minix.sb.blocksize << minix.sb.log_zone_size;     
    uint8_t i, data_index, b_left;
    // to keep track how many block we need to read
    int type_size;
    // step 1 - for the function to return and not read all the direct zones
    if(size <= 0){
        return -1;
    }
    // step 2 - determine if inode is dirent or reg file
    if(type==DIRECTORY){
       type_size = sizeof(dirent_t);
    }else{
        type_size = sizeof(uint8_t);
    }
    b_left = size;
    for (i=0; i< DIRECT_ZONES && b_left; i++){
        // to determine what size to read
        int read_size = MIN(b_left, ZONE_SIZE);
        // index where we are in reading data
        data_index = size - b_left;
        if(zones[i] == 0){
            continue;
        }
        safe_fseek(image,
            minix.part.lFirst*SECTOR_SIZE + // start of part
            zones[i]*ZONE_SIZE, // zone in bytes
            SEEK_SET
        );
        safe_fread(data + data_index, type_size, read_size, image);
        b_left = b_left-read_size;
    }
    return b_left;
}
uint32_t *read_indirect_zones(minix_t minix, inode_t from, FILE * image){
    // step 1 - read from the 
    static uint32_t dir_zones[DIRECT_ZONES];
    int BLOCKS_PER_ZONE = pow(2.0, minix.sb.log_zone_size);
    safe_fseek(image, 
        minix.part.lFirst*SECTOR_SIZE+
        from.indirect
        ,SEEK_SET
    );
    memset(dir_zones, 0, DIRECT_ZONES);
    // read into dir_zones the data of each direct zone at indirect block
    safe_fread(dir_zones, minix.sb.blocksize, BLOCKS_PER_ZONE, image);
    return dir_zones;
}
/**
 * 
 */
uint32_t *read_double_indirect_zones(minix_t minix, inode_t from , FILE *image){
    uint32_t two_indirect = from.two_indirect;
    uint32_t indirect_zones[DIRECT_ZONES];
    static uint32_t zones[DIRECT_ZONES*DIRECT_ZONES];
    int ZONE_SIZE = minix.sb.blocksize << minix.sb.log_zone_size;     
    int BLOCKS_PER_ZONE = pow(2.0, minix.sb.log_zone_size);
    int i;
    safe_fseek(image, 
        minix.part.lFirst*SECTOR_SIZE+
        from.two_indirect
        ,SEEK_SET
    );
    // step 1 - read all direct zones from the double indirect zone
    safe_fread(indirect_zones, minix.sb.blocksize, BLOCKS_PER_ZONE, image);
    // step 2 - go through every indirect zone and get the direct one
    for ( i = 0; i < DIRECT_ZONES; i++){
        // for each indirect zone read the data from it
       read_indirect_zones(minix, from, image);
    }
    return zones;
}

// Reads data from direct zones, indirect zones and then double indirect zones
void *read_inode(const minix_t minix, inode_t from, file_t type, FILE *image){
    void *data = safe_calloc(from.size, sizeof(uint8_t));
    unsigned int bytes_left;
    uint32_t zones[DIRECT_ZONES];
    // step 1 - read direct zones
    bytes_left = read_direct_inode(minix,from.zone, from.size, type, image, data);
    /*
    // step 2 - read indirect zones and then from that read those
    memcpy(zones, read_indirect_zones(minix, from, image), 2);
    bytes_left = read_direct_inode(minix, zones , bytes_left, type, image, data);
    // step 3 - read double direct zones then read each indirect zones 
    memcpy(zones, read_indirect_zones(minix, from, image), 2);
    bytes_left = read_direct_inode(minix, "fill in with zones of double indirect",bytes_left, type, image, data);
    */
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





/**********************************************************************************/

/**
 * This function is used for ls minix 
 */
void read_file(FILE *image, char * src_path, int prim_part, int sub_part){
    uint32_t start_addr; // in bytes
    inode_t dest; // destination data files
    minix_t minix;
    // Step 1 - get sb, partition, inodes store in minix strucutre
    minix=get_minix(image,prim_part, sub_part);
    // Step 2 - get inode for src_path
    dest = find_file(minix, image, src_path);
    // step 3 - print out contents of 
    print_inode(minix, dest);
}

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




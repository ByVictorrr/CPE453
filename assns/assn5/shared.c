
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <libgen.h>
#include <string.h>
#include <math.h>

#include "types.h"
#include "path.h"
#include "shared.h"

#define NOT_FOUND -1
#define DELETED_INODE 0
#define ROOT_INODE 1



//================================min get===========================================
void write_file(minix_t *minix, FILE *dest){
    int src_inode;
    uint8_t *src_data;
    char *mode;
    // step 1 - get inode number of dest
    if((src_inode = get_inode_num(minix, minix->opt.srcpath)) == NOT_FOUND){
        printf("destination not found\n");
        exit(EXIT_FAILURE);
    }
    // step 2.1 - check to see its not symlink or directory
    if(minix->inodes[src_inode].mode & MASK_REG != MASK_REG){
        // this should be for sym links and dirs
        printf("Cant get symlinks or directories");
        exit(EXIT_FAILURE);
    }
    // step 2.2 - get contents of src_inode (assumption here is reg file)



    // step 3 - update minix->inodes[dest_inode] folder with contents
    safe_fwrite(src_data, sizeof(uint8_t), minix->inodes[src_inode].size, dest);
}



// for minget
uint8_t *get_data(minix_t *minix, int inode_num){


}


//================================eo min get===========================================


/*
* A general function that allows us to read from an inode
* returns the index of where entrys is
*/
int set_entrys(const minix_t *minix, uint32_t *zones, int num_zones, int size ,int index, dirent_t * entrys){
                    
    int ZONE_SIZE = minix->sb.blocksize << minix->sb.log_zone_size;
    int i, j; 
    uint32_t b_left = size;
    // go through every direct zones
    for (i=0; i< num_zones && b_left; i++){
        // to determine what size to read
        int read_size = MIN(b_left, ZONE_SIZE), r_size=read_size;
        // continue if zone is 0
        if(zones[i] == 0){
            continue;
        }
        // set the cursor for that zone
        safe_fseek(minix->image,
                minix->part.lFirst*SECTOR_SIZE + // start of part
                zones[i]*ZONE_SIZE, // zone in bytes
                SEEK_SET
        );
        // read each entry in that zone
        while(read_size > 0){
            safe_fread(entrys+index, sizeof(dirent_t), 1, minix->image);
            read_size -= sizeof(dirent_t);
            index++;
        }
        b_left = b_left-r_size;
    }
    return index;
}
uint32_t *read_indirect_zones(const minix_t *minix, uint32_t indirect, int *num_zones){
    // step 1 - go though the one zone and read each
    int ZONE_SIZE = minix->sb.blocksize << minix->sb.log_zone_size;
    uint32_t *direct_zones = safe_calloc(ZONE_SIZE/sizeof(uint32_t), sizeof(uint32_t));
    int index=0;
    *num_zones=0;
    safe_fseek(minix->image,
                minix->part.lFirst*SECTOR_SIZE + // start of part
                indirect*ZONE_SIZE, // zone in bytes
                SEEK_SET
     );
     
    // go through each part and 
    while(ZONE_SIZE > 0){
        // single addr
        safe_fread(direct_zones + *num_zones, sizeof(uint32_t), 1, minix->image);
        ZONE_SIZE-=sizeof(uint32_t);
        *num_zones++;
    }
    return direct_zones;
}

// wrapper function for get_entrys
dirent_t *get_entrys(const minix_t *minix, const inode_t *dir){
    int index=0, j=0;
    dirent_t *entrys;
    uint32_t b_left, *indirect, *two_indirect;
    int num_zones = 0, inner_num_zones=0;
    entrys=safe_calloc(dir->size, sizeof(dirent_t));
    // step 1 - go through direct zones
    index = set_entrys(minix, dir->zone, DIRECT_ZONES, dir->size, index, entrys);
    // check if we need to go through indirect zones
    if((b_left = dir->size - index*sizeof(dirent_t))!= 0 && dir->indirect){
       // step 2 - go through indirect zones if needed
        indirect = read_indirect_zones(minix, dir->indirect, &num_zones);
        index = set_entrys(minix, indirect, num_zones, b_left, index, entrys);
        b_left = dir->size - index*sizeof(dirent_t);
        free(indirect);
    }
    // ceck to see if we need go through double indirect
    if(b_left != 0 && dir->two_indirect){
        // step 3 - go through every double indirect zones
        two_indirect = read_indirect_zones(minix, dir->two_indirect, &num_zones);
        for(j=0; j< num_zones && b_left; j++){
            // for each single indirect zone
            indirect = read_indirect_zones(minix, two_indirect[j], &inner_num_zones);
            index = set_entrys(minix, indirect, inner_num_zones, b_left, index, entrys);
            b_left = dir->size - index*sizeof(dirent_t);
            free(indirect);
        }
        free(two_indirect);
    }
     
    // size of each indirect address
    return entrys;
}















// finds the inode number for the given name
int find_inode_num(dirent_t *entrys, int size, char *name){
    int i;
    for(i=0; i< size; i++){
        if(strcmp(entrys[i].name, name)==0)
            return entrys[i].inode;
    }
    // case where name == root
    if(strcmp(name, "/") == 0){
        return ROOT_INODE;
    }
    return NOT_FOUND;
}



/**
 * returns the inode number of the file_path (could be directory / file)
 */
int Get_Inode_Num(const minix_t *minix, int inode_num, char *file_path){

    dirent_t *entrys;
    // index Pair<cwd inode num, file inode>
    char next_path[1000] = {0}, *base_dir;
    int next_inode;

    // base case stop when we find the basename(path) = filename
    if(strcmp(file_path, basename(file_path)) == 0){
        return inode_num;
    }else{
        // step 1 - get entrys from c
        entrys = get_entrys(minix, &minix->inodes[inode_num]);
        // step 2 - remove current directory from path
        basename_path(file_path, next_path);
        // step 3 - get the inode num for that next entry
        next_inode=find_inode_num(entrys, minix->inodes[inode_num].size, (base_dir=get_dirname(next_path)));
        free(entrys);
        free(base_dir);
        // step 4 - recurse
        return Get_Inode_Num(minix, next_inode, next_path);
    }
}


// wrapper function of for get INODE_NUM
int get_inode_num(minix_t *minix, char *_path){
    char *full_path, file_path[1000] = {0};
    char path[1000] = {0};
    strcpy(file_path, path);
    // have to append a / if folder_path isnt given it
    if(file_path[0] != '/'){
        // append it 
        strcpy(path, "/");
        strcat(path, file_path);
    }else{
        strcpy(path, file_path);
    }
    // start at root
    int index = Get_Inode_Num(minix, ROOT_INODE, path);
    return index;
}


/**********************************************************************************/


file_t get_type(inode_t file){
    if(GET_PERM(file.mode, MASK_DIR, 'd') == 'd'){
        return DIRECTORY;
    }
    return REGULAR;
}
void print_directory(minix_t *minix, dirent_t *entrys, inode_t *folder){
    int i, inode_num;
    inode_t inode_entry;
    char *mode;
    for(i=0; i< (folder->size)/sizeof(dirent_t); i++){
        // print out non 
        if((inode_num=entrys[i].inode) != DELETED_INODE){
            mode = get_mode(minix->inodes[inode_num].mode);
            printf(" %s, %d, %s,\n", mode, entrys[i].inode, entrys[i].name);
            free(mode);
        }
    }
}
void print_regular_file(minix_t *minix, int inode_num){
        char *mode = get_mode(minix->inodes[inode_num].mode);
        printf("%s, %d, %s,\n", mode, inode_num, minix->opt.srcpath);
        free(mode);
}

void print_all(minix_t *minix){
    int verbosity;
    file_t type;
    dirent_t * entrys;
    uint8_t *data;
    int inode_num = get_inode_num(minix, minix->opt.srcpath);

    
   if((verbosity=minix->opt.verbosity)){
      if(verbosity == 2) {
          print_options(minix);
      }else{
          print_superBlock(minix);
          print_inode_metadata(minix, minix->inodes[inode_num]);
      }
   }

   // print contents of inode
   if(get_type(minix->inodes[inode_num]) == DIRECTORY){
       entrys=get_entrys(minix, &minix->inodes[inode_num]);
       // print out each file in that directory
       print_directory(minix, entrys, &minix->inodes[inode_num]);
   }else{
       print_regular_file(minix, inode_num);
   }

}


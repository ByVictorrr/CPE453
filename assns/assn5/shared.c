
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
/************************PRINT FUNCTION ************************************************/

/*
* A general function that allows us to read from an inode
*/
dirent_t *get_entrys(const minix_t *minix, const inode_t *dir){
                    
    int ZONE_SIZE = minix->sb.blocksize << minix->sb.log_zone_size;
    int i, j; 
    uint32_t b_left, index=0;
    dirent_t *entrys;
    entrys=safe_calloc(dir->size, sizeof(dirent_t));
    b_left = dir->size;

    uint8_t data[10000];
    // go through every direct zones
    for (i=0; i< DIRECT_ZONES && b_left; i++){
        // to determine what size to read
        int read_size = MIN(b_left, ZONE_SIZE), r_size=read_size;
        // continue if zone is 0
        if(dir->zone[i] == 0){
            continue;
        }
        // set the cursor for that zone
        safe_fseek(minix->image,
                minix->part.lFirst*SECTOR_SIZE + // start of part
                dir->zone[i]*ZONE_SIZE, // zone in bytes
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
 * recursive function that goes through directies until it finds the src_path
 * returns the inode corresponding to the file_path
 */
inode_t *Find_Folder(const minix_t *minix, int inode_num, char *folder_path){

    inode_t *curr_folder = &minix->inodes[inode_num];
    dirent_t *entrys;
    int next_inode_num;
    char next_path[1000] = {0}, *next_folder_base;

    // base case stop when we find the base name
    if(strcmp(folder_path, basename(folder_path)) == 0){
        return curr_folder;
    }else{
        // step 1 - get entrys from c
        entrys = get_entrys(minix, curr_folder);
        // step 2 - remove current directory from path
        basename_path(folder_path, next_path);
        // step 3 - get the inode num for that next entry
        next_inode_num=find_inode_num(entrys, curr_folder->size, dirname(next_path));
        free(entrys);
        // step 4 - recurse
        return Find_Folder(minix, next_inode_num, next_path);
    }
}


// wrapper function of Find file_file
inode_t *find_folder(minix_t *minix){
    char *folder_path, file_path[1000] = {0};
    char path[1000] = {0};
    strcpy(file_path, minix->opt.srcpath);
    folder_path= dirname(file_path);
    // have to append a / if folder_path isnt given it
    if(folder_path[0] != '/'){
        // append it 
        strcpy(path, "/");
        strcat(path, folder_path);
    }else{
        strcpy(path, folder_path);
    }
    // start at root
    inode_t *found = Find_Folder(minix, ROOT_INODE, path);
    return found;
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
void print_regular_file(minix_t *minix, inode_t *folder, int inode_num){
        
        char *mode = get_mode(minix->inodes[inode_num].mode);
        printf("%s, %d, %s,\n", mode, inode_num, minix->opt.srcpath);
        free(mode);
}

void print_all(minix_t *minix){
    int verbosity;
    file_t type;
    inode_t *folder = find_folder(minix);
    dirent_t * entrys;
    uint8_t *data;
    int inode_num;
    
    // case were inode num is not found in directory
    if((inode_num=find_inode_num((entrys=get_entrys(minix, folder))
       ,folder->size, basename(minix->opt.srcpath))) == NOT_FOUND){
           printf("couldnt find", minix->opt.srcpath);
           exit(EXIT_FAILURE);
    }

   if((verbosity=minix->opt.verbosity)){
      if(verbosity == 2) {
          print_options(minix);
      }else{
          print_superBlock(minix);
          print_inode_metadata(minix, minix->inodes[inode_num]);
      }
   }

   // print contents of inode
   if((type=get_type(minix->inodes[inode_num])) == DIRECTORY){
       // print out each file in that directory
       print_directory(minix, entrys, &minix->inodes[inode_num]);
   }else{
       print_regular_file(minix, folder, inode_num);
   }

}


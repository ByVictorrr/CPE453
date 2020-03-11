
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



void write_file(minix_t *minix, FILE *dest){
    int src_inode;
    uint8_t *src_data;
    uint16_t mode;
    // step 1 - get inode number of dest
    if((src_inode = get_inode_num(minix, minix->opt.srcpath)) == NOT_FOUND){
        printf("destination not found\n");
        exit(EXIT_FAILURE);
    // step 3.3 - check to  see if file is deleted
    }else if(src_inode == DELETED_INODE){
        printf("%s: File not found.\n", minix->opt.srcpath);
        exit(EXIT_FAILURE);
    }
    // step 2.1 - check to see if directory
    if(((mode=minix->inodes[src_inode].mode) & FILE_TYPE) == MASK_DIR){
        printf("%s: Not a regular file.\n", minix->opt.srcpath);
        exit(EXIT_FAILURE);
    // step 2.2 - check to see if symlink
    }else if((mode & FILE_TYPE) == MASK_SYM){
        printf("SymLink: Not a regular file.\n");
        exit(EXIT_FAILURE);
    }


    // step 2.2 - get contents of src_inode (assumption here is reg file)
    src_data = get_data(minix, &minix->inodes[src_inode]);

    // step 3 - update minix->inodes[dest_inode] folder with contents
    safe_fwrite(src_data, sizeof(uint8_t), minix->inodes[src_inode].size, dest);
    free(src_data);
}





/*
* A general function that allows us to read from an inode direct blocks
* size - size of reading type 
*/
uint32_t set_data(const minix_t *minix, uint32_t *zones, 
             int num_zones, uint64_t bleft ,int index, void * data
             ,size_t type_size){
                    
    int ZONE_SIZE = minix->sb.blocksize << minix->sb.log_zone_size;
    int i, j; 
    uint64_t b_left = bleft;
    // go through every direct zones
    for (i=0; i< num_zones && b_left; i++){
        // to determine what size to read
        int read_size = MIN(b_left, ZONE_SIZE), r_size=read_size;

        // case where zone[i] is zero but the others are adjacent and not(HOLE)
        if(zones[i] == 0 && type_size == sizeof(uint8_t) && b_left){
            // pad with zeros if this if not adjacent zones
            memset((uint8_t*)data+index, 0, read_size);
            index+=read_size;
            b_left-= read_size;
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
            if(type_size == sizeof(dirent_t)){
                safe_fread(((dirent_t*)data)+index, type_size, 1, minix->image);
            }else{
                safe_fread(((uint8_t*)data)+index, type_size, 1, minix->image);
            }
            read_size -= type_size;
            index++;
        }
        b_left = b_left-r_size;
    }

    return index;
}
uint32_t *read_indirect_zones(const minix_t *minix, 
                              uint32_t indirect, 
                              int *num_zones){
    // step 1 - go though the one zone and read each
    int ZONE_SIZE = minix->sb.blocksize << minix->sb.log_zone_size;
    uint32_t *direct_zones = safe_calloc(ZONE_SIZE/sizeof(uint32_t)
                                          ,sizeof(uint32_t));
    *num_zones=0;
    safe_fseek(minix->image,
                minix->part.lFirst*SECTOR_SIZE + // start of part
                indirect*ZONE_SIZE, // zone in bytes
                SEEK_SET
     );
     
    // go through each part and 
    while(ZONE_SIZE > 0){
        // single addr
        safe_fread(direct_zones + *num_zones, 
                    sizeof(uint32_t), 1, minix->image);

            ZONE_SIZE-=sizeof(uint32_t);
            *num_zones=*num_zones+1;
    }
    return direct_zones;
}
void debug_two_indirect(uint32_t * two_indirect, int num_zones){
    int i;
    for(i=0; i < num_zones; i++){
        printf("%d\n",two_indirect[i]);
    }
}



// wrapper function for get_entrys
void *get_data(const minix_t *minix, const inode_t *inode){
    uint32_t index=0, j, k;
    void *data;
    uint64_t b_left, *indirect, *two_indirect;
    int num_zones = 0, inner_num_zones=0;
    size_t type_size;
    // check if directory or file
    if(get_type(inode) == DIRECTORY){
        type_size=sizeof(dirent_t);
    }else{
        type_size=sizeof(uint8_t);
    }
    data=safe_calloc(inode->size/type_size, type_size);
    // step 1 - go through direct zones
    index = set_data(minix, inode->zone, DIRECT_ZONES, 
                     inode->size, index, data, type_size);
    // check if we need to go through indirect zones
    if((b_left = inode->size - index*type_size) != 0){
            // we have to padd holes with zeros to get to two_indirect
            if(!inode->indirect && inode->two_indirect){
               int ZONE_SIZE = minix->sb.blocksize << minix->sb.log_zone_size;
               indirect = safe_calloc(ZONE_SIZE/sizeof(uint32_t), 
                                            sizeof(uint32_t));
               num_zones= ZONE_SIZE/sizeof(uint32_t);
            }else{
                indirect = read_indirect_zones(minix, inode->indirect, 
                                                &num_zones);
            }
       // step 2 - go through indirect zones if needed
        index = set_data(minix, indirect, num_zones, b_left, 
                        index, data, type_size);
        b_left = inode->size - index*type_size;
        free(indirect);
    }
    // ceck to see if we need go through double indirect
    if(b_left != 0 && inode->two_indirect){
        // step 3 - go through every double indirect zones
        two_indirect = read_indirect_zones(minix, inode->two_indirect, 
                                            &num_zones);
        for(j=0; j< num_zones && b_left; j++){
            // for each single indirect zone
            if(two_indirect[j] == 0){
                // tell indirect to memset to all zeros
               int ZONE_SIZE = minix->sb.blocksize << minix->sb.log_zone_size;
               indirect = safe_calloc(ZONE_SIZE/sizeof(uint32_t)
                                          ,sizeof(uint32_t));
                inner_num_zones= ZONE_SIZE/sizeof(uint32_t);
            }else{
               indirect = read_indirect_zones(minix, two_indirect[j], 
                                            &inner_num_zones); 
            }
                //debug_two_indirect(indirect, inner_num_zones);
            index = set_data(minix, indirect, inner_num_zones, 
                              b_left, index, data, type_size);
            b_left = inode->size - index*type_size;
            free(indirect);
        }

        free(two_indirect);
    }
     
    // size of each indirect address
    return data;
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
        entrys = get_data(minix, &minix->inodes[inode_num]);
        // step 2 - remove current directory from path
        basename_path(file_path, next_path);
        // step 3 - get the inode num for that next entry
        next_inode=find_inode_num(entrys, minix->inodes[inode_num].size, 
                                (base_dir=get_dirname(next_path)));
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
    strcpy(file_path, _path);
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



file_t get_type(inode_t *file){
    if(GET_PERM(file->mode, MASK_DIR, 'd') == 'd'){
        return DIRECTORY;
    }
    return REGULAR;
}
void print_directory(minix_t *minix, dirent_t *entrys, inode_t *folder){
    int i, inode_num;
    inode_t inode_entry;
    char *mode;
    printf("%s:\n", minix->opt.srcpath);
    for(i=0; i< (folder->size)/sizeof(dirent_t); i++){
        // print out non 
        if((inode_num=entrys[i].inode) != DELETED_INODE){
            mode = get_mode(minix->inodes[inode_num].mode);
            printf("%s%10d %s\n", mode, minix->inodes[inode_num].size, 
                            entrys[i].name);
            free(mode);
        }
    }
}
void print_regular_file(minix_t *minix, int inode_num){
        char *mode = get_mode(minix->inodes[inode_num].mode);
        printf("%s%10d %s\n", mode, minix->inodes[inode_num].size, 
                            minix->opt.srcpath);
        free(mode);
}

void print_all(minix_t *minix){
    int verbosity;
    file_t type;
    dirent_t * entrys;
    uint8_t *data;
    int inode_num;

    // case where file isnt found 
    if((inode_num=get_inode_num(minix, minix->opt.srcpath)) == NOT_FOUND || 
        inode_num == DELETED_INODE
    ){
        fprintf(stderr, "%s: File not found.\n", minix->opt.srcpath);
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
   if(get_type(&minix->inodes[inode_num]) == DIRECTORY){
       entrys=get_data(minix, &minix->inodes[inode_num]);
       // print out each file in that directory
       print_directory(minix, entrys, &minix->inodes[inode_num]);
   }else{
       print_regular_file(minix, inode_num);
   }

}


#include <libgen.h>
#include <stdio.h>
#include <string.h>
#include "types.h"

/**
 * Takes in a path as a string and then gives you back
 * a base_name_path which is cutting off a directory above the path
 * */
void basename_path(const char *path, char*base_name_path){
    // step 1 - look for "/"" or name till next /
    int i=0;
    // case 1 - where root is in path
    if(path[0] == '/'){
        strcpy(base_name_path, path+1);
        return;
    }
    // case 2 - where name of curr dir then / followed
    for(i=0; i < strlen(path); i++){
        if(path[i]== '/'){
            strcpy(base_name_path, path+i+1);
            return;
        }
    }
}
// wrapper to dirname to help support when return . 
// it will give back the name instead
char *get_dirname(char *path){
    char *base_dir_name = NULL;
    char buff[1000] = {0}, next_buff[1000] = {0};
    base_dir_name = safe_calloc(strlen(path)+1, sizeof(char));
    strcpy(next_buff, path);
    strcpy(buff, next_buff);
    while(strcmp(dirname(next_buff),".") != 0){
        strcpy(buff, next_buff);
    }
    // incase the doesnt contain '.'
    if(buff[0] != '.'){
        strcpy(next_buff, buff);
    }
    strcpy(base_dir_name, next_buff);
    return base_dir_name;
}
#include <libgen.h>
#include <stdio.h>
#include <string.h>
#include "types.h"

/**
 * Takes in a path as a string and then gives you back
 * a base_name_path which is cutting off a directory above the path
 * */
void basename_path(char *path, char*base_name_path){
    // step 1 - look for "/"" or name till next /
    int i=0;
    // case 1 - where root is in path
    if(path[0] == '/'){
        strcpy(base_name_path, path+1);
    }
    // case 2 - where name of curr dir then / followed
    for(i=0; i < strlen(path); i++){
        if(path[i]== '/'){
            strcpy(base_name_path, path+i+1);
        }
    }
}
#ifndef DIRENT_H_
#define DIRENT_H_
#include <stdint.h>

typedef struct dirent{
    uint32_t inode; /*i node number*/
    unsigned char name[60]; /* filename string*/
}dirent_t;
#endif
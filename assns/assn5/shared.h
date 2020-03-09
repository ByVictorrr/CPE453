#ifndef SHARED_H_
#define SHARED_H_

#include "types.h"
#define MIN(a,b) ((a) < (b) ? (a):(b))
typedef enum{REGULAR, DIRECTORY} file_t;



dirent_t *get_entrys(const minix_t *minix,const inode_t *dir);                    
int find_inode_num(dirent_t *entrys, int size, char *name);
inode_t *Find_Folder(const minix_t *minix,int inode_num,char *folder_path);
inode_t *find_folder(minix_t *minix);


file_t get_type(inode_t file);
void print_directory(minix_t *minix, dirent_t *entrys, inode_t *folder);
void print_regular_file(minix_t *minix, inode_t *folder, int inode_num);
void print_all(minix_t *minix);
#endif


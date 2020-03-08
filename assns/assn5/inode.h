#ifndef INODE_H_
#define INODE_H_

#include <stdint.h>
#define DIRECT_ZONES 7

typedef struct inode {
      uint16_t mode;                /* file type, protection, etc. */
      uint16_t links;               /* how many links to this file */
      uint16_t uid;                 /* user id of the file's owner */
      uint16_t gid;                 /* group number */
      uint32_t size;                /* current file size in bytes */
      int32_t atime;                /* time of last access (V2 only) */
      int32_t mtime;                /* when was file data last changed */
      int32_t ctime;                /* when was inode itself changed (V2 only)*/
      uint32_t zone[DIRECT_ZONES];  /* zone numbers for direct, ind, and dbl ind */
      uint32_t indirect;            /* # direct zones (Vx_NR_DZONES) */
      uint32_t two_indirect;        /* # indirect zones per indirect block */
      uint32_t unused;
} inode_t;


#endif

#ifndef INODE_H_
#define INODE_H_

#include <stdint.h>
#define DIRECT_ZONES 7

typedef struct inode {
      uint16_t mode;            /* mode */
      uint16_t links;           /* number or links */
      uint16_t uid;
      uint16_t gid;
      uint32_t size;
      int32_t atime;
      int32_t mtime;
      int32_t ctime;
      uint32_t zone[DIRECT_ZONES];
      uint32_t indirect;
      uint32_t two_indirect;
      uint32_t unused;
}inode_t;

#endif
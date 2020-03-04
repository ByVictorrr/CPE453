#ifndef __MFS_SUPER_H__
#define __MFS_SUPER_H__
#include <stdint.h>
#include <stdlib.h>

/* Super block table.  The root file system and every mounted file system
 * has an entry here.  The entry holds information about the sizes of the bit
 * maps and inodes.  The s_ninodes field gives the number of inodes available
 * for files and directories, including the root directory.  Inode 0 is 
 * on the disk, but not used.  Thus s_ninodes = 4 means that 5 bits will be
 * used in the bit map, bit 0, which is always 1 and not used, and bits 1-4
 * for files and directories.  The disk layout is:
 *
 *    Item        # blocks
 *    boot block      1
 *    super block     1    (offset 1kB)
 *    inode map     s_imap_blocks
 *    zone map      s_zmap_blocks
 *    inodes        (s_ninodes + 'inodes per block' - 1)/'inodes per block'
 *    unused        whatever is needed to fill out the current zone
 *    data zones    (s_zones - s_firstdatazone) << s_log_zone_size
 *
 * A super_block slot is free if s_dev == NO_DEV. 
 */

typedef struct super_block {
  uint32_t ninodes;		/* # usable inodes on the minor device */
  uint16_t pad1;
  int16_t i_blocks; /* # of blocks used by inode bit map */
  int16_t z_blocks; /* # of blocks used by zone bit map */
  uint16_t firstdata; /* number of first data zone */
  int16_t log_zone_size; /* log2 of blocks per zone */
  int16_t pad2; /* make things line up*/
  uint32_t max_file; /* max file size */
  uint32_t zones; /* number of zones on disk */
  int16_t magic; /* magic number */
  int16_t pad3; /* make things line up*/
  uint16_t blocksize; /* block size in bytes */
  uint8_t subversion; /* filesystem sub-version */
} superblock_t;

#define IMAP		0	/* operating on the inode bit map */
#define ZMAP		1	/* operating on the zone bit map */

#endif


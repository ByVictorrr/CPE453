#ifndef TYPES_H_
#define TYPES_H_
#include <stdint.h>
#include <stdint.h>

#define LENGTH 1000
// In bytes
#define SECTOR_SIZE 512
// Location of partition table
#define ADDR_PARTITION_TABLE  0x1BE
// indicates a valid minix partition
#define MINIX_PART 0x81
// minix 3 magic number (little endian)
#define MINIX_MAGIC 0x4D5A
#define BOOT_SIZE 1024
#define SUPER_BLOCK_SIZE BOOT_SIZE
#define DIRECT_ZONES 7


#define MASK_DIR  0040000
#define MASK_O_R  0000400
#define MASK_O_W  0000200
#define MASK_O_X  0000100
#define MASK_G_R  0000040
#define MASK_G_W  0000020
#define MASK_G_X  0000010
#define MASK_OT_R 0000004
#define MASK_OT_W 0000002
#define MASK_OT_X 0000001
#define GET_PERM(mode, mask, c) ( (((mode)&(mask)) == mask) ? c : '-' )

typedef enum BOOL{FALSE,TRUE} bool_t;
typedef struct __attribute__ ((__packed__)) inode {
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


typedef struct __attribute__ ((__packed__)) dirent{
    uint32_t inode; /*i node number*/
    unsigned char name[60]; /* filename string*/
}dirent_t;

typedef struct __attribute__ ((__packed__)) partition{
    uint8_t bootind; /* boot magic number (0x80) if bootable*/
    uint8_t start_head; /* start of partition in CHS*/
    uint8_t start_sec;
    uint8_t start_cyl;
    uint8_t type; /* type of partion (0x81 is minix)*/
    uint8_t end_head; /* end of partion in CHS*/
    uint8_t end_sec; /* end of partion in CHS*/
    uint8_t end_cyl;
    uint32_t lFirst; /* first sector (Lba addressing) */
    uint32_t size;  /* size of partion (in sectors)*/
}partition_t;

typedef struct __attribute__ ((__packed__)) super_block {
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

typedef uint64_t bmap_t;

typedef struct minix{
  partition_t part;
  superblock_t sb;
  bmap_t i_map, z_map;
  inode_t *inodes;
}minix_t;

void safe_fseek(FILE *fp, long int offset, int pos);
void safe_fread(void *ptr, size_t size, size_t nmemb, FILE *stream);
void *safe_malloc(size_t size);
void *safe_calloc(size_t nitems, size_t size);


/*====================PARTITION================================*/

bool_t is_part_table_valid(FILE *image, uint32_t start_addr);
partition_t get_partition(FILE *image, uint32_t addr);
partition_t read_partition(FILE * image, uint32_t offset, uint32_t im_addr);
/*                   MF                                          */
partition_t find_minix_partion(FILE *image, int prim_part, int sub_part);
void print_partition(superblock_t sb, inode_t * inodes);

/*****************************EO PARTITION*************************************************/

/******************************SUPER BLOCK*******************************************/
/* Given an image at @parm2 start reading SB from there */
superblock_t get_SB(FILE *image, const uint32_t first_sector);
void print_superBlock(minix_t minix);

/******************************INODE BLOCK*******************************************/
inode_t *get_inodes(FILE *image,const uint32_t first_sector, superblock_t sb);
void print_inode(minix_t minix, inode_t inode);

 #endif

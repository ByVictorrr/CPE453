

# Command to test IndirectCount

$/home/pn-cs453/demos/minls -v -v -v Images/BigIndirectDirs Level1/Level2

Options:
  opt->part      -1
  opt->subpart   -1
  opt->imagefile Images/BigIndirectDirs
  opt->srcpath   Level1/Level2
  opt->dstpath   (null)

Superblock Contents:
Stored Fields:
  ninodes          768
  i_blocks           1
  z_blocks           1
  firstdata         16
  log_zone_size      0 (zone size: 4096)
  max_file  4294967295
  magic         0x4d5a
  zones            360
  blocksize       4096
  subversion         0
Computed Fields:
  version            3
  firstImap          2
  firstZmap          3
  firstIblock        4
  zonesize        4096
  ptrs_per_zone   1024
  ino_per_block     64
  wrongended         0
  fileent_size      64
  max_filename      60
  ent_per_zone      64

File inode:

  unsigned short mode         0x41ed    (drwxr-xr-x)
  unsigned short links             3
  unsigned short uid               0
  unsigned short gid               0
  uint32_t  size          49024
  uint32_t  atime    1307218260 --- Sat Jun  4 13:11:00 2011
  uint32_t  mtime    1307218084 --- Sat Jun  4 13:08:04 2011
  uint32_t  ctime    1307218084 --- Sat Jun  4 13:08:04 2011

  Direct zones:
              zone[0]   =         19
              zone[1]   =         21
              zone[2]   =         22
              zone[3]   =         23
              zone[4]   =         24
              zone[5]   =         25
              zone[6]   =         26
   uint32_t  indirect   =         28
   uint32_t  double     =          0
Level1/Level2:
drwxr-xr-x     49024 .
drwxr-xr-x       192 ..
drwxr-xr-x       128 BigDir
-rw-r--r--         0 file_000
-rw-r--r--         0 file_001
-rw-r--r--         0 file_002
-rw-r--r--         0 file_003
-rw-r--r--         0 file_004


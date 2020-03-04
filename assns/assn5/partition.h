#include <stdint.h>

typedef struct partition{
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

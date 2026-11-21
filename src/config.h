// *** config.h *******************************************************

#ifndef CONFIG_INCLUDED
#define CONFIG_INCLUDED  1

#define FLASH_START  0x9D000000

#define FLASH_BYTES  (512*1024)  // PIC32MK0512GPK064
#define FLASH_PAGE_SIZE  4096

#define PIC32IPL4  IPL4SRS

#define FLASH2_START  0x9FC00000  // boot flash, for flash upgrade
#define FLASH2_BYTES  (20*1024)

#endif

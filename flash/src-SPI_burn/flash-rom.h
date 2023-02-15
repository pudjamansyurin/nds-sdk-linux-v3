#ifndef __FLASHROM__
#define __FLASHROM__

typedef struct  _flash_dev {
        int (*flash_chk)(void);
        int (*flash_erase)(unsigned int, unsigned int);
        int (*flash_program)(unsigned int, unsigned char *, unsigned int);
        int (*flash_read)(unsigned int, unsigned char *, unsigned int);
        int (*flash_lock)(unsigned int, unsigned int);
        int (*flash_unlock)(unsigned int, unsigned int);
        unsigned int flash_blksize;
        unsigned int flash_chipsize;
} flash_dev, *pflash_dev;



#endif


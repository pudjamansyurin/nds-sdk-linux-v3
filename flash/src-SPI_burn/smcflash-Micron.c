#include <sys/time.h>
#ifdef __MINGW32__
#include <winsock2.h>  // for Sleep()
#endif
#include "util.h"
#include "flash-rom.h"

extern unsigned int guiFastMode;

#define MX_Type     (0x01)
#define SST_Type    (0x02)
#define Intel_Type  (0x03)
#define AMD_Type    (0x04)
#define Micron_Type (0x05)

#define FLASH_SINGLE (0x00)
#define FLASH_DOUBLE (0x01)
#define FLASH_FOUR   (0x02)

#ifdef __MINGW32__
#define SLEEP(milliseconds) Sleep(milliseconds)
#else
#define SLEEP(milliseconds) usleep((milliseconds) * 1000)
#endif

#define FLASH_BLKSIZE    0x20000
#define FLASH_CHIPSIZE   0x4000000

void micron_wait_program_done(unsigned base, unsigned last_addr, unsigned short last_data);
void micron_flash_unlock_bypass(unsigned base);
void micron_flash_unlock_bypass_reset(unsigned base);
void micron_flash_reset(unsigned base);
void micron_flash_return_to_read(unsigned base);
int micron_check(void);
int micron_program(unsigned int FlashAddr, unsigned char *start, unsigned int DataSize);
int micron_erase(unsigned int FlashAddr, unsigned int DataSize);
int micron_read(unsigned int FlashAddr, unsigned char *start, unsigned int DataSize);
int micron_lock(unsigned int FlashAddr, unsigned int DataSize);
int micron_unlock(unsigned int FlashAddr, unsigned int DataSize);

flash_dev flash_Micron = {
        micron_check,
        micron_erase,
        micron_program,
        micron_read,
        micron_lock,
        micron_unlock,
        FLASH_BLKSIZE,
        FLASH_CHIPSIZE
};

extern FILE *pLogFile;
extern unsigned int guiCtrlBase;

void micron_ChipErase(unsigned int addr_start, unsigned int BusWidth)
{
        unsigned int base = guiCtrlBase;

        base += addr_start;
        //x16 555-AA 2AA-55 555-80 555-AA 2AA-55 BAd-30
        outh(base + 0x555*2, 0xaa);
        outh(base + 0x2aa*2, 0x55);
        outh(base + 0x555*2, 0x80);
        outh(base + 0x555*2, 0xaa);
        outh(base + 0x2aa*2, 0x55);
        outh(base, 0x30);

        while ((inh(base) ^ 0xffff) & 0xffff) {
                SLEEP (32);
        }
}

int micron_ProgramMultiWord(unsigned int base, volatile unsigned int address, unsigned char *buffer, unsigned int size)
{
        volatile unsigned int bytes = size % 4;
        volatile unsigned int word;
        unsigned int last_address;
        unsigned int j;

        /* not word align */
        if (size % 4 != 0)
                size = size - bytes;
        if (size != 0) {
                //x16 555-AA 2AA-55 BAd-25 BAd-N PA PD
                word = (size / 2) - 1;
                outh(base + 0x555*2, 0xaa);
                outh(base + 0x2aa*2, 0x55);
                outh(base, 0x25);
                outh(base, word);
                if (fastout(base + address, size, (char*)buffer) < 0)
                        return -1;

                /* last program address */
                last_address = address + size - 0x2;

                /* WRITE TO BUFFER PROGRAM CONFIRM */
                outh(base + last_address, 0x29);
                SLEEP(1);
                if (guiFastMode == 0) {
                        /* NOTE: if skip this checking, it may cause some stability issues in some platforms */
                        /* NOTE: typical block write timeout 128us */
                        micron_wait_program_done(base, last_address, *((short *)(buffer + size - 2)));
                        //SLEEP(100);
                }
        }
        if (bytes) {
                word = 0;
                for (j = 0; j <= bytes; j++)
                        word = word | (buffer[size+j] << (j*8));

                if (micron_ProgramMultiWord(base, address + size, (unsigned char *)&word, 4) < 0)
                        return -1;
        }
        return 0;
}

/*
 * functions for Micron flash
 */
void micron_wait_program_done(unsigned int base, unsigned int last_addr, unsigned short last_data)
{
        short data;
        unsigned time = 0;
        unsigned retry_count = 4096 * 2;  // NOTE: typical max block erase timeout = 4s
        data = inh(base + last_addr);

        while ((data ^ last_data) & 0x80) {
                data = inh(base + last_addr);
                /* check failure or abort bit */
                if (data & 0x22) {
                        data = inh(base + last_addr);
                        if ((data ^ last_data) & 0x80) {
                                /* BUFFERED PROGRAM ABORT and RESET command */
                                outh(base + 0x555*2, 0xaa);
                                outh(base + 0x2aa*2, 0x55);
                                outh(base + 0x555*2, 0xf0);
                        }
                }
                time++;
                if (time > 100) {
                        SLEEP(1);
                        fprintf(pLogFile, "program failure or abort! status = %08x\n", data);
                }
                if (time > retry_count) {
                        fprintf(pLogFile, "program fail, operation abort\n");
                        terminate();
                        exit(1);
                }
        }
}

void micron_flash_unlock_bypass(unsigned int base)
{
        outh(base + 0x555*2, 0xaa);
        outh(base + 0x2aa*2, 0x55);
        outh(base + 0x555*2, 0x20);
}

void micron_flash_unlock_bypass_reset(unsigned int base)
{
        outh(base, 0x0090);
        outh(base, 0x0000);
}

void micron_flash_reset(unsigned int base)
{
        outh(base, 0x00f0);
}

void micron_flash_return_to_read(unsigned int base)
{
        /* execute UNLOCK BYPASS RESET command to return to read/reset mode from unlock bypass mode */
        micron_flash_unlock_bypass_reset(base);
        /* execute RESET command to return to read/reset mode and resets the errors */
        micron_flash_reset(base);
}
// TODO: modify this function to target-specific function - check flash
int micron_check(void)
{
        unsigned int base = guiCtrlBase;
        unsigned short id1, id2;

        micron_flash_return_to_read(base);

        /* go to AUTO SELECT mode */
        outh(base+0x555*2, 0x00aa);
        outh(base+0x2aa*2, 0x0055);
        outh(base+0x555*2, 0x0090);

        /* read ID */
        id1 = inh(base+0x0);
        id2 = inh(base+0x1*2);

        /* return to READ mode */
        micron_flash_reset(base);

        if (id1 == 0x0089 && id2 == 0x227e) {
                printf("Micron: ID1 = 0x%04hx, ID2 = 0x%04hx\n", id1, id2);
                return 0;
        } else
                return -1;
}
// TODO: modify this function to target-specific function - program flash
int micron_program(unsigned int FlashAddr, unsigned char *start, unsigned int DataSize)
{
        unsigned int base = guiCtrlBase;
        unsigned int address = FlashAddr;
        unsigned int end = address + DataSize;
        unsigned int buf_size=0;
        unsigned int boundary;
        int result = 0;

        while (address < end) {
                /*
                 * Micron flash cannot write across 1k boundary, if
                 * this is the case, it should be split into two part.
                 */

                boundary = 1024;
                buf_size = ((end - address) > boundary) ? boundary : (end - address);
                if (address / boundary != (address + buf_size - 4) / boundary) {
                        buf_size = ((address / boundary) + 1) * boundary - address;
                        printf("buf_size = %d\n", buf_size);
                }
                if (micron_ProgramMultiWord(base, address, start, buf_size) < 0) {
                        result = -1;
                        break;
                }

                start += buf_size;
                address += buf_size;
                if (address % 8*1024 == 0) {	// IDE use 8K/dot
                        fprintf(pLogFile, ".");
                        fflush(pLogFile);
                }
        }
        return result;
}
// TODO: modify this function to target-specific function - erase flash
int micron_erase(unsigned int FlashAddr, unsigned int DataSize)
{
        unsigned int EraseAddrStart, EraseSize, EraseBlkCnt, EraseBlkIndex, i;
        unsigned int base, Status;

        EraseAddrStart = (FlashAddr / FLASH_BLKSIZE) * FLASH_BLKSIZE;
        EraseSize = (FlashAddr - EraseAddrStart) + DataSize;
        EraseBlkCnt = (EraseSize + (FLASH_BLKSIZE - 1)) / FLASH_BLKSIZE;
        EraseBlkIndex = (EraseAddrStart / FLASH_BLKSIZE);

        base = guiCtrlBase;
        base += EraseAddrStart;
        for (i = 0; i < EraseBlkCnt; i++) {
                //ENTER NONVOLATILE PROTECTION COMMAND SET (C0h)
                // 555-AA 2AA-55 555-C0
                outh(base + 0x555*2, 0xaa);
                outh(base + 0x2aa*2, 0x55);
                outh(base + 0x555*2, 0xC0);

                //READ NONVOLATILE PROTECTION BIT STATUS
                // BAd READ(0)
                Status = inh(base+0x0);
                //printf("micron_erase Status= %x\n", Status);
                if (Status == 0) {
                        printf("\nmicron_erase - Flash block locked.\n");
                        return 1;
                }
                //EXIT PROTECTION COMMAND SET (90/00h)
                // X-90 X-00
                outh(base, 0x90);
                outh(base, 0x00);

                base += FLASH_BLKSIZE;
        }

        for (i = 0; i < EraseBlkCnt; i++) {
                fprintf(pLogFile, "erasing block %03d (0x%07x ~ 0x%07x)\n", EraseBlkIndex, EraseAddrStart, EraseAddrStart + FLASH_BLKSIZE);
                fflush(pLogFile);
                micron_ChipErase(EraseAddrStart, FLASH_FOUR);
                EraseAddrStart += FLASH_BLKSIZE;
                EraseBlkIndex++;
        }
        return 0;
}
// TODO: modify this function to target-specific function - read flash
int micron_read(unsigned int FlashAddr, unsigned char *start, unsigned int DataSize)
{
        unsigned int base = guiCtrlBase;
        base += FlashAddr;
        DataSize = ((DataSize + 3) / 4) * 4;
        fastin(base, DataSize, (char *)start);
        return 0;
}
// TODO: modify this function to target-specific function - lock flash
int micron_lock(unsigned int FlashAddr, unsigned int DataSize)
{
        unsigned int base = guiCtrlBase + FlashAddr;
        unsigned int addr_end = (base+DataSize);
        unsigned int Status=0;

        while (base < addr_end) {
                //ENTER NONVOLATILE PROTECTION COMMAND SET (C0h)
                // 555-AA 2AA-55 555-C0
                outh(base + 0x555*2, 0xaa);
                outh(base + 0x2aa*2, 0x55);
                outh(base + 0x555*2, 0xC0);

                //READ NONVOLATILE PROTECTION BIT STATUS
                // BAd READ(0)
                Status = inh(base+0x0);
                //printf("micron_lock Status= %x\n", Status);

                //PROGRAM NONVOLATILE PROTECTION BIT (A0h)
                // X-A0 BAd-00
                outh(base, 0xA0);
                outh(base, 0x00);

                //CLEAR ALL NONVOLATILE PROTECTION BITS (80/30h)
                // X-80 00-30
                //outh(base, 0x80);
                //outh(base, 0x30);

                Status = inh(base+0x0);
                printf("micron_lock Status= %x\n", Status);

                //EXIT PROTECTION COMMAND SET (90/00h)
                // X-90 X-00
                outh(base, 0x90);
                outh(base, 0x00);

                base += FLASH_BLKSIZE;
        }
        return 0;
}

// TODO: modify this function to target-specific function - unlock flash
int micron_unlock(unsigned int FlashAddr, unsigned int DataSize)
{
        unsigned int base = guiCtrlBase + FlashAddr;
        unsigned int addr_end = (base+DataSize);
        unsigned int Status=0;

        while (base < addr_end) {
                //ENTER NONVOLATILE PROTECTION COMMAND SET (C0h)
                // 555-AA 2AA-55 555-C0
                outh(base + 0x555*2, 0xaa);
                outh(base + 0x2aa*2, 0x55);
                outh(base + 0x555*2, 0xC0);

                //READ NONVOLATILE PROTECTION BIT STATUS
                // BAd READ(0)
                Status = inh(base+0x0);
                //printf("micron_lock Status= %x\n", Status);

                //PROGRAM NONVOLATILE PROTECTION BIT (A0h)
                // X-A0 BAd-00
                //outh(base, 0xA0);
                //outh(base, 0x00);

                //CLEAR ALL NONVOLATILE PROTECTION BITS (80/30h)
                // X-80 00-30
                outh(base, 0x80);
                outh(base, 0x30);

                while (1) {
                        Status = inh(base+0x0);
                        Status = inh(base+0x0);
                        //printf("micron_lock Status= %x\n", Status);
                        if (Status == 0x01)
                                break;
                }
                //EXIT PROTECTION COMMAND SET (90/00h)
                // X-90 X-00
                outh(base, 0x90);
                outh(base, 0x00);

                base += FLASH_BLKSIZE;
        }
        return 0;
}


#include <sys/time.h>
#ifdef __MINGW32__
#include <winsock2.h>  // for Sleep()
#endif
#include "util.h"
#include "flash-rom.h"

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

#define FLASH_BLKSIZE    0x40000
#define FLASH_CHIPSIZE   0x2000000

int IntelJ3_check(void);
int IntelJ3_program(unsigned int FlashAddr, unsigned char *start, unsigned int DataSize);
int IntelJ3_erase(unsigned int FlashAddr, unsigned int DataSize);
int IntelJ3_read(unsigned int FlashAddr, unsigned char *start, unsigned int DataSize);
int IntelJ3_lock(unsigned int FlashAddr, unsigned int DataSize);
int IntelJ3_unlock(unsigned int FlashAddr, unsigned int DataSize);

flash_dev flash_IntelJ3 = {
        IntelJ3_check,
        IntelJ3_erase,
        IntelJ3_program,
        IntelJ3_read,
        IntelJ3_lock,
        IntelJ3_unlock,
        FLASH_BLKSIZE,
        FLASH_CHIPSIZE
};

extern FILE *pLogFile;
extern unsigned int guiCtrlBase;
extern unsigned int guiFastMode;

/* Reference: Numonyx Embedded Flash Memory (J3 v. D)
   Table 19: Status Register Bit Definitions:
   --------------------------------------------------------------------------------------------
   Bit   |Name                   | Description
   --------------------------------------------------------------------------------------------
   1     |Block-Locked Error     |0 = Block NOT locked during program or erase
         |                       |1 = Block locked during program or erase
   --------------------------------------------------------------------------------------------
   2     |Program Suspend Status |0 = Program suspend not in effect
         |                       |1 = Program suspend in effect
   --------------------------------------------------------------------------------------------
   3     |Error                  |0 = within acceptable limits during program or erase operation
         |                       |1 = not within acceptable limits during program or erase operation.
         |                       |    Operation aborted
   --------------------------------------------------------------------------------------------
   4:5   |Command Sequence Error |00 = Program or erase operation successful
         |                       |10 = Program error - operation aborted
         |                       |01 = Erase error - operation aborted
         |                       |11 = Command sequence error - command aborted
   --------------------------------------------------------------------------------------------
   6     |Erase Suspend Status   |0 = Erase suspend not in effect
         |                       |1 = Erase suspend in effect
   --------------------------------------------------------------------------------------------
   7     |Ready Status           |0 = Device is busy; SR[6:] are invalid (Not driven)
         |                       |1 = Device is ready SR[6:0] are valid
   --------------------------------------------------------------------------------------------
   Figure 22: Status Register Flowchart */
// TODO: modify this function to target-specific check status function
void check(unsigned int data, const char *label)
{
        int err_occur = 0;

        if (data & 0x40) {
                fprintf(pLogFile, "\n%s - Erase suspend.\n", label);
                fflush(pLogFile);
                err_occur = 1;
        } else if (data & 0x04) {
                fprintf(pLogFile, "\n%s - Program suspend.\n", label);
                fflush(pLogFile);
                err_occur = 1;
        } else if (data & 2) {
                fprintf(pLogFile, "\n%s - Flash block locked.\n", label);
                fflush(pLogFile);
                err_occur = 1;
        } else if (data & 0x30) {
                if ((data & 0x30) == 0x30) {
                        fprintf(pLogFile, "\n%s - Command sequence error.\n", label);
                        fflush(pLogFile);
                } else if ((data & 0x30) == 0x20) {
                        fprintf(pLogFile, "\n%s - Erase error - operation aborted.\n", label);
                        fflush(pLogFile);
                } else if ((data & 0x30) == 0x10) {
                        fprintf(pLogFile, "\n%s - Program error - operation aborted.\n", label);
                        fflush(pLogFile);
                }
                err_occur = 1;
        } else if (data & 0x8) {
                fprintf(pLogFile, "\n%s - not within acceptable limits during program or erase operation.\n", label);
                fflush(pLogFile);
                err_occur = 1;
        }

        if (err_occur == 1) {
                terminate();
                exit(1);
        }
}

static void wait_done (unsigned int base, const char *label)
{
        unsigned data = 0;
        unsigned time = 0;
        unsigned retry_count = 4096 * 2;  // NOTE: typical max block erase timeout = 4s

        while (data != 0x00800080) {
                outw(base,0x00700070);
                data=inw(base);

                time++;
                if (time > 16)
                        SLEEP(1);

                if (time > retry_count)
                        check(data, label);
        }
}

/* Reference: Numonyx Embedded Flash Memory (J3 v. D) */
/* Table 30: Command Bus Operation */
/* Command                |Address Bus     |Data Bus   */
/* --------------------------------------------------- */
/* Read Identifier Codes  |Device Address  |0x90       */
/* --------------------------------------------------- */
// TODO: modify this function to target-specific function
void Flash_ReadID(unsigned int base, unsigned int BusWidth, unsigned int* DDI1, unsigned int* DDI2)
{
        switch (BusWidth) {
                case FLASH_SINGLE: // 8 bit
                        outw(base,0x90);
                        *DDI1 = inw(base+0x00);
                        *DDI2 = inw(base+0x01);
                        outw(base,0xff);  // Issue "Read Array" to exit Read Status Register mode
                        break;
                case FLASH_DOUBLE: // double 16 bit
                        outw(base,0x9090);
                        *DDI1 = inw(base+0x00);
                        *DDI2 = inw(base+0x02);
                        outw(base,0xffff); // Issue "Read Array" to exit Read Status Register mode
                        break;
                case FLASH_FOUR:// four 32 bit
                        outw(base,0x00500050);
                        outw(base,0x00900090);
                        *DDI1 = inw(base+0x00);
                        *DDI2 = inw(base+0x04);
                        outw(base,0x00ff00ff); // Issue "Read Array" to exit Read Status Register mode
                        break;
        }
}

/* Reference: Numonyx Embedded Flash Memory (J3 v. D) */
/* Table 30: Command Bus Operation */
/* Command                |Address Bus     |Data Bus   |Address Bus     |Data Bus   */
/* -------------------------------------------------------------------------------- */
/* Clear Status Register  |Device Address  |0x50       |---             |---        */
/* Unlock Block           |Device Address  |0x60       |Device Address  |0xD0       */
/* Read Status Register   |Device Address  |0x70       |---             |---        */
/* -------------------------------------------------------------------------------- */
// TODO: modify this function to target-specific function
void Flash_UnLock(unsigned int addr_start, unsigned int BusWidth)
{
        unsigned int base = guiCtrlBase;
        base += addr_start;

        outw(base,0x00600060);
        outw(base,0x00d000d0);
        wait_done (base, "Flash_UnLock");
        outw(base,0x00ff00ff);
}


/* Reference: Numonyx Embedded Flash Memory (J3 v. D) */
/* Table 30: Command Bus Operation */
/* Command                |Address Bus     |Data Bus   |Address Bus     |Data Bus   */
/* -------------------------------------------------------------------------------- */
/* Lock Block             |Device Address  |0x60       |Device Address  |0x01       */
/* Read Status Register   |Device Address  |0x70       |---             |---        */
/* -------------------------------------------------------------------------------- */
// TODO: modify this function to target-specific function
void Flash_Lock(unsigned int addr_start, unsigned int BusWidth)
{
        unsigned int base = guiCtrlBase;
        base += addr_start;

        outw(base,0x00600060);
        outw(base,0x00010001);
        // read status register
        wait_done (base, "Flash_Lock");
}

/* Reference: Numonyx Embedded Flash Memory (J3 v. D) */
/* Table 30: Command Bus Operation */
/* Command                |Address Bus     |Data Bus   |Address Bus     |Data Bus   */
/* -------------------------------------------------------------------------------- */
/* Block Erase            |Block Address   |0x20       |Block Address   |0xD0       */
/* Read Array             |Device Address  |0xFF       |---             |---        */
/* Read Status Register   |Device Address  |0x70       |---             |---        */
/* -------------------------------------------------------------------------------- */
// TODO: modify this function to target-specific function
void Flash_ChipErase(unsigned int addr_start, unsigned int BusWidth)
{
        unsigned int base = guiCtrlBase;
        base += addr_start;

        outw(base,0x00200020);
        outw(base,0x00d000d0);
        SLEEP (32);  // typical erase time = (2^10)ms = 1s
        wait_done (base, "Flash_ChipErase");
}

/* Reference: Numonyx Embedded Flash Memory (J3 v. D) */
/* Table 30: Command Bus Operation */
/* Command                |Address Bus     |Data Bus   |Address Bus     |Data Bus   */
/* -------------------------------------------------------------------------------- */
/* Word Program           |Device Address  |0x40       |Device Address  |Array Data */
/* Read Status Register   |Device Address  |0x70       |---             |---        */
/* Read Array             |Device Address  |0xFF       |---             |---        */
/* -------------------------------------------------------------------------------- */
// TODO: modify this function to target-specific function - write single word
int Flash_ProgramWord(unsigned int base, volatile unsigned int address, volatile unsigned int data)
{
        if (outw(base, 0x00400040) < 0)
                return -1;

        outw(base + address, data);
        wait_done (base, "Flash_ProgramWord");
        return 0;
}

/* Reference: Numonyx Embedded Flash Memory (J3 v. D) */
/* Table 30: Command Bus Operation */
/* Command                |Address Bus     |Data Bus   |Address Bus     |Data Bus   */
/* -------------------------------------------------------------------------------- */
/* Buffered Program       |Word Address    |0xE8       |Device Address  |0xD0       */
/* Read Status Register   |Device Address  |0x70       |---             |---        */
/* Read Array             |Device Address  |0xFF       |---             |---        */
/* -------------------------------------------------------------------------------- */
/* Figure 21: Write to Buffer Flowchart */
/* NOTE: Max is 16 words in one time for this flash */
// TODO: modify this function to target-specific function - write multiple words
int Flash_ProgramMultiWord(unsigned int base, volatile unsigned int address, unsigned char *buffer, unsigned int size)
{
        volatile unsigned int s_reg = 0;
        volatile unsigned int block_addr = (address/FLASH_BLKSIZE)*FLASH_BLKSIZE;
        volatile unsigned int bytes = size % 4;
        volatile unsigned int word;
        volatile unsigned int time = 0;
        unsigned int j;

        /* not word align */
        if (size % 4 != 0)
                size = size - bytes;

        if (size != 0) {
                time = 0;
                do {
                        outw(base + block_addr, 0x00e800e8);
                        s_reg = inw(base);

                        time++;
                        if (time >= 1000)
                                check(s_reg, "Flash_ProgramMultiWord");

                } while (s_reg != 0x00800080);  // check SR7

                word = (size/4) - 1;
                word = (word << 16) + word;
                outw(base + block_addr, word);
#if 0
                for (unsigned int i = 0; i < (size/4); i++) {
                        data = ((((unsigned int)buffer[4*i+3]<< 24) & 0xFF000000) |
                                (((unsigned int)buffer[4*i+2]<< 16) & 0x00FF0000) |
                                (((unsigned int)buffer[4*i+1]<< 8) & 0x0000FF00) |
                                (((unsigned int)buffer[4*i]<< 0) & 0x000000FF));
                        outw(base + address + 4*i, data);
                }
#else
                if (fastout(base + address, size, (char*)buffer) < 0)
                        return -1;
#endif
                outw(base + block_addr, 0x00d000d0);
                s_reg = 0;

                if (guiFastMode == 0)
                        /* NOTE: if skip this checking, it may cause some stability issues in some platforms */
                        wait_done (base, "Flash_ProgramMultiWord_Finish");
        }

        if (bytes) {
                word = 0;
                for (j = 0; j <= bytes; j++)
                        word = word | (buffer[size+j] << (j*8));

                if (Flash_ProgramWord (base, address+size, word) < 0)
                        return -1;
        }
        return 0;
}
// TODO: modify this function to target-specific function - check flash
int IntelJ3_check(void)
{
        unsigned int base = guiCtrlBase;
        unsigned int id1, id2;

        // TODO: change 0x890089 & 0x180018 to target-specific flash ID
        outw(base, 0x00500050);
        outw(base, 0x00900090);
        id1 = inw(base + 0x00);
        id2 = inw(base + 0x04);
        outw(base, 0x00ff00ff); // Issue "Read Array" to exit Read Status Register mode

        if (id1 == 0x890089 && id2 == 0x180018) {
                printf("IntelJ3: ID1 = 0x%08x, ID2 = 0x%08x\n", id1, id2);
                return 0;
        } else
                return -1;
}

/* main function to burn image to flash */
/* Use Flash_ProgramMultiWord () to write 64 bytes one time */
// TODO: modify this function to target-specific function - program flash
int IntelJ3_program(unsigned int FlashAddr, unsigned char *start, unsigned int DataSize)
{
        unsigned int base = guiCtrlBase;
        unsigned int address = FlashAddr;
        unsigned int end = FlashAddr + DataSize;
        unsigned int buf_size=0;
        unsigned int boundary;
        int result = 0;
        //unsigned int EraseAddrStart, EraseSize, EraseBlkCnt, i, j;

        while (address < end) {
                /* The boundary of Intel flash is 64 bytes */
                boundary = 64; //64=>J3D75, 128=>J3F75
                buf_size = ((end - address) > boundary) ? boundary : (end - address);
                if (Flash_ProgramMultiWord(base, address, start, buf_size) < 0) {
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
        outw(base, 0x00ff00ff);
        /* do not lock here, option "--lock" to do lock. */
#if 0
        /* lock flash */
        EraseAddrStart = (FlashAddr / FLASH_BLKSIZE) * FLASH_BLKSIZE;
        EraseSize = (FlashAddr - EraseAddrStart) + DataSize;
        EraseBlkCnt = (EraseSize + (FLASH_BLKSIZE - 1)) / FLASH_BLKSIZE;
        for (i = 0, j = EraseAddrStart; i < EraseBlkCnt; i++) {
                Flash_Lock(j, FLASH_FOUR);
                j += FLASH_BLKSIZE;
        }
        outw(base,0x00ff00ff);
#endif

        return result;
}
// TODO: modify this function to target-specific function - erase flash
int IntelJ3_erase(unsigned int FlashAddr, unsigned int DataSize)
{
        /*unsigned int base = guiCtrlBase;*/
        unsigned int EraseAddrStart, EraseSize, EraseBlkCnt, EraseBlkIndex, i;

        EraseAddrStart = (FlashAddr / FLASH_BLKSIZE) * FLASH_BLKSIZE;
        EraseSize = (FlashAddr - EraseAddrStart) + DataSize;
        EraseBlkCnt = (EraseSize + (FLASH_BLKSIZE - 1)) / FLASH_BLKSIZE;
        EraseBlkIndex = (EraseAddrStart / FLASH_BLKSIZE);
        /* unlock flash */
        /* do not unlock here, option "--unlock" to do unlock. */
        //Flash_UnLock(EraseAddrStart, FLASH_FOUR);

        for (i = 0; i < EraseBlkCnt; i++) {
                fprintf(pLogFile, "erasing block %03d (0x%07x ~ 0x%07x)\n", EraseBlkIndex, EraseAddrStart, EraseAddrStart + FLASH_BLKSIZE);
                fflush(pLogFile);
                Flash_ChipErase(EraseAddrStart, FLASH_FOUR);
                EraseAddrStart += FLASH_BLKSIZE;
                EraseBlkIndex++;
        }
        /* reset to read mode */
        /*outw(base, 0x00ff00ff);*/
        return 0;
}
// TODO: modify this function to target-specific function - read flash
int IntelJ3_read(unsigned int FlashAddr, unsigned char *start, unsigned int DataSize)
{
        unsigned int base = guiCtrlBase;
        fastin(base + FlashAddr, DataSize, (char *)start);
        return 0;
}
// TODO: modify this function to target-specific function - lock flash
int IntelJ3_lock(unsigned int FlashAddr, unsigned int DataSize)
{
        unsigned int addr = FlashAddr;
        unsigned int addr_end = (addr+DataSize);

        while (addr < addr_end) {
                Flash_Lock(addr, FLASH_FOUR);
                addr += FLASH_BLKSIZE;
        }
        return 0;
}
// TODO: modify this function to target-specific function - unlock flash
int IntelJ3_unlock(unsigned int FlashAddr, unsigned int DataSize)
{
        unsigned int addr=FlashAddr;
        unsigned int addr_end = (addr+DataSize);

        while (addr < addr_end) {
                Flash_UnLock(addr, FLASH_FOUR);
                addr += FLASH_BLKSIZE;
        }
        return 0;
}

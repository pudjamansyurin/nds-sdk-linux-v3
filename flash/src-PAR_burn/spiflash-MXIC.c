#include <sys/time.h>
#ifdef __MINGW32__
#include <winsock2.h>  // for Sleep()
#endif
#include "platform.h"
#include "util.h"
#include "flash-rom.h"

#define INDIVIDUAL_BLK_PROTECT     0
/*--------------------------------------------*/
/* SPI Flash ROM definition                   */
/*--------------------------------------------*/
#define SPIROM_ID_VERSION    0x1420c2  /* MXIC 25L8006  */
/* #define SPIROM_ID_VERSION   0x1520c2   MXIC 25L1606  */
#define SPIROM_ID_MASK       0x00ffffff
#define SPIROM_OP_READ       0x03        /*-- read --*/
#define SPIROM_OP_FAST_READ  0x0b        /*-- read in higher speed --*/
#define SPIROM_OP_RDID       0x9f        /*-- manufacturer/device ID read --*/
#define SPIROM_OP_READ_ID    0x90        /*-- manufacturer/device ID read --*/
#define SPIROM_OP_WREN       0x06        /*-- write enable --*/
#define SPIROM_OP_WRDI       0x04        /*-- write disable --*/
#define SPIROM_OP_SE         0x20        /*-- sector erase --*/
#define SPIROM_OP_BE         0x52        /*-- block erase --*/
#define SPIROM_OP_PP         0x02        /*-- page program --*/
#define SPIROM_OP_RDSR       0x05        /*-- read status register --*/
#define SPIROM_OP_WRSR       0x01        /*-- write status register --*/
#define SPIROM_OP_SBLK       0x36        /*-- single block lock --*/
#define SPIROM_OP_SBULK      0x39        /*-- single block unlock --*/
#define SPIROM_OP_RDBLOCK    0x3C
#define SPIROM_OP_RDSCUR     0x2B
#define SPIROM_OP_WPSEL      0x68
#define SPIROM_OP_GBLK       0x7E
#define SPIROM_OP_GBULK      0x98


/*-- bit mask for status register --*/
#define SPIROM_SR_WIP_MASK   0x01   /*-- write in progress --*/
#define SPIROM_SR_WEL_MASK   0x02   /*-- write enable --*/
#define SPIROM_SR_BP_MASK    0x3C   /*-- BP protection mode --*/
#define SPIROM_SECTOR_SIZE   0x1000 /*-- sector size in bytes, must be 4byte-aligned --*/
#define SPIROM_PAGE_SIZE     0x100  /*-- page size --*/
#define SPIROM_PAGEPROG_ADDR_MASK     0xFFFFFF00
/*-- SPI ROM cmd --*/
#define SPIROM_CMD_READ      0x0
#define SPIROM_CMD_RDID      0x1
#define SPIROM_CMD_RDST      0x2
#define SPIROM_CMD_WREN      0x3
#define SPIROM_CMD_WRDI      0x4
#define SPIROM_CMD_ERASE     0x5
#define SPIROM_CMD_PROGRAM   0x6
#define SPIROM_CMD_LOCK      0x7
#define SPIROM_CMD_UNLOCK    0x8
#define SPIROM_CMD_RDBLOCK   0x9
#define SPIROM_CMD_RDSCUR    0xA
#define SPIROM_CMD_WPSEL     0xB
#define SPIROM_CMD_CHIP_UNLOCK   0xC
#define SPIROM_CMD_WRSR      0xD

#define SPIROM_REAL_SIZE     0x100000

#define FLASH_BLKSIZE    SPIROM_SECTOR_SIZE
#define FLASH_CHIPSIZE   SPIROM_REAL_SIZE
#define FLASH_RETRY_TIMES    100

int mxic_check(void);
int mxic_program(unsigned int FlashAddr, unsigned char *start, unsigned int DataSize);
int mxic_erase(unsigned int FlashAddr, unsigned int DataSize);
int mxic_read(unsigned int FlashAddr, unsigned char *start, unsigned int DataSize);
int mxic_lock(unsigned int FlashAddr, unsigned int DataSize);
int mxic_unlock(unsigned int FlashAddr, unsigned int DataSize);
int mxic_set_wrsr(unsigned int uiStat);

flash_dev flash_MXIC = {
        mxic_check,
        mxic_erase,
        mxic_program,
        mxic_read,
        mxic_lock,
        mxic_unlock,
        FLASH_BLKSIZE,
        FLASH_CHIPSIZE
};

extern FILE *pLogFile;
//extern unsigned int guiFastMode;

unsigned int spirom_prepare_cmd(unsigned int cmd, unsigned int addr)
{
        unsigned int b0 = (cmd & 0xff);
        unsigned int b1 = (((addr >> 16) & 0xff) << 8);
        unsigned int b2 = (((addr >> 8) & 0xff) << 16);
        unsigned int b3 = ((addr & 0xff) << 24);
        unsigned int word = (b0 | b1 | b2 | b3);
        return word;
}

unsigned int spirom_cmd_send (
        unsigned int cmd,
        unsigned int addr,
        unsigned int bytes,
        unsigned int *pdata,
        unsigned int *Retdata)
{
        unsigned int i;
        unsigned int timeout = FLASH_RETRY_TIMES;
        unsigned int spib_dctrl;
        unsigned int op_addr;
        unsigned int data = 0;
        unsigned int spib_busy = 0;
        unsigned int spib_rx_empty;

        /*-- wait if there is active transaction --*/
        spib_busy = spib_wait_spi();
        if (spib_busy != 0) {
                printf("spirom_cmd_send: timeout\n");
                *Retdata = 0;
                return 1;
        }
        /*-- clear tx/rx fifo --*/
        spib_clr_fifo();

        /*-- prepare opcode and address --*/
        if (cmd == SPIROM_CMD_READ) {
                /*-- execute command --*/
                op_addr = spirom_prepare_cmd(SPIROM_OP_READ, addr);
                spib_dctrl = spib_prepare_dctrl (0x0, 0x0, SPIB_TM_WR_RD, 3, 0, bytes-1);
                spib_exe_cmmd(op_addr, spib_dctrl);

                spib_rx_data(pdata, bytes);
        } else if (cmd == SPIROM_CMD_WREN) {
                /*-- execute command --*/
                op_addr = SPIROM_OP_WREN;
                spib_dctrl = spib_prepare_dctrl (0x0, 0x0, SPIB_TM_WRonly, 0, 0, 0);
                spib_set_data(op_addr);        /*-- push flash command into tx fifo --*/
                spib_set_dctrl(spib_dctrl);    /*-- set dctrl --*/
                spib_set_cmd(0x0);             /*-- set dummy command to trigger transation start --*/

                /*-- wait completion --*/
                spib_busy = spib_wait_spi();
                if (spib_busy != 0) {
                        printf("spirom_cmd_send: WREN timeout\n");
                        *Retdata = 0;
                        return 1;
                }
        } else if (cmd == SPIROM_CMD_WRDI) {
                /*-- execute command --*/
                op_addr = SPIROM_OP_WRDI;
                spib_dctrl = spib_prepare_dctrl (0x0, 0x0, SPIB_TM_WRonly, 0, 0, 0);
                spib_set_data(op_addr);        /*-- push flash command into tx fifo --*/
                spib_set_dctrl(spib_dctrl);    /*-- set dctrl --*/
                spib_set_cmd(0x0);             /*-- set dummy command to trigger transation start --*/

                /*-- wait completion --*/
                spib_busy = spib_wait_spi();
                if (spib_busy != 0) {
                        printf("spirom_cmd_send: WRDI timeout\n");
                        *Retdata = 0;
                        return 1;
                }
        } else if (cmd == SPIROM_CMD_ERASE) {
                /*-- execute command --*/
                op_addr = spirom_prepare_cmd (SPIROM_OP_SE, addr);
                spib_dctrl = spib_prepare_dctrl (0x0, 0x0, SPIB_TM_WRonly, 3, 0, 0);
                spib_set_data(op_addr);        /*-- push flash command into tx fifo --*/
                spib_set_dctrl(spib_dctrl);    /*-- set dctrl --*/
                spib_set_cmd(0x0);             /*-- set dummy command to trigger transation start --*/

                /*-- wait completion --*/
                spib_busy = spib_wait_spi();
                if (spib_busy != 0) {
                        printf("spirom_cmd_send: ERASE timeout\n");
                        *Retdata = 0;
                        return 1;
                }
        } else if (cmd == SPIROM_CMD_PROGRAM) {
                /*-- execute command --*/
                op_addr = spirom_prepare_cmd (SPIROM_OP_PP, addr);
                spib_dctrl = spib_prepare_dctrl (0x0, 0x0, SPIB_TM_WRonly, 3+bytes, 0, 0);
                spib_exe_cmmd(op_addr, spib_dctrl);

                /*-- write data --*/
                spib_tx_data(pdata, bytes);
        } else if (cmd == SPIROM_CMD_RDID) {
                /*-- execute command --*/
                op_addr = SPIROM_OP_RDID;
                spib_dctrl = spib_prepare_dctrl (0x0, 0x0, SPIB_TM_WR_RD, 0, 0, 2);
                spib_set_data(op_addr);        /*-- push flash command into tx fifo --*/
                spib_set_dctrl(spib_dctrl);    /*-- set dctrl --*/
                spib_set_cmd(0x0);             /*-- set dummy command to trigger transation start --*/

                /*-- wait data completion --*/
                for (i = 1; i < timeout; i++) {
                        spib_rx_empty = spib_get_rx_empty ();
                        if (spib_rx_empty == 0)
                                break;
                }
                if (spib_rx_empty != 0) {
                        printf("spirom_cmd_send: RDID timeout\n");
                        *Retdata = 0;
                        return 1;
                }
                data = spib_get_data();
        } else if (cmd == SPIROM_CMD_RDST) {
                /*-- execute command --*/
                op_addr = SPIROM_OP_RDSR;
                spib_dctrl = spib_prepare_dctrl (0x0, 0x0, SPIB_TM_WR_RD, 0, 0, 0);
                spib_set_data(op_addr);        /*-- push flash command into tx fifo --*/
                spib_set_dctrl(spib_dctrl);    /*-- set dctrl --*/
                spib_set_cmd(0x0);             /*-- set dummy command to trigger transation start --*/

                /*-- wait data completion --*/
                for (i = 1; i < timeout; i++) {
                        spib_rx_empty = spib_get_rx_empty ();
                        if (spib_rx_empty == 0)
                                break;
                }
                if (spib_rx_empty != 0) {
                        printf("spirom_cmd_send: RDST timeout\n");
                        *Retdata = 0;
                        return 1;
                }
                data = spib_get_data();
                /*-- printf("data = %x\n", data) --*/
        } else if ((cmd == SPIROM_CMD_LOCK) || (cmd == SPIROM_CMD_UNLOCK)) {
                if (cmd == SPIROM_CMD_LOCK)
                        cmd = SPIROM_OP_SBLK;//SPIROM_OP_GBLK;
                else
                        cmd = SPIROM_OP_SBULK;//SPIROM_OP_GBULK;
                //printf("spirom_cmd_send: cmd=%x, addr=%x\n", cmd, addr);
                /*-- execute command --*/
                //op_addr = cmd;
                op_addr = spirom_prepare_cmd (cmd, addr);
                //spib_dctrl = spib_prepare_dctrl (0x0, 0x0, SPIB_TM_WRonly, 0, 0, 0);
                spib_dctrl = spib_prepare_dctrl (0x0, 0x0, SPIB_TM_WRonly, 3, 0, 0);
                spib_set_data(op_addr);        /*-- push flash command into tx fifo --*/
                spib_set_dctrl(spib_dctrl);    /*-- set dctrl --*/
                spib_set_cmd(0x0);             /*-- set dummy command to trigger transation start --*/

                /*-- wait completion --*/
                spib_busy = spib_wait_spi();
                if (spib_busy != 0) {
                        printf("spirom_cmd_send: lock/unlock timeout\n");
                        *Retdata = 0;
                        return 1;
                }
        } else if (cmd == SPIROM_CMD_RDBLOCK) {
                /*-- execute command --*/
                //op_addr = SPIROM_OP_RDBLOCK;
                op_addr = spirom_prepare_cmd (SPIROM_OP_RDBLOCK, addr);
                spib_dctrl = spib_prepare_dctrl (0x0, 0x0, SPIB_TM_WR_RD, 3, 0, 0);
                spib_set_data(op_addr);        /*-- push flash command into tx fifo --*/
                spib_set_dctrl(spib_dctrl);    /*-- set dctrl --*/
                spib_set_cmd(0x0);             /*-- set dummy command to trigger transation start --*/

                /*-- wait data completion --*/
                for (i = 1; i < timeout; i++) {
                        spib_rx_empty = spib_get_rx_empty ();
                        if (spib_rx_empty == 0)
                                break;
                }
                if (spib_rx_empty != 0) {
                        printf("spirom_cmd_send: RDBLOCK timeout\n");
                        *Retdata = 0;
                        return 1;
                }
                data = spib_get_data();
        } else if (cmd == SPIROM_CMD_RDSCUR) {
                /*-- execute command --*/
                op_addr = SPIROM_OP_RDSCUR;
                spib_dctrl = spib_prepare_dctrl (0x0, 0x0, SPIB_TM_WR_RD, 0, 0, 0);
                spib_set_data(op_addr);        /*-- push flash command into tx fifo --*/
                spib_set_dctrl(spib_dctrl);    /*-- set dctrl --*/
                spib_set_cmd(0x0);             /*-- set dummy command to trigger transation start --*/

                /*-- wait data completion --*/
                for (i = 1; i < timeout; i++) {
                        spib_rx_empty = spib_get_rx_empty ();
                        if (spib_rx_empty == 0)
                                break;
                }
                if (spib_rx_empty != 0) {
                        printf("spirom_cmd_send: RDSCUR timeout\n");
                        *Retdata = 0;
                        return 1;
                }
                data = spib_get_data();
        } else if (cmd == SPIROM_CMD_CHIP_UNLOCK) {
                op_addr = SPIROM_OP_GBULK;
                spib_dctrl = spib_prepare_dctrl (0x0, 0x0, SPIB_TM_WRonly, 0, 0, 0);
                spib_set_data(op_addr);
                spib_set_dctrl(spib_dctrl);
                spib_set_cmd(0x0);

                spib_busy = spib_wait_spi();
                if (spib_busy != 0) {
                        printf("spirom_cmd_send: GBULK timeout\n");
                        *Retdata = 0;
                        return 1;
                }
        }
#if INDIVIDUAL_BLK_PROTECT
        else if (cmd == SPIROM_CMD_WPSEL) {
                op_addr = SPIROM_OP_WPSEL;
                spib_dctrl = spib_prepare_dctrl (0x0, 0x0, SPIB_TM_WRonly, 0, 0, 0);
                spib_set_data(op_addr);
                spib_set_dctrl(spib_dctrl);
                spib_set_cmd(0x0);

                spib_busy = spib_wait_spi();
                if (spib_busy != 0) {
                        printf("spirom_cmd_send: WPSEL timeout\n");
                        *Retdata = 0;
                        return 1;
                }
        }
#endif
        else if (cmd == SPIROM_CMD_WRSR) {
                /*-- execute command --*/
                op_addr = (SPIROM_OP_WRSR | (addr<<8));
                spib_dctrl = spib_prepare_dctrl (0x0, 0x0, SPIB_TM_WRonly, 1, 0, 0);
                spib_set_data(op_addr);        /*-- push flash command into tx fifo --*/
                spib_set_dctrl(spib_dctrl);    /*-- set dctrl --*/
                spib_set_cmd(0x0);             /*-- set dummy command to trigger transation start --*/

                /*-- wait completion --*/
                spib_busy = spib_wait_spi();
                if (spib_busy != 0) {
                        printf("spirom_cmd_send: ERASE timeout\n");
                        *Retdata = 0;
                        return 1;
                }
        } else {
                printf("spirom_cmd_send: wrong cmd\n");
                *Retdata = 0;
                return 1;
        }

        *Retdata = data;
        return 0;
}
#if INDIVIDUAL_BLK_PROTECT
unsigned int spirom_WPSEL_command(void)
{
        unsigned int RetData;

        spirom_cmd_send (SPIROM_CMD_RDSCUR, 0x0, 0, NULL, &RetData);
        //printf("spirom_WPSEL_command-0, %x \n", RetData);
        if (RetData & 0x80)
                return 0;

        /*-- write enable --*/
        spirom_cmd_send (SPIROM_CMD_WREN, 0x0, 0, NULL, &RetData);

        spirom_cmd_send (SPIROM_CMD_WPSEL, 0x0, 0, NULL, &RetData);
        /*-- get enable status --*/
        while (1) {
                spirom_cmd_send (SPIROM_CMD_RDST, 0x0, 0, NULL, &RetData);
                if ((RetData & SPIROM_SR_WIP_MASK) == 0)
                        break;
        }
        spirom_cmd_send (SPIROM_CMD_RDSCUR, 0x0, 0, NULL, &RetData);
        //printf("spirom_WPSEL_command-1, %x \n", RetData);
        if (RetData & 0x80)
                return 0;
        else
                return 1;
}
#endif

int mxic_unlock_chip(void)
{
        unsigned int j, RetData, timeout=FLASH_RETRY_TIMES;
        /*-- write enable --*/
        spirom_cmd_send (SPIROM_CMD_WREN, 0x0, 0, NULL, &RetData);

        /*-- get enable status --*/
        for (j = 1; j < timeout; j++) {
                spirom_cmd_send (SPIROM_CMD_RDST, 0x0, 0, NULL, &RetData);
                if ((RetData & SPIROM_SR_WIP_MASK) == 0)
                        break;
        }
        if ((RetData & SPIROM_SR_WEL_MASK) == 0)
                printf("mxic_unlock_chip: (ULock) status %x, write enable is not set\n", RetData);

        /*-- Chip-UnLock --*/
        spirom_cmd_send (SPIROM_CMD_CHIP_UNLOCK, 0x0, 0, NULL, &RetData);

        /*-- get ULock status --*/
        for (j = 1; j < timeout; j++) {
                spirom_cmd_send (SPIROM_CMD_RDST, 0x0, 0, NULL, &RetData);
                if ((RetData & SPIROM_SR_WIP_MASK) == 0)
                        break;
        }
        if ((RetData & SPIROM_SR_WIP_MASK) != 0) {
                printf("mxic_unlock_chip: status %x, ULock is still in progress\n", RetData);
                return 1;
        }
        spirom_cmd_send (SPIROM_CMD_RDBLOCK, 0, 0, NULL, &RetData);

        if ((RetData & 0xFF) == 0xFF) {
                printf("mxic_unlock_chip: ULock-chip fail\n");
                return 1;
        }
        return 0;
}
// TODO: modify this function to target-specific function - check flash
int mxic_check(void)
{
        unsigned int result, RetData;
        unsigned int cpu_ver_value;
        /* mark fast-mode
        unsigned int SCLK_DIV;
        if (guiFastMode == 0)
            SCLK_DIV=0xC0;
        else
            SCLK_DIV=0x01;

        RetData = (spib_get_regtiming() & (~0xFF));
        spib_set_regtiming(RetData|SCLK_DIV);
        SCLK_DIV = spib_get_regtiming();
        //printf("mxic_check: SCLK_DIV=%x\n", SCLK_DIV);
        */

        // Set SCLK period = ((0+1)*2)*(Period of the SPI clock source)
        cpu_ver_value = inr(0x2A);
        RetData = (spib_get_regtiming() & (~0xFF));
        switch ( (cpu_ver_value&0xFF000000) >> 24 ) {
                case 0x06:	//N650
                        spib_set_regtiming(RetData|0x0);
                        break;
                default:
                        spib_set_regtiming(RetData|0x1);
                        break;
        };
        printf("mxic_check: REGTIMING=%x\n", spib_get_regtiming());

        result = spirom_cmd_send (SPIROM_CMD_RDID, 0x0, 0, NULL, &RetData);
        if (result != 0) {
                printf("ERROR: read spi rom id fail\n");
                return 1;
        }
        printf("MXIC: ROM ID = 0x%08x \n", RetData);

#if (INDIVIDUAL_BLK_PROTECT==0)
        // check if "WPSEL"==1, then do chip-unlock
        spirom_cmd_send (SPIROM_CMD_RDSCUR, 0x0, 0, NULL, &RetData);
        //printf("WPSEL = %x \n", RetData);
        if (RetData & 0x80) {
                mxic_unlock_chip();
        }
#endif
        return 0;
}
// TODO: modify this function to target-specific function - erase flash
int mxic_erase(unsigned int FlashAddr, unsigned int DataSize)
{
        unsigned int EraseAddrStart, EraseSize, EraseSectorCnt, EraseSectorIndex, i, j;
        unsigned int result, RetData, timeout=FLASH_RETRY_TIMES;

        EraseAddrStart = (FlashAddr / SPIROM_SECTOR_SIZE) * SPIROM_SECTOR_SIZE;
        EraseSize = (FlashAddr - EraseAddrStart) + DataSize;
        EraseSectorCnt = (EraseSize + (SPIROM_SECTOR_SIZE - 1)) / SPIROM_SECTOR_SIZE;
        EraseSectorIndex = (EraseAddrStart / SPIROM_SECTOR_SIZE);

        for (i = 0; i < EraseSectorCnt; i++) {
                /*---------------------*/
                /*-- ERASE procedure   */
                /*---------------------*/
                for (j = 1; j < timeout; j++) {
                        //if (j >= 2)
                        //  printf("mxic_erase: retry-WREN\n");
                        /*-- write enable --*/
                        result = spirom_cmd_send (SPIROM_CMD_WREN, 0x0, 0, NULL, &RetData);
                        if (result != 0) {
                                printf("mxic_erase: (erase) enable write fail\n");
                                return 1;
                        }
                        /*-- get enable status --*/
                        result = spirom_cmd_send (SPIROM_CMD_RDST, 0x0, 0, NULL, &RetData);
                        if (result != 0) {
                                printf("mxic_erase: (erase) get write enable status fail\n");
                                return 1;
                        }
                        if (RetData & SPIROM_SR_BP_MASK) {
                                printf("mxic_erase: Flash block locked.\n");
                                return 1;
                        } else if (RetData & SPIROM_SR_WEL_MASK)
                                break;
                }
                if ((RetData & SPIROM_SR_WEL_MASK) == 0)
                        printf("mxic_erase: (erase) status %x, write enable is not set\n", RetData);

#if INDIVIDUAL_BLK_PROTECT
                spirom_cmd_send (SPIROM_CMD_RDBLOCK, EraseAddrStart, 0, NULL, &RetData);
                //printf("mxic_erase: addr=%x, RDBLOCK=%x \n", EraseAddrStart, RetData);
                if ((RetData & 0xFF) == 0xFF) {
                        printf("\nmxic_erase - Flash block locked.\n");
                        return 1;
                }
#endif
                fprintf(pLogFile, "erasing block %03d (0x%06x ~ 0x%06x)\n", EraseSectorIndex, EraseAddrStart, EraseAddrStart + SPIROM_SECTOR_SIZE);
                fflush(pLogFile);

                /*-- erase --*/
                result = spirom_cmd_send (SPIROM_CMD_ERASE, EraseAddrStart, 0, NULL, &RetData);
                if (result != 0) {
                        printf("mxic_erase: rom erase fail\n");
                        return 1;
                }

                /*-- get erase status --*/
                for (j = 1; j < timeout; j++) {
                        //if (j >= 2)
                        //  printf("mxic_erase: retry-RDST\n");
                        result = spirom_cmd_send (SPIROM_CMD_RDST, 0x0, 0, NULL, &RetData);
                        if (result != 0) {
                                printf("mxic_erase: get erase status fail\n");
                                return 1;
                        }
                        if ((RetData & SPIROM_SR_WIP_MASK) == 0)
                                break;
                }
                if ((RetData & SPIROM_SR_WIP_MASK) != 0) {
                        printf("mxic_erase: status %x, erase is still in progress\n", RetData);
                        return 1;
                }
                EraseAddrStart += SPIROM_SECTOR_SIZE;
                EraseSectorIndex++;
        }
        return 0;
}
// TODO: modify this function to target-specific function - program flash
int mxic_program(unsigned int FlashAddr, unsigned char *start, unsigned int DataSize)
{
        unsigned int result, RetData, PageCnt, addr, i;
        unsigned int j, timeout=FLASH_RETRY_TIMES;
        unsigned int buffer_size = SPIROM_PAGE_SIZE;

        if (FlashAddr & (~SPIROM_PAGEPROG_ADDR_MASK)) {
                fprintf(pLogFile, "mxic_program: addr is NOT page-align!!");
                fflush(pLogFile);
                return 1;
        }
        PageCnt = (DataSize + (SPIROM_PAGE_SIZE - 1)) / SPIROM_PAGE_SIZE;

        /*---------------------------*/
        /*-- PAGE PROGRAM procedure  */
        /*---------------------------*/
        for (i = 0; i < PageCnt; i++) {
                /*-- write enable --*/
                for (j = 1; j < timeout; j++) {
                        //if (j >= 2)
                        //  printf("mxic_program: retry-WREN\n");
                        result = spirom_cmd_send (SPIROM_CMD_WREN, 0x0, 0, NULL, &RetData);
                        if (result != 0) {
                                printf("mxic_program: (program) page %d enable write fail\n", i);
                                return 1;
                        }
                        /*-- get enable status --*/
                        result = spirom_cmd_send (SPIROM_CMD_RDST, 0x0, 0, NULL, &RetData);
                        if (result != 0) {
                                printf("mxic_program: get (program) page %d write enable status fail\n", i);
                                return 1;
                        }
                        if (RetData & SPIROM_SR_BP_MASK) {
                                printf("mxic_program: Flash block locked.\n");
                                return 1;
                        } else if (RetData & SPIROM_SR_WEL_MASK)
                                break;
                }
                if ((RetData & SPIROM_SR_WEL_MASK) == 0) {
                        printf("mxic_program: (program) page %d, write enable is not set (status %x)\n", i, RetData);
                        return 1;
                }

                addr = FlashAddr + (i * SPIROM_PAGE_SIZE);

                if ( (DataSize-(i*SPIROM_PAGE_SIZE)) > SPIROM_PAGE_SIZE )
                        buffer_size = SPIROM_PAGE_SIZE;
                else
                        buffer_size = DataSize - (i * SPIROM_PAGE_SIZE);

                result = spirom_cmd_send (SPIROM_CMD_PROGRAM, addr, buffer_size, (unsigned int*)start, &RetData);
                if (result != 0) {
                        printf("mxic_program: (program) page %d fail\n", i);
                        return 1;
                }
                /*-- ckeck completion --*/
                for (j = 1; j < timeout; j++) {
                        //if (j >= 2)
                        //  printf("mxic_program: retry-RDST\n");
                        result = spirom_cmd_send (SPIROM_CMD_RDST, 0x0, 0, NULL, &RetData);
                        if (result != 0) {
                                printf("mxic_program: get (program) page %d write enable status fail\n", i);
                                return 1;
                        }
                        if ((RetData & SPIROM_SR_WIP_MASK) == 0)
                                break;
                }
                if ((RetData & SPIROM_SR_WEL_MASK) != 0) {
                        printf("mxic_program: (program) page %d, write enable is not clear (status %x)\n", i, RetData);
                        return 1;
                }

                if ( i%(8*1024/SPIROM_PAGE_SIZE) == 0 ) {	// IDE use 8K/dot
                        fprintf(pLogFile, ".");
                        fflush(pLogFile);
                }
                start += SPIROM_PAGE_SIZE;
        }  /* for (i = 0; i < PageCnt; i++) */
        return 0;
}
// TODO: modify this function to target-specific function - read flash
int mxic_read(unsigned int FlashAddr, unsigned char *start, unsigned int DataSize)
{
        unsigned int result, RetData, CurrSize;
        /*-- SPIB_DCTRL_RCNT_MASK(0x1ff) --*/
        while (DataSize) {
                if (DataSize >= 0x200)
                        CurrSize = 0x200;
                else
                        CurrSize = DataSize;
                result = spirom_cmd_send (SPIROM_CMD_READ, FlashAddr, CurrSize, (unsigned int*)start, &RetData);
                if (result != 0) {
                        printf("Flash_Read: fail\n");
                        return 1;
                }
                FlashAddr += CurrSize;
                start += CurrSize;
                DataSize -= CurrSize;
        }
        return 0;
}
// TODO: modify this function to target-specific function - lock flash
int mxic_lock(unsigned int FlashAddr, unsigned int DataSize)
{
#if INDIVIDUAL_BLK_PROTECT
        unsigned int LockAddrStart, LockSize, LockSectorCnt, LockSectorIndex;
        unsigned int RetData, timeout=FLASH_RETRY_TIMES, i, j;

        LockAddrStart = (FlashAddr / SPIROM_SECTOR_SIZE) * SPIROM_SECTOR_SIZE;
        LockSize = (FlashAddr - LockAddrStart) + DataSize;
        LockSectorCnt = (LockSize + (SPIROM_SECTOR_SIZE - 1)) / SPIROM_SECTOR_SIZE;
        LockSectorIndex = (LockAddrStart / SPIROM_SECTOR_SIZE);

        spirom_WPSEL_command();
        for (i = 0; i < LockSectorCnt; i++) {
                /*---------------------*/
                /*-- Lock procedure   */
                /*---------------------*/
                /*-- write enable --*/
                spirom_cmd_send (SPIROM_CMD_WREN, 0x0, 0, NULL, &RetData);

                /*-- get enable status --*/
                for (j = 1; j < timeout; j++) {
                        spirom_cmd_send (SPIROM_CMD_RDST, 0x0, 0, NULL, &RetData);
                        if ((RetData & SPIROM_SR_WIP_MASK) == 0)
                                break;
                }
                if ((RetData & SPIROM_SR_WEL_MASK) == 0)
                        printf("mxic_lock: (Lock) status %x, write enable is not set\n", RetData);

                /*-- lock --*/
                spirom_cmd_send (SPIROM_CMD_LOCK, LockAddrStart, 0, NULL, &RetData);

                /*-- get lock status --*/
                for (j = 1; j < timeout; j++) {
                        spirom_cmd_send (SPIROM_CMD_RDST, 0x0, 0, NULL, &RetData);
                        if ((RetData & SPIROM_SR_WIP_MASK) == 0)
                                break;
                }
                if ((RetData & SPIROM_SR_WIP_MASK) != 0) {
                        printf("mxic_lock: status %x, Lock is still in progress\n", RetData);
                        return 1;
                }
                spirom_cmd_send (SPIROM_CMD_RDBLOCK, LockAddrStart, 0, NULL, &RetData);
                if ((RetData & 0xFF) != 0xFF) {
                        printf("mxic_lock: Lock fail %x\n", RetData);
                        return 1;
                }
                LockAddrStart += SPIROM_SECTOR_SIZE;
                LockSectorIndex++;
        }
#else
        unsigned int RetData=0;

        spirom_cmd_send (SPIROM_CMD_RDST, 0x0, 0, NULL, &RetData);
        mxic_set_wrsr(RetData | 0x3C);
        spirom_cmd_send (SPIROM_CMD_RDST, 0x0, 0, NULL, &RetData);
        printf("mxic_lock: status = %08x \n", RetData);
#endif
        return 0;
}
// TODO: modify this function to target-specific function - unlock flash
int mxic_unlock(unsigned int FlashAddr, unsigned int DataSize)
{
#if INDIVIDUAL_BLK_PROTECT
        unsigned int ULockAddrStart, ULockSize, ULockSectorCnt, ULockSectorIndex;
        unsigned int RetData, timeout=FLASH_RETRY_TIMES, i, j;

        ULockAddrStart = (FlashAddr / SPIROM_SECTOR_SIZE) * SPIROM_SECTOR_SIZE;
        ULockSize = (FlashAddr - ULockAddrStart) + DataSize;
        ULockSectorCnt = (ULockSize + (SPIROM_SECTOR_SIZE - 1)) / SPIROM_SECTOR_SIZE;
        ULockSectorIndex = (ULockAddrStart / SPIROM_SECTOR_SIZE);

        spirom_WPSEL_command();
        for (i = 0; i < ULockSectorCnt; i++) {
                /*---------------------*/
                /*-- ULock procedure   */
                /*---------------------*/
                /*-- write enable --*/
                spirom_cmd_send (SPIROM_CMD_WREN, 0x0, 0, NULL, &RetData);

                /*-- get enable status --*/
                for (j = 1; j < timeout; j++) {
                        spirom_cmd_send (SPIROM_CMD_RDST, 0x0, 0, NULL, &RetData);
                        if ((RetData & SPIROM_SR_WIP_MASK) == 0)
                                break;
                }
                if ((RetData & SPIROM_SR_WEL_MASK) == 0)
                        printf("mxic_unlock: (ULock) status %x, write enable is not set\n", RetData);

                /*-- ULock --*/
                spirom_cmd_send (SPIROM_CMD_UNLOCK, ULockAddrStart, 0, NULL, &RetData);

                /*-- get ULock status --*/
                for (j = 1; j < timeout; j++) {
                        spirom_cmd_send (SPIROM_CMD_RDST, 0x0, 0, NULL, &RetData);
                        if ((RetData & SPIROM_SR_WIP_MASK) == 0)
                                break;
                }
                if ((RetData & SPIROM_SR_WIP_MASK) != 0) {
                        printf("mxic_unlock: status %x, ULock is still in progress\n", RetData);
                        return 1;
                }
                spirom_cmd_send (SPIROM_CMD_RDBLOCK, ULockAddrStart, 0, NULL, &RetData);
                //printf("mxic_unlock: addr=%x, RDBLOCK=%x \n", ULockAddrStart, RetData);
                if ((RetData & 0xFF) == 0xFF) {
                        printf("mxic_unlock: ULock fail\n");
                        return 1;
                }
                //spirom_cmd_send (SPIROM_CMD_RDSCUR, 0x0, 0, NULL, &RetData);
                //printf("mxic_unlock: RDSCUR %x \n", RetData);

                ULockAddrStart += SPIROM_SECTOR_SIZE;
                ULockSectorIndex++;
        }
#else
        unsigned int RetData=0;

        spirom_cmd_send (SPIROM_CMD_RDST, 0x0, 0, NULL, &RetData);
        mxic_set_wrsr(RetData & ~0x3C);
        spirom_cmd_send (SPIROM_CMD_RDST, 0x0, 0, NULL, &RetData);
        printf("mxic_unlock: status = %08x \n", RetData);
#endif
        return 0;
}

int mxic_set_wrsr(unsigned int uiStat)
{
        unsigned int i, j, RetData, timeout=FLASH_RETRY_TIMES;

        for (i = 1; i < timeout; i++)  {
                /*-- write enable --*/
                spirom_cmd_send (SPIROM_CMD_WREN, 0x0, 0, NULL, &RetData);
                /*-- get enable status --*/
                for (j = 1; j < timeout; j++)  {
                        spirom_cmd_send (SPIROM_CMD_RDST, 0x0, 0, NULL, &RetData);
                        if ((RetData & SPIROM_SR_WIP_MASK) == 0)
                                break;
                }
                if (RetData & SPIROM_SR_WEL_MASK)
                        break;
        }
        /*-- set WRSR --*/
        spirom_cmd_send (SPIROM_CMD_WRSR, uiStat, 0, NULL, &RetData);

        /*-- get status --*/
        for (j = 1; j < timeout; j++)  {
                spirom_cmd_send (SPIROM_CMD_RDST, 0x0, 0, NULL, &RetData);
                if ((RetData & SPIROM_SR_WIP_MASK) == 0)
                        break;
        }
        //printf("mxic_set_wrsr: uiStat=%x RetData=%x\n", uiStat, RetData);
        return 0;
}

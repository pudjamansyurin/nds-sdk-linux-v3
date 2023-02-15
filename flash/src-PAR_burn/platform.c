#include "platform.h"
#include "util.h"

extern unsigned int guiWithMultiout;

#define REG_SMU_BASE        0xF0100000
#define CPE_SPIB_BASE       0xF0B00000
#define REG_SMU_BASE_16     0x00F01000
#define CPE_SPIB_BASE_16    0x00F0B000

#define SMU_SYSID_AE100         0x41451
#define SMU_SYSID_AE210_16MB    0x41452
#define SMU_SYSID_AE210_4GB     0x41452
#define SMU_SYSID_AE300_4GB     0x41453
#define SMU_SYSID_AE250_4GB     0x41452500
#define SMU_SYSID_AE350_4GB     0x41453500

#define SPI_TX_FIFO    256  // bytes
#define SPI_RX_FIFO    256  // bytes

#define MEMMAP_AE100        0
#define MEMMAP_AE210_16MB   1
#define MEMMAP_AE210_4GB    2
#define MEMMAP_AE300_4GB    3
#define MEMMAP_AE250_4GB    4
#define MEMMAP_AE350_4GB    5
#define MEMMAP_MAX          6

unsigned int memmapping_spib_base[MEMMAP_MAX] = {
        CPE_SPIB_BASE_16,       // AE100
        CPE_SPIB_BASE_16,       // AE210_16MB
        CPE_SPIB_BASE,          // AE210_4GB
        CPE_SPIB_BASE,          // AE300
        CPE_SPIB_BASE,          // AE250_4GB
        CPE_SPIB_BASE,          // AE350_4GB
};

unsigned int memmapping_smu_base[MEMMAP_MAX] = {
        REG_SMU_BASE_16,        // AE100
        REG_SMU_BASE_16,        // AE210_16MB
        REG_SMU_BASE,           // AE210_4GB
        REG_SMU_BASE,           // AE300
        REG_SMU_BASE,           // AE250_4GB
        REG_SMU_BASE,           // AE350_4GB
};

unsigned int memmapping_system_id[MEMMAP_MAX] = {
        SMU_SYSID_AE100,        // AE100
        SMU_SYSID_AE210_16MB,   // AE210_16MB
        SMU_SYSID_AE210_4GB,    // AE210_4GB
        SMU_SYSID_AE300_4GB,    // AE300
        SMU_SYSID_AE250_4GB>>12,// AE250_4GB
        SMU_SYSID_AE350_4GB>>12,// AE350_4GB
};

const char* memmapping_target_name[MEMMAP_MAX] = {
        "AE100",
        "AE210 with 16MB",
        "AE210 with 4GB",
        "AE300",
        "AE250 with 4GB",
        "AE350 with 4GB",
};

/* Fast memory access with a constant address, EDM_CFG >= 0x1011 only */
unsigned int guiConstFastMode = 0;
unsigned int guiUserDefConstFastMode = 0;

unsigned int platform_init(void);
unsigned int platform_get_version_id(void);
unsigned int spib_ctrl = 0;
unsigned int spib_base = 0x0;
unsigned int mem_mapping_mode = MEMMAP_AE210_16MB;

/* for 24 bit platform */
unsigned int platform_init(void)
{
        platform_get_version_id();
        spib_base = memmapping_spib_base[mem_mapping_mode];
        spib_ctrl = spib_get_ctrl();
        return spib_base;
}

extern FILE *pLogFile;
unsigned int platform_get_version_id(void)
{
        unsigned int id1=0, EDM_cfg=0, EDM_version=0;
        unsigned int i, smu_base;
        unsigned int smu_id_reg = 0;

        for (i = 0; i < MEMMAP_MAX; i++) {
                smu_base = memmapping_smu_base[i];
                smu_id_reg = inw(smu_base + 0x00);
                id1 = (smu_id_reg & 0xFFFFF000) >> 12;
                // Check SMU System ID
                if (id1 == memmapping_system_id[i]) {
                        mem_mapping_mode = i;
                        break;
                }
        }
        if ( i == MEMMAP_MAX ) {
                fprintf(stderr, "Failed to find SMU ID\n");
                fprintf(pLogFile, "Failed to find SMU ID\n");
                fflush(pLogFile);
                terminate();
                exit(-1);
        }
        if (smu_id_reg == SMU_SYSID_AE250_4GB) {
                // V5 orca AE250
                mem_mapping_mode = MEMMAP_AE250_4GB;
                guiWithMultiout = 0;
                if (guiUserDefConstFastMode == 0)
                	guiConstFastMode = 1;
                printf("SMU_VER_ID = 0x%08x \n", smu_id_reg);
                printf("Target = %s\n", memmapping_target_name[mem_mapping_mode]);
                return 0;
        }
        if (smu_id_reg == SMU_SYSID_AE350_4GB) {
                // V5 orca AE350
                mem_mapping_mode = MEMMAP_AE350_4GB;
                guiWithMultiout = 0;
                if (guiUserDefConstFastMode == 0)
                	guiConstFastMode = 1;
                printf("SMU_VER_ID = 0x%08x \n", smu_id_reg);
                printf("Target = %s\n", memmapping_target_name[mem_mapping_mode]);
                return 0;
        }
        printf("SMU_VER_ID = 0x%08x \n", smu_id_reg);
        printf("Target = %s\n", memmapping_target_name[mem_mapping_mode]);

        read_edm_cfg(&EDM_cfg);
        printf("EDM_cfg = 0x%08x \n", EDM_cfg);
        EDM_version = (EDM_cfg & 0xFFFF0000) >> 16;
        if (EDM_version >= 0x1011)
                guiConstFastMode = 1;
        return 0;
}

/* ae100, ae300, ae250 and ae350: restore IVB to flash */
/* ae210p: no need restore_ivb because loading the flash data to ILM on ae210p must do power-on(power-on will reset SMU) */
unsigned int restore_ivb(void)
{
    if (mem_mapping_mode == MEMMAP_AE100) {
        fprintf(pLogFile, "restore SMU IVB to 0x800000\n");
        fflush(pLogFile);
        outw(REG_SMU_BASE_16 + 0x24, 0x800000);
    } else if (mem_mapping_mode == MEMMAP_AE300_4GB || mem_mapping_mode == MEMMAP_AE250_4GB || mem_mapping_mode == MEMMAP_AE350_4GB) {
        fprintf(pLogFile, "restore SMU IVB to 0x80000000\n");
        fflush(pLogFile);
        outw(REG_SMU_BASE + 0x50, 0x80000000);
    }
    return 0;
}

/*===========================================*/
/*   SPI driver                              */
/*===========================================*/

/*======================================================*/
/* SPIB register definition  */
/*======================================================*/
#define SPIB_REG_VER        (spib_base+0x0)
#define SPIB_REG_IFSET      (spib_base+0x10)
#define SPIB_REG_PIO        (spib_base+0x14)
#define SPIB_REG_DCTRL      (spib_base+0x20)
#define SPIB_REG_CMD        (spib_base+0x24)
#define SPIB_REG_ADDR       (spib_base+0x28)
#define SPIB_REG_DATA       (spib_base+0x2c)
#define SPIB_REG_CTRL       (spib_base+0x30)
#define SPIB_REG_FIFOST     (spib_base+0x34)
#define SPIB_REG_INTEN      (spib_base+0x38)
#define SPIB_REG_INTST      (spib_base+0x3c)
#define SPIB_REG_REGTIMING  (spib_base+0x40)
/*-- Data Control Reg --*/
#define SPIB_DCTRL_CMDEN_MASK       0x40000000
#define SPIB_DCTRL_ADDREN_MASK      0x20000000
#define SPIB_DCTRL_TRAMODE_MASK     0x0f000000
#define SPIB_DCTRL_WCNT_MASK        0x001ff000
#define SPIB_DCTRL_DYCNT_MASK       0x00000600
#define SPIB_DCTRL_RCNT_MASK        0x000001ff
#define SPIB_DCTRL_CMDEN_OFFSET     30
#define SPIB_DCTRL_ADDREN_OFFSET    29
#define SPIB_DCTRL_TRAMODE_OFFSET   24
#define SPIB_DCTRL_WCNT_OFFSET      12
#define SPIB_DCTRL_DYCNT_OFFSET     9
#define SPIB_DCTRL_RCNT_OFFSET      0
/*-- Control Reg --*/
#define SPIB_CTRL_TXFRST_MASK       0x00000004
#define SPIB_CTRL_RXFRST_MASK       0x00000002
#define SPIB_CTRL_SPIRST_MASK       0x00000001
/*-- FIFO Status Reg --*/
#define SPIB_FIFOST_TXFFL_MASK      0x00800000
#define SPIB_FIFOST_TXFEM_MASK      0x00400000
#define SPIB_FIFOST_TXFVE_MASK      0x001f0000
#define SPIB_FIFOST_RXFFL_MASK      0x00008000
#define SPIB_FIFOST_RXFEM_MASK      0x00004000
#define SPIB_FIFOST_RXFVE_MASK      0x00001f00
#define SPIB_FIFOST_SPIBSY_MASK     0x00000001
#define SPIB_FIFOST_TXFFL_OFFSET    23
#define SPIB_FIFOST_TXFEM_OFFSET    22
#define SPIB_FIFOST_TXFVE_OFFSET    16
#define SPIB_FIFOST_RXFFL_OFFSET    15
#define SPIB_FIFOST_RXFEM_OFFSET    14
#define SPIB_FIFOST_RXFVE_OFFSET    8
#define SPIB_FIFOST_SPIBSY_OFFSET   0
#define SPIB_FIFOST_SPIBSYnRXFEM    (SPIB_FIFOST_RXFEM_MASK|SPIB_FIFOST_SPIBSY_MASK)
/*-- SPIB transfer mode--*/
/*
#define SPIB_TM_WRsim               0x0
#define SPIB_TM_WRonly              0x1
#define SPIB_TM_RDonly              0x2
#define SPIB_TM_WR_RD               0x3
#define SPIB_TM_RD_WR               0x4
#define SPIB_TM_WR_DY_RD            0x5
#define SPIB_TM_RD_DY_WR            0x6

#define SPIB_VERSION                0x02002000
*/
/*--------------------------------------------*/
/* SPIB function                              */
/*--------------------------------------------*/
unsigned int spib_get_ifset (void)
{
        unsigned int reg = inw(SPIB_REG_IFSET);
        return reg;
}

void spib_set_ifset (unsigned int reg)
{
        outw(SPIB_REG_IFSET, reg);
}

unsigned int spib_get_pio (void)
{
        unsigned int reg = inw(SPIB_REG_PIO);
        return reg;
}

void spib_set_pio (unsigned int reg)
{
        outw(SPIB_REG_PIO, reg);
}

unsigned int spib_get_ctrl (void)
{
        unsigned int reg = inw(SPIB_REG_CTRL);
        return reg;
}

void spib_set_ctrl (unsigned int reg)
{
        outw(SPIB_REG_CTRL, reg);
}

unsigned int spib_get_fifost (void)
{
        unsigned int reg = inw(SPIB_REG_FIFOST);
        return reg;
}

unsigned int spib_get_inten (void)
{
        unsigned int reg = inw(SPIB_REG_INTEN);
        return reg;
}

void spib_set_inten (unsigned int reg)
{
        outw(SPIB_REG_INTEN, reg);
}

unsigned int spib_get_intst (void)
{
        unsigned int reg = inw(SPIB_REG_INTST);
        return reg;
}

void spib_set_intst (unsigned int reg)
{
        outw(SPIB_REG_INTST, reg);
}

unsigned int spib_get_dctrl (void)
{
        unsigned int reg = inw(SPIB_REG_DCTRL);
        /*check_timeout("read SPIB_REG_DCTRL failed");*/
        return reg;
}

void spib_set_dctrl (unsigned int reg)
{
        outw(SPIB_REG_DCTRL, reg);
}

unsigned int spib_get_cmd(void)
{
        return inw(SPIB_REG_CMD);
}

void spib_set_cmd(unsigned int cmd)
{
        outw(SPIB_REG_CMD, cmd);
}

unsigned int spib_get_addr(void)
{
        return inw(SPIB_REG_ADDR);
}

void spib_set_addr(unsigned int addr)
{
        outw(SPIB_REG_ADDR, addr);
}

unsigned int spib_get_data(void)
{
        return inw(SPIB_REG_DATA);
}

void spib_set_data(unsigned int data)
{
        outw(SPIB_REG_DATA, data);
}

unsigned int spib_get_regtiming(void)
{
        return inw(SPIB_REG_REGTIMING);
}

void spib_set_regtiming(unsigned int data)
{
        outw(SPIB_REG_REGTIMING, data);
}

unsigned int spib_prepare_dctrl(
        unsigned int cmden,
        unsigned int addren,
        unsigned int tm,
        unsigned int wcnt,
        unsigned int dycnt,
        unsigned int rcnt)
{
        unsigned int v[8];
        unsigned int i;
        unsigned int dctrl = 0x0;

        v[0] = ((cmden << SPIB_DCTRL_CMDEN_OFFSET) & SPIB_DCTRL_CMDEN_MASK);
        v[1] = ((addren << SPIB_DCTRL_ADDREN_OFFSET) & SPIB_DCTRL_ADDREN_MASK);
        v[2] = ((tm << SPIB_DCTRL_TRAMODE_OFFSET) & SPIB_DCTRL_TRAMODE_MASK);
        v[3] = ((wcnt << SPIB_DCTRL_WCNT_OFFSET) & SPIB_DCTRL_WCNT_MASK);
        v[4] = ((dycnt << SPIB_DCTRL_DYCNT_OFFSET) & SPIB_DCTRL_DYCNT_MASK);
        v[5] = ((rcnt << SPIB_DCTRL_RCNT_OFFSET) & SPIB_DCTRL_RCNT_MASK);

        for (i = 0; i < 6; i++)
                dctrl |= v[i];
        //printf("dctrl = %x\n", dctrl);
        return dctrl;
}

unsigned int spib_get_version (void)
{
        unsigned int reg = inw(SPIB_REG_VER);
        return reg;
}

unsigned int spib_get_busy (void)
{
        unsigned int reg = inw(SPIB_REG_FIFOST);
        return (reg & SPIB_FIFOST_SPIBSY_MASK);
}

unsigned int spib_wait_spi (void)
{
        unsigned int i;
        unsigned int timeout = 100;

        for (i = 1; i < timeout; i++) {
                if (spib_get_busy () == 0)
                        return 0;
        }
        printf("spib_wait_spi: timeout\n");
        return 1;
}

unsigned int spib_get_rx_empty (void)
{
        unsigned int reg = inw(SPIB_REG_FIFOST);
        return (reg & SPIB_FIFOST_RXFEM_MASK);
}

unsigned int spib_get_rx_entries (void)
{
        unsigned int reg = inw(SPIB_REG_FIFOST);
        unsigned int RetData;

        RetData = ((reg & SPIB_FIFOST_RXFVE_MASK) >> SPIB_FIFOST_RXFVE_OFFSET);
        return (RetData);
}

void spib_clr_fifo (void)
{
        //unsigned int spib_ctrl = inw(SPIB_REG_CTRL);
        spib_ctrl |= (SPIB_CTRL_TXFRST_MASK | SPIB_CTRL_RXFRST_MASK);
        spib_set_ctrl(spib_ctrl);
}

void spib_exe_cmmd (unsigned int op_addr, unsigned int spib_dctrl)
{
        unsigned int *addr_entry;
        unsigned int *data_entry;

        /*-- execute command --*/
        if (guiWithMultiout == 0) {
                spib_set_data(op_addr);        /*-- push flash command into tx fifo --*/
                spib_set_dctrl(spib_dctrl);    /*-- set dctrl --*/
                spib_set_cmd(0x0);             /*-- set dummy command to trigger transation start --*/
        } else {
                addr_entry = (unsigned int *) malloc (3*sizeof(unsigned int));
                data_entry = (unsigned int *) malloc (3*sizeof(unsigned int));
                addr_entry[0] = SPIB_REG_DATA;
                addr_entry[1] = SPIB_REG_DCTRL;
                addr_entry[2] = SPIB_REG_CMD;
                data_entry[0] = op_addr;
                data_entry[1] = spib_dctrl;
                data_entry[2] = 0x0;
                multiout_w (addr_entry, data_entry, 3);
                free (addr_entry);
                free (data_entry);
        }
}

void spib_rx_data (unsigned int *pRxdata, int RxBytes)
{
        unsigned int i, RxWords = 0;
        unsigned int *p_dst_buffer = (unsigned int *)pRxdata;

        // Fast memory access with a constant address (EDM v3.1.1)
        if (guiConstFastMode == 1) {
		        RxBytes = ((RxBytes + 3) / 4) * 4;
                fastin(SPIB_REG_DATA|0x02, RxBytes, (char *)p_dst_buffer);
                return;
        }

        if (guiWithMultiout == 0) {
                /*-- wait completion --*/
                while (spib_get_busy () != 0) {
                        if (spib_get_rx_empty () == 0) {
                                RxWords = spib_get_rx_entries ();
                                //printf("spib_get_rx_entries: %d\n", RxWords);
                                for (i = 0; i < RxWords; i++) {
                                        *p_dst_buffer++ = inw(SPIB_REG_DATA);
                                }
                        }
                }
                RxWords = spib_get_rx_entries ();
                for (i = 0; i < RxWords; i++) {
                        *p_dst_buffer++ = inw(SPIB_REG_DATA);
                }
        } else {
                RxWords = (SPI_RX_FIFO / 4);
                unsigned int *addr_entry = (unsigned int *) malloc (RxWords*sizeof(unsigned int));

                for (i = 0; i < RxWords; i++) {
                        addr_entry[i] = SPIB_REG_DATA;
                }
                while (RxBytes) {
                        multiin_w (addr_entry, p_dst_buffer, RxWords);
                        if (RxBytes >= (RxWords << 2))
                                RxBytes -= (RxWords << 2);
                        else
                                RxBytes = 0;
                        if (RxBytes<=0)
                                break;
                        p_dst_buffer += RxWords;
                }
                free (addr_entry);
        }
}

void spib_tx_data (unsigned int *pTxdata, int TxBytes)
{
        unsigned int i, j;
        unsigned int TxWords = ((TxBytes + 3)/ 4);
        unsigned int *p_src_buffer = (unsigned int *)pTxdata;
        unsigned int timeout = 100;
        unsigned int spib_tx_full;

        // Fast memory access with a constant address (EDM v3.1.1)
        if (guiConstFastMode == 1) {
                TxBytes = TxWords * 4;
                fastout(SPIB_REG_DATA|0x02, TxBytes, (char *)p_src_buffer);
                return;
        }

        if (guiWithMultiout == 0) {
                for (i = 0; i < TxWords; i++) {
                        for (j = 0; j < timeout; j++) {
                                spib_tx_full = (spib_get_fifost() & SPIB_FIFOST_TXFFL_MASK);

                                if (spib_tx_full == 0) {
                                        break;
                                }
                        }
                        if (spib_tx_full == 1) {
                                printf("spib_set_fifo: write fifo timeout\n");
                                return;
                        }
                        spib_set_data(*p_src_buffer++);
                }
        } else {
                TxWords = (SPI_TX_FIFO / 4);
                unsigned int *addr_entry = (unsigned int *) malloc (TxWords*sizeof(unsigned int));

                for (i = 0; i < TxWords; i++) {
                        addr_entry[i] = SPIB_REG_DATA;
                }
                while (TxBytes) {
                        multiout_w (addr_entry, p_src_buffer, TxWords);
                        if (TxBytes >= (TxWords << 2))
                                TxBytes -= (TxWords << 2);
                        else
                                TxBytes = 0;
                        if (TxBytes<=0)
                                break;
                        p_src_buffer += TxWords;
                }
                free (addr_entry);
        }
}

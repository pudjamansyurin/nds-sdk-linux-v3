#include "util.h"
#include "flash-rom.h"

extern flash_dev *gpFlash;

#define REG_AHBC_BASE       0x90100000
#define REG_SMC_BASE        0x90200000
#define REG_PCU_BASE        0x98100000
#define REG_AHBC_BASE_16    0x00E00000
#define REG_SMC_BASE_16     0x00E01000
#define REG_PCU_BASE_16     0x00F01000
//#define REG_DMAC_BASE     0x90400000
//#define REG_DMAC_BASE_16  0x00E03000
#define REG_SMC_BASE_AE300  0xE0400000
#define REG_SMU_BASE_AE300  0xF0100000
#define REG_AHBC_BASE_AE300 0xE0000000

#define PCU_SYSID_AG101_16MB 0x01000000  // 0x01010000 or 0x010x0000
#define PCU_SYSID_AG101_4GB  0x01000000
#define SMU_SYSID_AE300_4GB  0x41453000
#define SMU_SYSID_AE350_4GB  0x41453500

unsigned int platform_init_ae3004gb(void);
unsigned int platform_init_4gb(void);
unsigned int platform_init_16mb(void);
unsigned int platform_get_version_id(void);
int check_need_run_delay_count(void);

#define MEMMAP_AG101_16MB   0
#define MEMMAP_AG101_4GB    1
#define MEMMAP_AE300_4GB    2
#define MEMMAP_AE350_4GB    3
#define MEMMAP_MAX          4

unsigned int memmapping_ahbc_base[MEMMAP_MAX] = {
        REG_AHBC_BASE_16,
        REG_AHBC_BASE,
        REG_AHBC_BASE_AE300,
        REG_AHBC_BASE_AE300,
};

unsigned int memmapping_smc_base[MEMMAP_MAX] = {
        REG_SMC_BASE_16,
        REG_SMC_BASE,
        REG_SMC_BASE_AE300,
        REG_SMC_BASE_AE300,
};

unsigned int memmapping_pcu_base[MEMMAP_MAX] = {
        REG_PCU_BASE_16,
        REG_PCU_BASE,
        REG_SMU_BASE_AE300,
        REG_SMU_BASE_AE300,
};

unsigned int memmapping_system_id[MEMMAP_MAX] = {
        PCU_SYSID_AG101_16MB,
        PCU_SYSID_AG101_4GB,
        SMU_SYSID_AE300_4GB,
        SMU_SYSID_AE350_4GB,
};

unsigned int mem_mapping_mode = MEMMAP_AG101_16MB;
/* Fast memory access with a constant address, EDM_CFG >= 0x1011 only */
unsigned int guiConstFastMode = 0;
unsigned int guiUserDefConstFastMode = 0;

unsigned int platform_init(void)
{
        unsigned int Ret;

        platform_get_version_id();
        if (mem_mapping_mode == MEMMAP_AE300_4GB || mem_mapping_mode == MEMMAP_AE350_4GB)
                Ret = platform_init_ae3004gb();
        else if (mem_mapping_mode == MEMMAP_AG101_4GB)
                Ret = platform_init_4gb();
        else //if(mem_mapping_mode == MEMMAP_AG101_16MB)
                Ret = platform_init_16mb();
        printf("Ret = 0x%08x \n", Ret);
        return Ret;
}

unsigned int platform_init_ae3004gb(void)
{
        const unsigned int SMC_BASE  = memmapping_smc_base[mem_mapping_mode];
        unsigned int smc_bank0_reg;

        /* Write memory bank 0 configuration register */
        /* Disable write-protect, set BNK_SIZE to 64MB, set BNK_MBW to 32 */
        smc_bank0_reg = inw(SMC_BASE);
        /* limit BNK_SIZE = 32MB */
        smc_bank0_reg = (smc_bank0_reg & ~0xf0) | 0x50;
        /* disable write protection */
        smc_bank0_reg = (smc_bank0_reg & ~0x800);
        /* debug ae350 */
        //smc_bank0_reg = (smc_bank0_reg | 0x8000000);
        outw(SMC_BASE, smc_bank0_reg);
        /*outw(SMC_BASE+4, 0x00153153);*/

#if CONFIG_PARBURN
        return 0x88000000;
#else
        return 0x80000000;
#endif
}

/* XC5 initial function */
unsigned int platform_init_4gb(void)
{
        const unsigned int AHBC_BASE = memmapping_ahbc_base[mem_mapping_mode];
        const unsigned int SMC_BASE  = memmapping_smc_base[mem_mapping_mode];
        unsigned int ahbc_control_reg;
        unsigned int base_address_high; // [31:20]
        unsigned int base_address_low;  // [27:15]
        unsigned int smc_bank0_reg;

        //platform_get_version_id();
        /* read AHBC control register. If bit0 is 0, un-remap. If bit0 is 1, remap. */
        ahbc_control_reg = inw(AHBC_BASE + 0x88);
        if ((ahbc_control_reg & 0x1) == 0) {
                /* Write memory bank 0 configuration register */
                /* Disable write-protect, set BNK_SIZE to 64MB, set BNK_MBW to 32 */
                smc_bank0_reg = inw(SMC_BASE);
                /* limit BNK_SIZE = 32MB */
                smc_bank0_reg = (smc_bank0_reg & ~0xf0) | 0x50;
                /* disable write protection */
                smc_bank0_reg = (smc_bank0_reg & ~0x800);
                outw(SMC_BASE, smc_bank0_reg);
                /*outw(SMC_BASE+4, 0x00153153);*/
                return 0; // un-remap, base is 0x0
        }

        base_address_high = inw(AHBC_BASE + 0x10);
        base_address_low = inw(SMC_BASE);
        return (base_address_high & 0xFFF00000) | (base_address_low & 0x0FFF8000);
}

/* for 24 bit platform */
unsigned int platform_init_16mb(void)
{
        const unsigned int AHBC_BASE = memmapping_ahbc_base[mem_mapping_mode];
        const unsigned int SMC_BASE  = memmapping_smc_base[mem_mapping_mode];
        unsigned int ahbc_control_reg;
        unsigned int base_address_high; // [31:20]
        unsigned int base_address_low;  // [27:15]
        unsigned int smc_bank0_reg;

        /* read AHBC control register. If bit0 is 0, un-remap. If bit0 is 1, remap. */
        ahbc_control_reg = inw(AHBC_BASE + 0x88);
        //fprintf (pLogFile, "ahbc control: 0x%x\n", ahbc_control_reg);
        if ((ahbc_control_reg & 0x1) == 0) {
                /* Write memory bank 0 configuration register */
                /* Disable write-protect, set BNK_SIZE to 32MB, set BNK_MBW to 32 */
                smc_bank0_reg = inw(SMC_BASE);
                /* limit BNK_SIZE = 1MB */
                smc_bank0_reg = (smc_bank0_reg & ~0xf0);
                /* disable write protection */
                smc_bank0_reg = (smc_bank0_reg & ~0x800);
                outw(SMC_BASE, smc_bank0_reg);
                return 0; // un-remap, base is 0x0
        }
        base_address_high = inw(AHBC_BASE + 0x10);
        base_address_low = inw(SMC_BASE);
        //platform_get_version_id();
        gpFlash->flash_chipsize = 0x100000;
        //fprintf (pLogFile, "base: 0x%x\n", ((base_address_high & 0xFFF00000) | (base_address_low & 0x0FFF8000)) >> 8);
        return ((base_address_high & 0xFFF00000) | (base_address_low & 0x0FFF8000)) & 0x00FFFFFF;
}

unsigned int platform_get_version_id(void)
{
        unsigned int id1, pcu_base;

        pcu_base = memmapping_pcu_base[MEMMAP_AE300_4GB];
        id1 = inw(pcu_base + 0x00);
        id1 &= 0xFFFFFF00;
        if (id1 == memmapping_system_id[MEMMAP_AE300_4GB]) {
                mem_mapping_mode = MEMMAP_AE300_4GB;
        }else if (id1 == memmapping_system_id[MEMMAP_AE350_4GB]) {
                mem_mapping_mode = MEMMAP_AE350_4GB;
        }else {
                pcu_base = memmapping_pcu_base[MEMMAP_AG101_4GB];
                id1 = inw(pcu_base + 0x00);
                if ( (id1 & 0x010F0000) != 0)
                        mem_mapping_mode = MEMMAP_AG101_4GB;
                else {
                        pcu_base = memmapping_pcu_base[MEMMAP_AG101_16MB];
                        id1 = inw(pcu_base + 0x00);
                        mem_mapping_mode = MEMMAP_AG101_16MB;
                }
        }
        printf("PMU_VER_ID = 0x%08x \n", id1);
        return 0;
}

/* ae100, ae300, ae250 and ae350: restore IVB to flash */
/* ae210p: no need restore_ivb because loading the flash data to ILM on ae210p must do power-on(power-on will reset SMU) */
unsigned int restore_ivb(void)
{
    if (mem_mapping_mode == MEMMAP_AE300_4GB || mem_mapping_mode == MEMMAP_AE350_4GB) {
        printf("restore SMU IVB to 0x80000000\n");
        outw(REG_SMU_BASE_AE300 + 0x50, 0x80000000);
    }
    return 0;
}

int check_need_run_delay_count(void)
{
    unsigned int id1, pcu_base;

    pcu_base = memmapping_pcu_base[MEMMAP_AE300_4GB];
    id1 = inw(pcu_base + 0x00);
    id1 &= 0xFFFFFF00;
    if (id1 == memmapping_system_id[MEMMAP_AE350_4GB]) 
        return 0;
    else
        return 1;
}

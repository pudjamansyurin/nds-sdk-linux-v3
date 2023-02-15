#ifndef __PLAT_AE210P__
#define __PLAT_AE210P__

#define SPIB_TM_WRsim               0x0
#define SPIB_TM_WRonly              0x1
#define SPIB_TM_RDonly              0x2
#define SPIB_TM_WR_RD               0x3
#define SPIB_TM_RD_WR               0x4
#define SPIB_TM_WR_DY_RD            0x5
#define SPIB_TM_RD_DY_WR            0x6

#define SPIB_VERSION                0x02002000

extern unsigned int spib_get_ifset (void);
extern void spib_set_ifset (unsigned int reg);
extern unsigned int spib_get_pio (void);
extern void spib_set_pio (unsigned int reg);
extern unsigned int spib_get_ctrl (void);
extern void spib_set_ctrl (unsigned int reg);
/*extern unsigned int spib_get_fifost (void);*/
extern unsigned int spib_get_inten (void);
extern void spib_set_inten (unsigned int reg);
extern unsigned int spib_get_intst (void);
extern void spib_set_intst (unsigned int reg);
extern unsigned int spib_get_dctrl (void);
extern void spib_set_dctrl (unsigned int reg);
extern unsigned int spib_get_cmd(void);
extern void spib_set_cmd(unsigned int cmd);
extern unsigned int spib_get_addr(void);
extern void spib_set_addr(unsigned int addr);
extern unsigned int spib_get_data(void);
extern void spib_set_data(unsigned int data);
extern unsigned int spib_get_regtiming(void);
extern void spib_set_regtiming(unsigned int data);
extern void spib_get_fifo(unsigned int *buffer, unsigned int start, unsigned int length);
extern unsigned int spib_set_fifo(unsigned int *buffer, unsigned int start, unsigned int length);
extern unsigned int spib_prepare_dctrl(unsigned int,unsigned int,unsigned int,unsigned int,unsigned int,unsigned int);
extern unsigned int spib_wait_spi (void);
extern unsigned int spib_get_version (void);
extern unsigned int spib_get_busy (void);
extern unsigned int spib_get_rx_empty (void);
extern unsigned int spib_get_rx_entries (void);
extern void spib_clr_fifo (void);
extern void spib_exe_cmmd (unsigned int op_addr, unsigned int spib_dctrl);
extern void spib_rx_data (unsigned int *pRxdata, int RxBytes);
extern void spib_tx_data (unsigned int *pTxdata, int TxBytes);

#endif


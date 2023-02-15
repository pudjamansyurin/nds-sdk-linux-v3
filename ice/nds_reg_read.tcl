#source [find nds_debug_defines.tcl]
source [find dmi.tcl]
set NDS_TAP "123"
scan [jtag names] "%s" NDS_TAP

scan [nds reg_number] "%x" NDS_REG_NUMBER
echo [format "reg_number = 0x%x" $NDS_REG_NUMBER]

set NDS_REG_DATA [reg_read_abstract $NDS_TAP $NDS_REG_NUMBER]
nds reg_data $NDS_REG_DATA

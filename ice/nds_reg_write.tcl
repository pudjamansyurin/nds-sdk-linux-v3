#source [find nds_debug_defines.tcl]
source [find dmi.tcl]
set NDS_TAP "123"
scan [jtag names] "%s" NDS_TAP

scan [nds reg_number] "%x" NDS_REG_NUMBER
scan [nds reg_data] "%x" NDS_REG_DATA
echo [format "reg_number = 0x%x" $NDS_REG_NUMBER]

reg_write_abstract $NDS_TAP $NDS_REG_NUMBER $NDS_REG_DATA

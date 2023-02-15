source [find dmi.tcl]
set NDS_TAP "123"
scan [jtag names] "%s" NDS_TAP

set NDS_DMI_ADDR	    0
set NDS_DMI_DATA	    0
scan [nds dmi_addr] "%x" NDS_DMI_ADDR
scan [nds dmi_data] "%x" NDS_DMI_DATA
#puts [format "dmi-write = 0x%x, 0x%x" $NDS_DMI_ADDR $NDS_DMI_DATA]

dmi_write $NDS_TAP $NDS_DMI_ADDR $NDS_DMI_DATA

source [find dmi.tcl]
set NDS_TAP "123"
scan [jtag names] "%s" NDS_TAP

set NDS_DMI_ADDR	    0
scan [nds dmi_addr] "%x" NDS_DMI_ADDR
#puts [format "dmi-read = 0x%x" $NDS_DMI_ADDR]

set NDS_DMI_DATA [dmi_read $NDS_TAP $NDS_DMI_ADDR]
nds dmi_data $NDS_DMI_DATA

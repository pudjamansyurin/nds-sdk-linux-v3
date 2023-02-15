source [find dmi.tcl]
source [find debug_util.tcl]

set NDS_TAP "123"

set DMI_DMCONTROL      0x10
set DMI_DMCONTROL_HALTREQ   0x80000000

proc set_srst {delay_ms} {
	ftdi_write_pin nSRST 0
	sleep $delay_ms
}

proc clear_srst {delay_ms} {
	ftdi_write_pin nSRST 1
	sleep $delay_ms
}

proc set_dbgi {delay_ms} {
	global NDS_TAP
	global DMI_DMCONTROL
	global DMI_DMCONTROL_HALTREQ

	scan [nds jtag_tap_name] "%s" NDS_TAP
	set test_dmcontrol [dmi_read $NDS_TAP $DMI_DMCONTROL]
	set test_dmcontrol [expr $test_dmcontrol | $DMI_DMCONTROL_HALTREQ]
	dmi_write $NDS_TAP $DMI_DMCONTROL $test_dmcontrol
	sleep $delay_ms
}

proc clear_dbgi {delay_ms} {
	global NDS_TAP
	global DMI_DMCONTROL
	global DMI_DMCONTROL_HALTREQ

	scan [nds jtag_tap_name] "%s" NDS_TAP
	set test_dmcontrol [dmi_read $NDS_TAP $DMI_DMCONTROL]
	set test_dmcontrol [expr $test_dmcontrol & ~$DMI_DMCONTROL_HALTREQ]
	dmi_write $NDS_TAP $DMI_DMCONTROL $test_dmcontrol
	sleep $delay_ms
}

proc set_trst {delay_ms} {
	ftdi_write_pin nTRST 0
	sleep $delay_ms
}

proc clear_trst {delay_ms} {
	ftdi_write_pin nTRST 1
	sleep $delay_ms
}

proc delay {delay_ms} {
	sleep $delay_ms
}

proc set_current_target {set_target} {
	global NDS_TAP
	targets $set_target
	scan [nds jtag_tap_name] "%s" NDS_TAP
}

proc get_current_target {} {
	global number_of_target
	global TARGET_NAME
	global NDS_TARGETS_NAME
	global NDS_TARGETS_COREID
	global NDS_TARGETS_CORENUMS

	for {set i 0} {$i < $number_of_target} {incr $i} {
		scan [nds target_info $i] "%s %x %x" NDS_TARGETS_NAME($i) NDS_TARGETS_COREID($i) NDS_TARGETS_CORENUMS($i)
		puts [format "target-%d = %s 0x%x 0x%x" $i $NDS_TARGETS_NAME($i) $NDS_TARGETS_COREID($i) $NDS_TARGETS_CORENUMS($i)]
	}
}

proc reset_and_halt_current_hart {hartsel} {
	global NDS_TAP

	scan [nds jtag_tap_name] "%s" NDS_TAP
	reset_and_halt_one_hart $NDS_TAP $hartsel
}

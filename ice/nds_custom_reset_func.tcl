proc read_dmi {tap addr} {
	set DTM_IR_DMI 0x11
	irscan $tap $DTM_IR_DMI

	scan [drscan $tap 2 0x1 32 0x0 7 $addr] "%x %x %x" op rdata addr_out
	runtest 5
	scan [drscan $tap 2 0x0 32 0x0 7 $addr] "%x %x %x" op rdata addr_out

	return $rdata
}

proc write_dmi {tap addr wdata} {
	set DTM_IR_DMI 0x11
	irscan $tap $DTM_IR_DMI

	scan [drscan $tap 2 0x2 32 $wdata 7 $addr] "%x %x %x" op rdata addr_out
	runtest 5
	scan [drscan $tap 2 0x1 32 $wdata 7 $addr] "%x %x %x" op rdata addr_out
}

proc read_dmi_dmstatus {tap} {
	set DMI_ADDR_DMSTATUS 0x11
	return [read_dmi $tap $DMI_ADDR_DMSTATUS]
}

proc read_dmi_dmcontrol {tap} {
	set DMI_ADDR_DMCONTROL	0x10
	return [read_dmi $tap $DMI_ADDR_DMCONTROL]
}

proc write_dmi_dmcontrol {tap wdata} {
	set DMI_ADDR_DMCONTROL	0x10
	write_dmi $tap $DMI_ADDR_DMCONTROL $wdata
}

proc reset_and_halt_one_hart {tap hartsel} {
	set MAX_NHARTS 16
	set HARTSEL_MASK 0x3F0000

	# Assert ndmreset and haltreq
	set bf_haltreq [expr 1<<31]
	set bf_hartsel [expr $hartsel<<16]
	set bf_ndmreset [expr 1<<1]
	set bf_dmactive 0x1
	set dmcontrol [expr $bf_haltreq | $bf_hartsel | $bf_ndmreset | $bf_dmactive]
	write_dmi_dmcontrol $tap $dmcontrol

	# delay
	sleep 300

	set dmcontrol [read_dmi_dmcontrol $tap]
	if {[expr ($dmcontrol & $HARTSEL_MASK) != ($hartsel << 16)]} {
		break;
	}

	set dmstatus [read_dmi_dmstatus $tap]
	set dmstatus_anynonexistent [expr ($dmstatus>>14)&0x1]

	if {$dmstatus_anynonexistent} {
		break;
	}
	# De-assert ndmreset
	set bf_haltreq [expr 1<<31]
	set bf_dmactive 0x1
	set dmcontrol [expr $bf_haltreq | $bf_hartsel | $bf_dmactive]
	write_dmi_dmcontrol $tap $dmcontrol
}

proc parsing_targets {} {
	global NDS_TARGETS_COUNT
	global NDS_TARGETS_NAME
	global NDS_TARGETS_COREID
	global NDS_TARGETS_CORENUMS

	scan [nds target_count] "%x" NDS_TARGETS_COUNT
	for {set i 0} {$i < $NDS_TARGETS_COUNT} {incr $i} {
		scan [nds target_info $i] "%s %x %x" NDS_TARGETS_NAME($i) NDS_TARGETS_COREID($i) NDS_TARGETS_CORENUMS($i)
		#puts [format "target-%d = %s 0x%x 0x%x" $i $NDS_TARGETS_NAME($i) $NDS_TARGETS_COREID($i) $NDS_TARGETS_CORENUMS($i)]
	}
	for {set i 0} {$i < $NDS_TARGETS_COUNT} {incr $i} {
		puts [format "target-%d = %s 0x%x 0x%x" $i $NDS_TARGETS_NAME($i) $NDS_TARGETS_COREID($i) $NDS_TARGETS_CORENUMS($i)]
	}
	return
}


proc reset_and_halt_all_harts {} {
	global NDS_TARGETS_COUNT
	global NDS_TARGETS_NAME
	global NDS_TARGETS_COREID
	global NDS_TARGETS_CORENUMS

	set targetcount $NDS_TARGETS_COUNT
	for {set i 0} {$i < $targetcount} {incr $i} {
		targets $NDS_TARGETS_NAME($i)
		scan [nds jtag_tap_name] "%s" tap
		puts [format "tap = %s" $tap]

		set hartstart $NDS_TARGETS_COREID($i)
		set hartcount $NDS_TARGETS_CORENUMS($i)
		for {set hartsel $hartstart} {$hartsel < $hartcount} {incr $hartsel} {
			reset_and_halt_one_hart $tap $hartsel
		}
	}
}

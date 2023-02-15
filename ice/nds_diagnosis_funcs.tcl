proc test_frequency {tap} {
	set nds_if_pass "PASS"
	# Get current speed
	scan [nds adapter_khz] "%x" cur_adapter_khz

	# Change to current speed/2
	set test_adapter_khz [expr $cur_adapter_khz >> 1]
	puts [format "Setting adapter speed: %d kHz" $test_adapter_khz]
	nds adapter_khz $test_adapter_khz

	# Get speed again
	scan [nds adapter_khz] "%x" test2_adapter_khz
	puts [format "Current adapter speed: %d kHz" $test2_adapter_khz]

	if [ expr $test2_adapter_khz == $test_adapter_khz ] {
		puts [format "Success to set speed!!"]
	} else {
		puts [format "Unable to set speed!!"]
		set nds_if_pass "NG"
	}
	# Restore cur_speed
	nds adapter_khz $cur_adapter_khz

	return $nds_if_pass
}

proc test_dtm_connect {tap} {
	set nds_if_pass "PASS"
	# Get all taps
	set test_core_nums 7
	scan [nds jtag_tap_count] "%x" test_core_nums
	if [ expr $test_core_nums == 0x01 ] {
		puts [format "There is %d core in tap" $test_core_nums]
	} else {
		puts [format "There are %d core in tap" $test_core_nums]
	}

	set DTM_IR_DTMCONTROL     0x10
	set DTMCONTROL_VERSION    0xf
	irscan $tap $DTM_IR_DTMCONTROL
	scan [drscan $tap 32 0x0] "%x" test_dtmcontrol
	#runtest 5
	#scan [drscan $tap 32 0x0] "%x" test_dtmcontrol

	#if ((dtmcontrol == 0x0) || (dtmcontrol == 0xFFFFFFFF))
	if [ expr $test_dtmcontrol == 0x0 ] {
		set nds_if_pass "NG"
	}
	if [ expr $test_dtmcontrol == 0xFFFFFFFF ] {
		set nds_if_pass "NG"
	}

	set test_version [expr $test_dtmcontrol & $DTMCONTROL_VERSION]
	puts [format "dtmcontrol=0x%x version=0x%x" $test_dtmcontrol $test_version]
	return $nds_if_pass
}

proc test_dm_operate {tap} {
	set nds_if_pass "PASS"
	
	# "check that Debug Module (DM) is operational ..."
	set DTM_IR_DTMCONTROL     0x10
	set DTMCONTROL_VERSION    0xf
	irscan $tap $DTM_IR_DTMCONTROL
	scan [drscan $tap 32 0x0] "%x" test_dtmcontrol
	if [ expr $test_dtmcontrol == 0x0 ] {
		puts [format "dtmcontrol is 0. Check JTAG connectivity/board power." ]
		set nds_if_pass "NG"
		return $nds_if_pass
	}
	set test_version [expr $test_dtmcontrol & $DTMCONTROL_VERSION]
	if ![ expr $test_version == 0x1 ] {
		puts [format "Unsupported DTM version %d. (dtmcontrol=0x%x)" $test_version $test_dtmcontrol]
		set nds_if_pass "NG"
		return $nds_if_pass
	}
	
	# Reset the Debug Module
	set DMI_DMCONTROL      0x10
	set DMI_DMSTATUS       0x11
	set DMI_ABSTRACTCS     0x16
	set DMI_DMCONTROL_DMACTIVE        0x00001
	set DMI_DMSTATUS_AUTHENTICATED    0x00080
	set DMI_DMSTATUS_ANYUNAVAIL       0x01000
	set DMI_DMSTATUS_ANYNONEXISTENT   0x04000

	dmi_write $tap $DMI_DMCONTROL 0
	dmi_write $tap $DMI_DMCONTROL $DMI_DMCONTROL_DMACTIVE
	
	set test_dmcontrol [dmi_read $tap $DMI_DMCONTROL]
	set test_dmstatus [dmi_read $tap $DMI_DMSTATUS]
	puts [format "dmcontrol: 0x%08x" $test_dmcontrol]
	puts [format "dmstatus:  0x%08x" $test_dmstatus]
	
	if ![ expr $test_dmcontrol & $DMI_DMCONTROL_DMACTIVE ] {
		puts [format "Debug Module did not become active. dmcontrol=0x%x" $test_dmcontrol]
	}
	if ![ expr $test_dmstatus & $DMI_DMSTATUS_AUTHENTICATED ] {
		puts [format "Authentication required by RISC-V core but not supported by OpenOCD. dmcontrol=0x%x" $test_dmcontrol]
	}
	if [ expr $test_dmstatus & $DMI_DMSTATUS_ANYUNAVAIL ] {
		puts [format "The hart is unavailable."]
	}
	if [ expr $test_dmstatus & $DMI_DMSTATUS_ANYNONEXISTENT ] {
		puts [format "The hart doesn't exist."]
	}
	puts [format "DM Initial Successful!!"]
	
	# Check that abstract data registers are accessible.
	set test_abstractcs [dmi_read $tap $DMI_ABSTRACTCS]
	puts [format "abstractcs: 0x%08x" $test_abstractcs]

	# Halt every hart so we can probe them.
	halt

	return $nds_if_pass
}

proc test_reset_and_debug {tap} {
	set nds_if_pass "PASS"
	
	puts [format "ASSERTING NDRESET"]
	set DMI_DMCONTROL      0x10
	set DMI_DMSTATUS       0x11
	set DMI_DMCONTROL_NDMRESET  0x02
	set DMI_DMCONTROL_HALTREQ   0x80000000
	set test_dmcontrol [dmi_read $tap $DMI_DMCONTROL]

	set test_dmcontrol [expr $test_dmcontrol | $DMI_DMCONTROL_NDMRESET]
	set test_dmcontrol [expr $test_dmcontrol | $DMI_DMCONTROL_HALTREQ]
	dmi_write $tap $DMI_DMCONTROL $test_dmcontrol

	# alive_sleep(nds32->reset_time);
	sleep 3000

	# deassert reset
	set test_dmcontrol [dmi_read $tap $DMI_DMCONTROL]
	#puts [format "test_dmcontrol 0x%x" $test_dmcontrol]
	
	# Clear the reset, but make sure haltreq is still set
	set test_dmcontrol [expr $test_dmcontrol & ~$DMI_DMCONTROL_NDMRESET]
	set test_dmcontrol [expr $test_dmcontrol | $DMI_DMCONTROL_HALTREQ]
	#puts [format "test_dmcontrol 0x%x" $test_dmcontrol]
	dmi_write $tap $DMI_DMCONTROL $test_dmcontrol
	
	puts [format "DEASSERTING RESET, waiting for hart to be halted"]
	
	set DMI_DMSTATUS_ALLHALTED   0x200
	set test_dmstatus [dmi_read $tap $DMI_DMSTATUS]
	puts [format "test_dmstatus 0x%x" $test_dmstatus]
	while { [expr $test_dmstatus & $DMI_DMSTATUS_ALLHALTED] == 0x00 } {
		sleep 1 
		set retry [expr $retry + 1]
		if [ expr $retry >= 100 ] {
			puts [format "DMSTATUS ALLHALTED Failed!!"]
			set nds_if_pass "NG"
			return $nds_if_pass
		}
		set test_dmstatus [dmi_read $tap $DMI_DMSTATUS]
	}

	# clear halt
	set test_dmcontrol [expr $test_dmcontrol & ~$DMI_DMCONTROL_HALTREQ]
	dmi_write $tap $DMI_DMCONTROL $test_dmcontrol
	return $nds_if_pass
}

proc test_pbuf_work {tap} {
	set nds_if_pass "UNTESTED"
	# Save/Restore S0 test
	#set GDB_REGNO_S0   8
	#set nds_reg_value [reg_read_abstract $tap $GDB_REGNO_S0]
	#puts [format "nds_reg_value = 0x%x" $nds_reg_value]
	
	#set nds_reg_set_value   0x1234
	#reg_write_abstract $tap $GDB_REGNO_S0 $nds_reg_set_value
	
	#set nds_reg_value [reg_read_abstract $tap $GDB_REGNO_S0]
	#puts [format "nds_reg_value = 0x%x" $nds_reg_value]
	return $nds_if_pass
}

proc memread32 {ADDR} {
	set foo(0) 0
	if ![ catch { mem2array foo 32 $ADDR 1  } msg ] {
		return $foo(0)
	} else {
		error "memread32: $msg"
	}
}

proc memwrite32 {ADDR DATA} {
	set foo(0) $DATA
	if ![ catch { array2mem foo 32 $ADDR 1  } msg ] {
		return $foo(0)
	} else {
		error "memwrite32: $msg"
	}
}

proc test_memory_access {tap} {
	set nds_if_pass "PASS"
	set test_memory_addr   0x100
	set test_memory_value [memread32 $test_memory_addr]
	puts [format "test_memory_value = 0x%x" $test_memory_value]
	
	memwrite32 $test_memory_addr 0x12345678
	set test_memory_value [memread32 $test_memory_addr]
	puts [format "test_memory_value = 0x%x" $test_memory_value]
	
	return $nds_if_pass
}



proc dmi_read {tap addr} {
	set DTM_IR_DMI 0x11
	irscan $tap $DTM_IR_DMI

	scan [drscan $tap 2 0x1 32 0x0 7 $addr] "%x %x %x" op rdata addr_out
	runtest 5
	scan [drscan $tap 2 0x0 32 0x0 7 $addr] "%x %x %x" op rdata addr_out

	return $rdata
}

proc dmi_write {tap addr wdata} {
	set DTM_IR_DMI 0x11
	irscan $tap $DTM_IR_DMI

	scan [drscan $tap 2 0x2 32 $wdata 7 $addr] "%x %x %x" op rdata addr_out
	runtest 5
	scan [drscan $tap 2 0x1 32 $wdata 7 $addr] "%x %x %x" op rdata addr_out
}

proc reg_read_abstract {tap reg_num} {
	set GDB_REGNO_XPR0     0
	set GDB_REGNO_XPR31    31
	set GDB_REGNO_PC       32
	set GDB_REGNO_FPR0     33
	set GDB_REGNO_FPR31    64
	set GDB_REGNO_CSR0     65
	set GDB_REGNO_CSR4095  4160
	set regdata 0

	if [ expr $reg_num <= $GDB_REGNO_XPR31 ] {
		#puts [format "XPR_number = 0x%x" $reg_num]
		set reg_num [expr $reg_num - $GDB_REGNO_XPR0]
		set reg_num [expr $reg_num + 0x1000]
	} elseif [ expr $reg_num <= $GDB_REGNO_FPR31 ] {
		#puts [format "FPR_number = 0x%x" $reg_num]
		set reg_num [expr $reg_num - $GDB_REGNO_FPR0]
		set reg_num [expr $reg_num + 0x1020]
	} elseif [ expr $reg_num <= $GDB_REGNO_CSR4095 ] {
		#puts [format "CSR_number = 0x%x" $reg_num]
		set reg_num [expr $reg_num - $GDB_REGNO_CSR0]
	} else {
		puts [format "err_reg_num = 0x%x" $reg_num]
		nds script_status 1
		return $regdata
	}
	#puts [format "reg_num = 0x%x" $reg_num]
	set AC_ACCESS_REGISTER_WRITE        0x0010000
	set AC_ACCESS_REGISTER_TRANSFER     0x0020000
	set AC_ACCESS_REGISTER_POSTEXEC     0x0040000
	set AC_ACCESS_REGISTER_SIZE_32      0x0200000
	set AC_ACCESS_REGISTER_SIZE_64      0x0300000

	scan [nds target_xlen] "%x" NDS_TARGET_XLEN
	#puts [format "target_xlen = 0x%x" $NDS_TARGET_XLEN]

	set DMI_DATA0                0x04
	set DMI_ABSTRACTCS           0x16
	set DMI_COMMAND              0x17
	set DMI_ABSTRACTCS_CMDERR    0x0700

	if [ expr $NDS_TARGET_XLEN == 32 ] {
		set command	    $AC_ACCESS_REGISTER_SIZE_32
	} else {
		set command	    $AC_ACCESS_REGISTER_SIZE_64
	}

	set command [expr $command | $reg_num | $AC_ACCESS_REGISTER_TRANSFER]
	dmi_write $tap $DMI_COMMAND $command
	set dmi_stat [dmi_read $tap $DMI_ABSTRACTCS]
	set dmi_error [expr $dmi_stat & $DMI_ABSTRACTCS_CMDERR]

	set regdata [dmi_read $tap $DMI_DATA0]
	if [ expr $NDS_TARGET_XLEN == 64 ] {
		set reg_value_h [dmi_read $tap [expr $DMI_DATA0 + 1]]
		set reg_value_h [expr $reg_value_h << 32]
		set regdata [expr $regdata | $reg_value_h]
	}
	return $regdata
}

proc reg_write_abstract {tap reg_num reg_value} {
	set GDB_REGNO_XPR0     0
	set GDB_REGNO_XPR31    31
	set GDB_REGNO_PC       32
	set GDB_REGNO_FPR0     33
	set GDB_REGNO_FPR31    64
	set GDB_REGNO_CSR0     65
	set GDB_REGNO_CSR4095  4160
	set regdata 0

	if [ expr $reg_num <= $GDB_REGNO_XPR31 ] {
		#puts [format "XPR_number = 0x%x" $reg_num]
		set reg_num [expr $reg_num - $GDB_REGNO_XPR0]
		set reg_num [expr $reg_num + 0x1000]
	} elseif [ expr $reg_num <= $GDB_REGNO_FPR31 ] {
		#puts [format "FPR_number = 0x%x" $reg_num]
		set reg_num [expr $reg_num - $GDB_REGNO_FPR0]
		set reg_num [expr $reg_num + 0x1020]
	} elseif [ expr $reg_num <= $GDB_REGNO_CSR4095 ] {
		#puts [format "CSR_number = 0x%x" $reg_num]
		set reg_num [expr $reg_num - $GDB_REGNO_CSR0]
	} else {
		puts [format "err_reg_num = 0x%x" $reg_num]
		nds script_status 1
		return $regdata
	}
	#puts [format "reg_num = 0x%x" $reg_num]
	set AC_ACCESS_REGISTER_WRITE        0x0010000
	set AC_ACCESS_REGISTER_TRANSFER     0x0020000
	set AC_ACCESS_REGISTER_POSTEXEC     0x0040000
	set AC_ACCESS_REGISTER_SIZE_32      0x0200000
	set AC_ACCESS_REGISTER_SIZE_64      0x0300000

	scan [nds target_xlen] "%x" NDS_TARGET_XLEN
	#puts [format "target_xlen = 0x%x" $NDS_TARGET_XLEN]

	set DMI_DATA0                0x04
	set DMI_DATA1                0x05
	set DMI_ABSTRACTCS           0x16
	set DMI_COMMAND              0x17
	set DMI_ABSTRACTCS_CMDERR    0x0700

	if [ expr $NDS_TARGET_XLEN == 32 ] {
		dmi_write $tap $DMI_DATA0 $reg_value
		set command	    $AC_ACCESS_REGISTER_SIZE_32
	} else {
		set reg_value_h [expr $reg_value >> 32]
		set reg_value [expr $reg_value & 0xFFFFFFFF]
		dmi_write $tap $DMI_DATA0 $reg_value
		dmi_write $tap $DMI_DATA1 $reg_value_h
		set command	    $AC_ACCESS_REGISTER_SIZE_64
	}

	set command [expr $command | $reg_num | $AC_ACCESS_REGISTER_TRANSFER | $AC_ACCESS_REGISTER_WRITE]
	dmi_write $tap $DMI_COMMAND $command
	set dmi_stat [dmi_read $tap $DMI_ABSTRACTCS]
	set dmi_error [expr $dmi_stat & $DMI_ABSTRACTCS_CMDERR]

	return $regdata
}

proc nds_select_current_hart {tap hartid} {
	set DMI_DMCONTROL      0x10
	set DMI_DMCONTROL_HARTSEL_SHIFT    16
	set DMI_DMCONTROL_HARTSEL          0x3ff0000
	#puts [format "nds_select_current_hart %d" $hartid]

	set test_dmcontrol [dmi_read $tap $DMI_DMCONTROL]
	set test_dmcontrol [expr $test_dmcontrol & ~$DMI_DMCONTROL_HARTSEL]
	set hartid [expr $hartid << $DMI_DMCONTROL_HARTSEL_SHIFT]
	set test_dmcontrol [expr $test_dmcontrol | $hartid]

	# Force active DM
	set test_dmcontrol [expr $test_dmcontrol | 0x1]
	echo [format "set_dmcontrol: 0x%x" $test_dmcontrol]
	dmi_write $tap $DMI_DMCONTROL $test_dmcontrol

	set test_dmcontrol2 [dmi_read $tap $DMI_DMCONTROL]
	echo [format "read_dmcontrol2: 0x%x" $test_dmcontrol2]
	if [ expr $test_dmcontrol2 == $test_dmcontrol ] {
		#puts [format "success"]
		return 0
	}
	#puts [format "NG"]
	return 1
}

proc nds_auto_create_multi_targets {target_name tap} {
	global _number_of_core

	nds_auto_detect_targets $tap
	if [ expr $_number_of_core == 0x01 ] {
		puts [format "There is %d core in tap" $_number_of_core]
		echo [format "There is %d core in tap" $_number_of_core]
		return
	} else {
		puts [format "There are %d cores in tap" $_number_of_core]
		echo [format "There are %d cores in tap" $_number_of_core]
	}

	global _create_multi_targets
	if [ expr $_create_multi_targets == 0x00 ] {
		return
	}

	global _use_smp
	if {$_use_smp == 1} {
		return
	}

	#puts [format "create targets..."]
	for {set i 1} {$i < $_number_of_core} {incr i} {
		target create $target_name$i nds_v5 -chain-position $tap -coreid $i
	}
	#init
}

proc nds_auto_detect_targets {tap} {
	global _number_of_core
	set count_cores 0
	set RISCV_MAX_HARTS   129
	set DMI_DMSTATUS       0x11
	set DMI_DMSTATUS_ANYNONEXISTENT          0x04000

	transport init
	for {set i 0} {$i < $RISCV_MAX_HARTS} {incr i} {
		set retvalue [nds_select_current_hart $tap $i]
		if ![ expr $retvalue == 0 ] {
			echo [format "select_current_hart NG"]
			continue
		}
		set nds_dmstatus [dmi_read $tap $DMI_DMSTATUS]
		echo [format "dmstatus:  0x%08x" $nds_dmstatus]
		if ![ expr $nds_dmstatus & $DMI_DMSTATUS_ANYNONEXISTENT ] {
			set count_cores [expr $count_cores + 1]
			#puts [format "count_cores:  0x%08x" $count_cores]
		}
	}
	echo [format "count_cores:  0x%08x" $count_cores]
	set _number_of_core $count_cores
}

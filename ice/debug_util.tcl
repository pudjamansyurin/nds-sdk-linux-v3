
if {![info exists verbosity]} {
	set verbosity 100
}

proc print_info {msg} {
	puts "Diagnosis: $msg"	
}

proc print_debug_msg {msg} {
	global verbosity
	if {$verbosity > 100} {
		puts "Diagnosis: $msg"	
	}
}

proc assert {cond {msg "assertion failed"}} {
	if {![uplevel 1 expr $cond]} {error $msg}
}

proc init_dmi {tap} {
	set DTM_IR_DTMCS	0x10
	set DTM_IR_IDCODE	0x1
	set ANDES_JDTM_IDCODE	0x1000563d

	irscan $tap $DTM_IR_IDCODE
	scan [drscan $tap 32 0] "%x" idcode
	assert {[expr $idcode eq $ANDES_JDTM_IDCODE]} [format "IDCODE 0x%x is not expected Andes JDTM IDCODE (0x%x)]" $idcode $ANDES_JDTM_IDCODE]

	irscan $tap $DTM_IR_DTMCS
	scan [drscan $tap 32 0x30000] "%x" dtmcs
	print_debug_msg [format "dtmcs=0x%x" $dtmcs]

}

proc read_dmi {tap addr} {
	set DTM_IR_DMI 0x11
	irscan $tap $DTM_IR_DMI

	scan [drscan $tap 2 0x1 32 0x0 7 $addr] "%x %x %x" op rdata addr_out
	print_debug_msg [format "read_dmi %02x_00000000_1 -> %02x_%08x_%x" $addr $addr_out $rdata $op]

	runtest 10
	scan [drscan $tap 2 0x0 32 0x0 7 $addr] "%x %x %x" op rdata addr_out
	print_debug_msg [format "read_dmi %02x_00000000_0 -> %02x_%08x_%x" $addr $addr_out $rdata $op]
	runtest 10

	return $rdata
}

proc write_dmi {tap addr wdata} {
	set DTM_IR_DMI 0x11
	irscan $tap $DTM_IR_DMI

	scan [drscan $tap 2 0x2 32 $wdata 7 $addr] "%x %x %x" op rdata addr_out
	print_debug_msg [format "write_dmi %02x_%08x_2 -> %02x_%08x" $addr $wdata $addr_out $rdata $op]
	runtest 10
	scan [drscan $tap 2 0x0 32 $wdata 7 $addr] "%x %x %x" op rdata addr_out
	print_debug_msg [format "write_dmi %02x_00000000 -> %x_%02x_%08x" $wdata $addr_out $rdata $op]
}

proc write_dmi_dmcontrol {tap wdata} {
	set DMI_ADDR_DMCONTROL	0x10
	write_dmi $tap $DMI_ADDR_DMCONTROL $wdata
}

proc write_dmi_progbuf {tap n wdata} {
	assert {[expr ($n < 16) & ($n >=0)]} "program buffer index out of range (0-15)"
	set DMI_ADDR_PROGBUF [expr 0x20 + $n]
	return [write_dmi $tap $DMI_ADDR_PROGBUF $wdata]
}

proc write_dmi_abstractdata {tap n wdata} {
	assert {[expr ($n < 12) & ($n >=0)]} "abstract data index out of range (0-12)"
	set DMI_ADDR_ABSTRACTDATA [expr 0x4 + $n]
	return [write_dmi $tap $DMI_ADDR_ABSTRACTDATA $wdata]
}

proc write_dmi_abstractcommand {tap wdata} {
	set DMI_ADDR_ABSTRACTCOMMAND 0x17
	return [write_dmi $tap $DMI_ADDR_ABSTRACTCOMMAND $wdata]
}

proc write_dmi_abstractcs {tap wdata} {
	set DMI_ADDR_ABSTRACTCS 0x16
	return [write_dmi $tap $DMI_ADDR_ABSTRACTCS $wdata]
}

proc write_dmi_abstractauto {tap wdata} {
	set DMI_ADDR_ABSTRACTAUTO 0x18
	return [write_dmi $tap $DMI_ADDR_ABSTRACTAUTO $wdata]
}

proc read_dmi_dmcontrol {tap} {
	set DMI_ADDR_DMCONTROL	0x10
	return [read_dmi $tap $DMI_ADDR_DMCONTROL]
}

proc read_dmi_dmstatus {tap} {
	set DMI_ADDR_DMSTATUS 0x11
	return [read_dmi $tap $DMI_ADDR_DMSTATUS]
}

proc read_dmi_hartinfo {tap} {
	set DMI_ADDR_HARTINFO 0x12
	return [read_dmi $tap $DMI_ADDR_HARTINFO]
}

proc read_dmi_progbuf {tap n} {
	assert {[expr ($n < 16) & ($n >=0)]} "program buffer index out of range (0-15)"
	set DMI_ADDR_PROGBUF [expr 0x20 + $n]
	return [read_dmi $tap $DMI_ADDR_PROGBUF]
}

proc read_dmi_abstractdata {tap n} {
	assert {[expr ($n < 12) & ($n >=0)]} "abstract data index out of range (0-12)"
	set DMI_ADDR_ABSTRACTDATA [expr 0x4 + $n]
	return [read_dmi $tap $DMI_ADDR_ABSTRACTDATA]
}

proc read_dmi_abstractcs {tap} {
	set DMI_ADDR_ABSTRACTCS 0x16
	return [read_dmi $tap $DMI_ADDR_ABSTRACTCS]
}

proc read_dmi_abstractauto {tap wdata} {
	set DMI_ADDR_ABSTRACTAUTO 0x18
	return [read_dmi $tap $DMI_ADDR_ABSTRACTAUTO]
}

proc reset_dm {tap} {
	# ndmreset
	write_dmi_dmcontrol $tap 0x0
	write_dmi_dmcontrol $tap 0x1

	# check dmstatus
	set dmstatus [read_dmi_dmstatus $tap]

	set dmstatus_version [expr $dmstatus&0xF]
	set dmstatus_authenticated [expr ($dmstatus>>7)&0x1]
	set dmstatus_impebreak [expr ($dmstatus>>22)&0x1]

	#print_info [format "dmstatus=0x%x" $dmstatus]
	#print_info [format "\tversion=0x%x" $dmstatus_version]
	#print_info [format "\timpebreak=0x%x" $dmstatus_impebreak]

}

proc reset_ndm {tap} {
	set dmcontrol [read_dmi_dmcontrol $tap]
	set NDMRESET_MASK [expr 1 << 1]

	set dmcontrol [expr $dmcontrol | $NDMRESET_MASK]
	write_dmi_dmcontrol $tap $dmcontrol

	set dmcontrol [expr $dmcontrol & ~$NDMRESET_MASK]
	write_dmi_dmcontrol $tap $dmcontrol
}

proc is_selected_hart_anyunavail {tap} {
	set dmstatus [read_dmi_dmstatus $tap]
	set ANYUNAVAIL_MASK 0x1000
	return  [expr $dmstatus & $ANYUNAVAIL_MASK]
}

proc is_selected_hart_halted {tap} {
	set dmstatus [read_dmi_dmstatus $tap]
	set ANYHALTED_MASK 0x100
	return  [expr $dmstatus & $ANYHALTED_MASK]
}

proc is_selected_hart_running {tap} {
	set dmstatus [read_dmi_dmstatus $tap]
	set ANYRUNNING_MASK 0x400
	return  [expr $dmstatus & $ANYRUNNING_MASK]
}


# select_single_hart 
# Return
#	0: okay
#	1: fail
proc select_single_hart {tap hartsel} {
	assert {[expr ($hartsel < 1024) & ($hartsel >=0)]} "hartsel out of range (0-1023)"
	set HARTSEL_MASK 0x3F0000

	set dmcontrol [read_dmi_dmcontrol $tap]
	set dmcontrol [expr ($dmcontrol & ~$HARTSEL_MASK) | ($hartsel << 16)]
	write_dmi_dmcontrol $tap $dmcontrol
	set dmcontrol [read_dmi_dmcontrol $tap]

	if {[expr ($dmcontrol & $HARTSEL_MASK) != ($hartsel << 16)]} {
		return 1
	} else {
		return 0
	}
}

# wait_selected_hart_halted {tap timeout_ms}
#	tap: tap
#	timeout_ms: timeout in ms
# Return
#	1: the selected is halted
#	0: timeout
proc wait_selected_hart_halted {tap timeout_ms} {
	for {set i 0} {$i < $timeout_ms} {set i [expr $i+10]} {
		if {[is_selected_hart_halted $tap]} {
			return 1
		} else {
			after 10
		}
	}

	return 0
}

proc halt_hart {tap hartsel} {
	assert {[expr ($hartsel < 1024) & ($hartsel >=0)]} "hartsel out of range (0-1023)"

	set bf_haltreq [expr 1<<31]
	set bf_hartsel [expr $hartsel<<16]
	set bf_dmactive 0x1
	set dmcontrol [expr $bf_haltreq | $bf_hartsel | $bf_dmactive]

	write_dmi_dmcontrol $tap $dmcontrol

	assert {[wait_selected_hart_halted $tap 3000]} [format "halt hart%d timeout" $hartsel]
}

# wait_selected_hart_running {tap timeout_ms}
#	tap: tap
#	timeout_ms: timeout in ms
# Return
#	1: the selected is running
#	0: timeout
proc wait_selected_hart_running {tap timeout_ms} {
	for {set i 0} {$i < $timeout_ms} {set i [expr $i+10]} {
		if {[is_selected_hart_running $tap]} {
			return 1
		} else {
			after 10
		}
	}

	return 0
}


proc resume_hart {tap hartsel} {
	assert {[expr ($hartsel < 1024) & ($hartsel >=0)]} "hartsel out of range (0-1023)"

	set bf_resumereq [expr 1<<30]
	set bf_hartsel [expr $hartsel<<16]
	set bf_dmactive 0x1
	set dmcontrol [expr $bf_resumereq | $bf_hartsel | $bf_dmactive]

	write_dmi_dmcontrol $tap $dmcontrol

	assert {[wait_selected_hart_running $tap 3000]} [format "Resume hart%d timeout" $hartsel]
}

proc scan_harts {tap} {
	set scan_hart_nums 0
	set MAX_NHARTS 16
	for {set hartsel 0} {$hartsel < $MAX_NHARTS} {incr $hartsel} {
		if {[select_single_hart $tap $hartsel]} {
			break
		}
		set dmstatus [read_dmi_dmstatus $tap]
		set dmstatus_anyhalted [expr ($dmstatus>>8)&0x1]
		set dmstatus_allhalted [expr ($dmstatus>>9)&0x1]
		set dmstatus_anyrunning [expr ($dmstatus>>10)&0x1]
		set dmstatus_allrunning [expr ($dmstatus>>11)&0x1]
		set dmstatus_anyunavail [expr ($dmstatus>>12)&0x1]
		set dmstatus_allunavail [expr ($dmstatus>>13)&0x1]
		set dmstatus_anynonexistent [expr ($dmstatus>>14)&0x1]
		set dmstatus_allnonexistent [expr ($dmstatus>>15)&0x1]
		set dmstatus_anyhavereset [expr ($dmstatus>>18)&0x1]
		set dmstatus_allhavereset [expr ($dmstatus>>19)&0x1]


		assert {$dmstatus_anyhalted == $dmstatus_allhalted}
		assert {$dmstatus_anyrunning == $dmstatus_allrunning}
		assert {$dmstatus_anyunavail == $dmstatus_allunavail}
		assert {$dmstatus_anynonexistent == $dmstatus_allnonexistent}
		assert {$dmstatus_anyhavereset == $dmstatus_allhavereset}

		set hartinfo [read_dmi_hartinfo $tap]
		set hartinfo_datasize [expr ($hartinfo >> 12) & 0xF]
		set hartinfo_nscratch [expr ($hartinfo >> 20) & 0xF]

		if {$dmstatus_anynonexistent} {
			break
		}
		#print_info [format "Hart %d dmstatus=0x%x (halted=%d, running=%d, unavail=%d, havereset=%d, datasize=%d, nscratch=%d)" $hartsel $dmstatus $dmstatus_anyhalted $dmstatus_anyrunning $dmstatus_anyunavail $dmstatus_anyhavereset $hartinfo_datasize $hartinfo_nscratch]
		set scan_hart_nums [expr $scan_hart_nums + 1]
	}
	return $scan_hart_nums
}

proc reset_and_halt_one_hart {tap hartsel} {
	set HARTSEL_MASK 0x3F0000

	# Assert ndmreset and haltreq
	set bf_haltreq [expr 1<<31]
	set bf_hartsel [expr $hartsel<<16]
	set bf_ndmreset [expr 1<<1]
	set bf_dmactive 0x1
	set dmcontrol [expr $bf_haltreq | $bf_hartsel | $bf_ndmreset | $bf_dmactive]
	write_dmi_dmcontrol $tap $dmcontrol

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
	assert {[wait_selected_hart_halted $tap 3000]} [format "halt hart%d timeout" $hartsel]
}

proc reset_and_halt_all_harts {tap hartstart hartcount} {
	set HARTSEL_MASK 0x3F0000
	for {set hartsel $hartstart} {$hartsel < $hartcount} {incr $hartsel} {
		# Assert ndmreset and haltreq
		set bf_haltonreset [expr 1<<3]
		set bf_haltreq [expr 1<<31]
		set bf_hartsel [expr $hartsel<<16]
		set bf_ndmreset [expr 1<<1]
		set bf_dmactive 0x1
		set dmcontrol [expr $bf_haltreq | $bf_hartsel | $bf_ndmreset | $bf_dmactive | $bf_haltonreset]
		write_dmi_dmcontrol $tap $dmcontrol

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
		set bf_clr_haltonreset [expr 1<<4]
		set bf_haltreq [expr 1<<31]
		set bf_dmactive 0x1
		set dmcontrol [expr $bf_haltreq | $bf_hartsel | $bf_dmactive | $bf_clr_haltonreset]
		write_dmi_dmcontrol $tap $dmcontrol
		assert {[wait_selected_hart_halted $tap 3000]} [format "halt hart%d timeout" $hartsel]
	}
}


proc is_abstractcs_busy {tap} {
	set abstractcs [read_dmi_abstractcs $tap]
	set BUSY_MASK [expr 1 << 12]
	return  [expr $abstractcs & $BUSY_MASK]
}

# wait_abstractcs_not_busy {tap timeout_ms}
#	tap: tap
#	timeout_ms: timeout in ms
# Return
#	1: abstractcs.busy is clear
#	0: timeout
proc wait_abstractcs_busy_clear {tap timeout_ms} {
	global ABSTRCT_ERR
	set ABSTRCT_ERR 0
	for {set i 0} {$i < $timeout_ms} {set i [expr $i+100]} {
		set abstractcs [read_dmi_abstractcs $tap]
		array set str_cmderr {0 none 1 busy 2 not-supported 3 exception 4 halt/resume 5 bus 6 reserved 7 other}
		set BUSY_MASK [expr 1 << 12]
		set cmderr [expr ($abstractcs >> 8) & 0x7]
		if {$cmderr != 0} {
			set ABSTRCT_ERR 1
			puts [format "abstract command error: %d (%s)" $cmderr $str_cmderr($cmderr)]
		}
		if {[expr $abstractcs & $BUSY_MASK]} {
			after 100
		} else {
			return 1
		}
	}

	return 0
}

proc execute_progbuf {tap} {
	# clear abstractcs.cmderr
	write_dmi_abstractcs $tap [expr 0x7 << 8]

	# execute program buffer
	set postexec [expr 1 << 18]
	set abstractcommand $postexec

	write_dmi_abstractcommand $tap $abstractcommand


	assert {[wait_abstractcs_busy_clear $tap 3000]} "executing program buffer timeout"

}

proc write_register {tap xlen regno wdata} {
	assert {![is_selected_hart_anyunavail $tap]} "selected hart is unavailable"
	assert {[is_selected_hart_halted $tap]} "selected hart is not halted"

	write_dmi_abstractdata $tap 0 [expr $wdata & 0xFFFFFFFF]
	if {$xlen == 64} {
		write_dmi_abstractdata $tap 1 [expr $wdata >> 32]
	}

	# clear abstractcs.cmderr
	write_dmi_abstractcs $tap [expr 0x7 << 8]

	# execute abstract command
	set abstractcommand_size [expr ($xlen == 64 ? 3 : 2) << 20]
	set abstractcommand_transfer [expr 1<<17]
	set abstractcommand_write [expr 1<<16]
	set abstractcommand [expr $abstractcommand_size | $abstractcommand_transfer | $abstractcommand_write | $regno]

	write_dmi_abstractcommand $tap $abstractcommand
	assert {[wait_abstractcs_busy_clear $tap 3000]} "executing abstract command timeout"
}

#
#read_register
#	regno:
#		0x0000 - 0x0fff: CSRs
#		0x1000 - 0x101f: GPRs
#		0x1020 - 0x103f: Floating point registers

proc read_register {tap xlen regno} {
	assert {![is_selected_hart_anyunavail $tap]} "selected hart is unavailable"
	assert {[is_selected_hart_halted $tap]} "selected hart is not halted"

	# execute abstract command
	set abstractcommand_cmdtype [expr 0x0 << 24]
	set abstractcommand_size [expr ($xlen == 64 ? 3 : 2) << 20]
	set abstractcommand_transfer [expr 1<<17]
	set abstractcommand [expr $abstractcommand_cmdtype | $abstractcommand_size | $abstractcommand_transfer | $regno]

	# clear abstractcs.cmderr
	write_dmi_abstractcs $tap [expr 0x7 << 8]

	write_dmi_abstractcommand $tap $abstractcommand
	assert {[wait_abstractcs_busy_clear $tap 3000]} "executing abstract command timeout"

	set data0 [read_dmi_abstractdata $tap 0]
	if {$xlen == 64} {
		set data1 [read_dmi_abstractdata $tap 1]
		return [expr $data0 | $data1<<32]
	} else {
		return $data0
	}
}

proc quick_access_dpc {tap xlen} {

	if {$xlen == 32} {
		# 0x0c802023 sw      s0,192(zero) # c0 <DMI_DATA0>
		# 0x7b102473 csrr    s0,dpc
		# 0x0c802423 sw      s0,200(zero) # c8 <DMI_DATA2>
		# 0x0c002403 lw      s0,192(zero) # c0 <DMI_DATA0>
		# 0x00100073 ebreak
		set program {0x0c802023 0x7b102473 0x0c802423 0x0c002403 0x00100073}
	} else {
		# 0x0c803023 sd      s0,192(zero) # c0 <DMI_DATA0>
		# 0x7b102473 csrr    s0,dpc
		# 0x0c803423 sd      s0,200(zero) # c8 <DMI_DATA2>
		# 0x0c003403 ld      s0,192(zero) # c0 <DMI_DATA0>
		# 0x00100073 ebreak
		set program {0x0c803023 0x7b102473 0x0c803423 0x0c003403 0x00100073}
	}

	write_dmi_progbuf $tap 0 [lindex $program 0]
	write_dmi_progbuf $tap 1 [lindex $program 1]
	write_dmi_progbuf $tap 2 [lindex $program 2]
	write_dmi_progbuf $tap 3 [lindex $program 3]
	write_dmi_progbuf $tap 4 [lindex $program 4]

	set abstractcommand [expr 0x1 << 24]
	write_dmi_abstractcommand $tap $abstractcommand
	assert {[wait_abstractcs_busy_clear $tap 3000]} "quick access dpc timeout"

	set data2 [read_dmi_abstractdata $tap 2]
	if {$xlen == 64} {
		set data3 [read_dmi_abstractdata $tap 3]
		return [expr $data2 | $data3<<32]
	} else {
		return $data2
	}
}

proc read_dpc {tap xlen} {
	return [read_register $tap $xlen 0x7b1]
}

proc write_dpc {tap xlen wdata} {
	return [write_register $tap $xlen 0x7b1 $wdata]
}

proc write_s0 {tap xlen wdata} {
	write_register $tap $xlen 0x1008 $wdata

        ## 0x09802403 lw      s0,152(zero) # 0x98 PROGBUF6
	## 0x00100073 ebreak
	#write_dmi_progbuf $tap 0 0x09802403
	#write_dmi_progbuf $tap 1 0x00100073
	#write_dmi_progbuf $tap 6 $wdata
	#execute_progbuf $tap
}

proc abstract_read_memory {tap xlen addr} {
	assert {![is_selected_hart_anyunavail $tap]} "selected hart is unavailable"
	assert {[is_selected_hart_halted $tap]} "selected hart is not halted"

	if {$xlen == 32} {
		write_dmi_abstractdata $tap 1 [expr $addr & 0xFFFFFFFF]
		set aamsize [expr 1 << 21]
	} elseif {$xlen == 64} {
		write_dmi_abstractdata $tap 2 [expr $addr & 0xFFFFFFFF]
		write_dmi_abstractdata $tap 3 [expr ($addr >> 32) & 0xFFFFFFFF]
		set aamsize [expr (1 << 20) | (1 << 21)]
	} else {
		puts [format "unknow xlen %d" $xlen]
		return 0
	}

	set cmdtype [expr 1 << 25]
	set command [expr $cmdtype | $aamsize]
	write_dmi_abstractcommand $tap $command
	assert {[wait_abstractcs_busy_clear $tap 3000]} "executing abstract read memory timeout"

	set data0 [read_dmi_abstractdata $tap 0]
	if {$xlen == 64} {
		set data1 [read_dmi_abstractdata $tap 1]
		return [expr $data0 | $data1<<32]
	} else {
		return $data0
	}
}

proc abstract_read_block_memory {tap xlen addr data_list} {
	assert {![is_selected_hart_anyunavail $tap]} "selected hart is unavailable"
	assert {[is_selected_hart_halted $tap]} "selected hart is not halted"

	write_dmi_abstractauto $tap 0x0
	if {$xlen == 32} {
		write_dmi_abstractdata $tap 1 [expr $addr & 0xFFFFFFFF]
		set aamsize [expr 1 << 21]
	} elseif {$xlen == 64} {
		write_dmi_abstractdata $tap 2 [expr $addr & 0xFFFFFFFF]
		write_dmi_abstractdata $tap 3 [expr ($addr >> 32) & 0xFFFFFFFF]
		set aamsize [expr (1 << 20) | (1 << 21)]
	} else {
		puts [format "unknow xlen %d" $xlen]
		return 0
	}

	set cmdtype [expr 1 << 25]
	set aampostincrement [expr 1 << 19]
	set command [expr $cmdtype | $aamsize | $aampostincrement]
	write_dmi_abstractcommand $tap $command
	assert {[wait_abstractcs_busy_clear $tap 3000]} "executing abstract read memory timeout"

	write_dmi_abstractauto $tap 0x1
	for {set i 0} {$i < [llength $data_list]} {incr i} {
		if {$xlen == 64} {
			set data1 [read_dmi_abstractdata $tap 1]
		}
		set data0 [read_dmi_abstractdata $tap 0]
		while 1 {
			if {![is_abstractcs_busy $tap]} {
				break
			}
		}
		if {$xlen == 64} {
			set read_data [expr $data0 | $data1<<32]
			set expect_data [lindex $data_list $i]
		} else {
			set read_data $data0
			set expect_data [expr [lindex $data_list $i] & 0xFFFFFFFF]
		}
		if {$read_data != $expect_data} {
			write_dmi_abstractauto $tap 0x0
			puts [format "read/write memory mismatch: addr=0x%x, wdata=0x%x, rdata=0x%x" $addr $expect_data $read_data]
			return 0
		}
	}
	write_dmi_abstractauto $tap 0x0
	return 1
}

proc abstract_write_block_memory {tap xlen addr data_list} {
	assert {![is_selected_hart_anyunavail $tap]} "selected hart is unavailable"
	assert {[is_selected_hart_halted $tap]} "selected hart is not halted"

	write_dmi_abstractauto $tap 0x0
	if {$xlen == 32} {
		write_dmi_abstractdata $tap 0 [expr [lindex $data_list 0] & 0xFFFFFFFF]
		write_dmi_abstractdata $tap 1 [expr $addr & 0xFFFFFFFF]
		set aamsize [expr 1 << 21]
	} elseif {$xlen == 64} {
		write_dmi_abstractdata $tap 0 [expr [lindex $data_list 0] & 0xFFFFFFFF]
		write_dmi_abstractdata $tap 1 [expr ([lindex $data_list 0] >> 32) & 0xFFFFFFFF]
		write_dmi_abstractdata $tap 2 [expr $addr & 0xFFFFFFFF]
		write_dmi_abstractdata $tap 3 [expr ($addr >> 32) & 0xFFFFFFFF]
		set aamsize [expr (1 << 20) | (1 << 21)]
	} else {
		puts [format "unknow xlen %d" $xlen]
		return 0
	}

	set cmdtype [expr 1 << 25]
	set aampostincrement [expr 1 << 19]
	set write [expr 1 << 16]
	set command [expr $cmdtype | $aamsize | $aampostincrement | $write]
	write_dmi_abstractcommand $tap $command
	assert {[wait_abstractcs_busy_clear $tap 3000]} "executing abstract write memory timeout"

	write_dmi_abstractauto $tap 0x1
	for {set i 1} {$i < [llength $data_list]} {incr i} {
		if {$xlen == 64} {
			write_dmi_abstractdata $tap 1 [expr ([lindex $data_list $i] >> 32) & 0xFFFFFFFF]
		}
		write_dmi_abstractdata $tap 0 [expr [lindex $data_list $i] & 0xFFFFFFFF]
		while 1 {
			if {![is_abstractcs_busy $tap]} {
				break
			}
		}
	}

	write_dmi_abstractauto $tap 0x0
	return 1
}

proc read_memory_word {tap addr} {

	assert {![is_selected_hart_anyunavail $tap]} "selected hart is unavailable"
	assert {[is_selected_hart_halted $tap]} "selected hart is not halted"

	scan [nds target_xlen] "%x" xlen
	write_s0 $tap $xlen $addr

	set s0 [read_register $tap $xlen 0x1008]
	echo [format "(After) s0:0x%x" $s0]

	# 0x00042483: lw      s1,0(s0)
	# 0x08902e23: sw      s1,156(zero) # 0x9c PROGBUF7
	# 0x0000000f: fence   unknown,unknow
	# 0x00100073: ebreak
	write_dmi_progbuf $tap 0 0x00042483
	write_dmi_progbuf $tap 1 0x08902e23
	write_dmi_progbuf $tap 2 0x0000000f
	write_dmi_progbuf $tap 3 0x00100073
	execute_progbuf $tap

	return [read_dmi_progbuf $tap 7]
}

proc write_memory_word {tap addr wdata} {
	assert {![is_selected_hart_anyunavail $tap]} "selected hart is unavailable"
	assert {[is_selected_hart_halted $tap]} "selected hart is not halted"


	# 0x09802403: lw      s0,152(zero) # 0x98 PROGBUF6
	# 0x09c02483: lw      s1,156(zero) # 0x9c PROGBUF7
	# 0x00942023: sw      s1,0(s0)
	# 0x0000000f: fence   unknown,unknow
	# 0x00100073: ebreak
	write_dmi_progbuf $tap 0 0x09802403
	write_dmi_progbuf $tap 1 0x09c02483
	write_dmi_progbuf $tap 2 0x00942023
	write_dmi_progbuf $tap 3 0x0000000f
	write_dmi_progbuf $tap 4 0x00100073
	write_dmi_progbuf $tap 6 $addr
	write_dmi_progbuf $tap 7 $wdata
	execute_progbuf $tap
}

proc batch_read_memory_word {tap addr len} {
	assert {![is_selected_hart_anyunavail $tap]} "selected hart is unavailable"
	assert {[is_selected_hart_halted $tap]} "selected hart is not halted"

	# s0
	scan [nds target_xlen] "%x" xlen
	write_s0 $tap $xlen $addr

	# 0x00042483: lw      s1,0(s0)
	# 0x0c902023: sw      s1,192(zero) # 0xc0 (DATA0)
	# 0x00440413: addi    s0,s0,4
	# 0x0000000f: fence   unknown,unknow
	# 0x00100073: ebreak
	write_dmi_progbuf $tap 0 0x00042483
	write_dmi_progbuf $tap 1 0x0c902023
	write_dmi_progbuf $tap 2 0x00440413
	write_dmi_progbuf $tap 3 0x0000000f
	write_dmi_progbuf $tap 4 0x00100073
	execute_progbuf $tap

	write_dmi_abstractauto $tap 0x1

	for {set i 0} {$i < ($len-1)} {incr i} {
		set data0 [read_dmi_abstractdata $tap 0]
	}

	write_dmi_abstractauto $tap 0x0
	set data0 [read_dmi_abstractdata $tap 0]
}


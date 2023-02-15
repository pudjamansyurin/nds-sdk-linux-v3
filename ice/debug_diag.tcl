
# Main
source [find openocd.cfg]
source [find debug_util.tcl]
set verbosity 100
set time_target_sec 1
#adapter_khz 10000
#source [find interface/olimex-arm-usb-tiny-h.cfg]

# ABSTRCT_ERR:
# 0 : no error
# 1 : error
set ABSTRCT_ERR 0

# Define
set CSR_MVENDORID  0xf11
set CSR_MARCHID    0xf12
set CSR_MIMPID     0xf13
set CSR_MISA       0x301
set CSR_DCSR       0x7b0
set CSR_MNVEC      0x7c3


set test_dm_operate_pass "NG"
set test_dtm_connect_pass "NG"
set test_memory_access_pass "NG"
set test_reset_and_debug_pass "NG"
set test_frequency_pass "NG"


proc test_memory_rw {tap start_addr} {
	set count 4
	set wdata_list {0xaaaaaaaa 0xbbbbbbbb 0xcccccccc 0xdddddddd}
	global ABSTRCT_ERR
	puts [format "Testing memory write from addr = 0x%x, size:4 words" $start_addr]
	for {set i 0} {$i < $count} {incr i} {
		set addr [expr $start_addr + $i*4]
		set wdata [lindex $wdata_list $i]
		write_memory_word $tap $addr $wdata
		if {$ABSTRCT_ERR} {
			puts "write memory error"
			return
		}
	}

	puts [format "Testing memory read addr = 0x%x, size:4 words" $start_addr]
	for {set i 0} {$i < $count} {incr i} {
		set addr [expr $start_addr + $i*4]
		set wdata [lindex $wdata_list $i]

		set rdata [read_memory_word $tap $addr]
		if {$ABSTRCT_ERR} {
			puts "read memory error"
			return
		}
		#assert {$rdata == $wdata } [format "read/write memory mismatch: addr=0x%x, wdata=0x%x, rdata=0x%x" $addr $wdata $rdata]
		if [ expr $rdata != $wdata ] {
			puts [format "read/write memory mismatch: addr=0x%x, wdata=0x%x, rdata=0x%x" $addr $wdata $rdata]
			return
		}
	}
	global test_memory_access_pass
	set test_memory_access_pass "PASS"
}

proc test_abstract_memory_rw {tap start_addr xlen} {
	set wdata_list {0xaaaaaaaaaaaaaaaa 0xbbbbbbbbbbbbbbbb 0xcccccccccccccccc 0xdddddddddddddddd}
	puts [format "Testing abstract block memory write addr from 0x%x, size:4 words" $start_addr]
	if {![abstract_write_block_memory $tap $xlen $start_addr $wdata_list]} {
		return
	}
	puts [format "Testing abstract block memory read addr from 0x%x, size:4 words" $start_addr]
	if {![abstract_read_block_memory $tap $xlen $start_addr $wdata_list]} {
		return
	}
	global test_memory_access_pass
	set test_memory_access_pass "PASS"
}

proc test_reset_and_halt_all_harts {tap hartstart hartcount} {

	if {$hartcount == 1} {
		puts "Testing reset_and_halt_one_hart"
		reset_and_halt_one_hart $tap $hartstart
	} else {
		puts "Testing reset_and_halt_all_harts"
		reset_and_halt_all_harts $tap $hartstart $hartcount
	}
	set hartmax [expr $hartstart + $hartcount]
	# wait 1 sconds
	after 1000
	global CSR_MNVEC
	global test_reset_and_debug_pass
	global scan_hart_nums
	global NDS_TARGETNAME
	for {set hartsel $hartstart} {$hartsel < $hartmax} {incr $hartsel} {
		if {[select_single_hart $tap $hartsel]} {
			break
		}
		set dmstatus [read_dmi_dmstatus $tap]
		set dmstatus_anyunavail [expr ($dmstatus>>12)&0x1]
		if {$dmstatus_anyunavail} {
				puts [format "core%d is unavailable" $hartsel]
				break;
		}

		scan [nds target_xlen] "%x" hartxlen
		set pc [read_dpc $tap $hartxlen]
		puts [format "Hart %d pc = 0x%x" $hartsel $pc]
		set mnvec [read_register $tap $hartxlen $CSR_MNVEC]
		if [ expr $pc != $mnvec ] {
			puts [format "pc != mnvec"]
			return
		}
	}
	set test_reset_and_debug_pass "PASS"
}

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

proc get_march_name {marchid} {
	#set arch_name_list {"N" "D" "E" "S" "" "" "" "" "NX" "DX" "EX" "SX"}
	#set name_idx [expr ($marchid>>12)&0xf]
	#set march_name [lindex $arch_name_list $name_idx]
	#set march_name_out [format "%s%d%d" $march_name [expr ($marchid>>4)&0xf] [expr ($marchid>>0)&0xf] ]
	##puts [format "march_name_1 %s" $march_name_out]
	set arch_name_list {"N" "" "" "" "FS" "" "" "" "" "" "A" "" "" "D"}
	set name_idx [expr ($marchid>>8)&0xF]
	set march_name [lindex $arch_name_list $name_idx]
	if { [expr ($marchid>>15)&0x1] == 1} {
		set march_name_out [format "%sX%d%d" $march_name [expr ($marchid>>4)&0xf] [expr ($marchid>>0)&0xf] ]
	} else {
		set march_name_out [format "%s%d%d" $march_name [expr ($marchid>>4)&0xf] [expr ($marchid>>0)&0xf] ]
	}
	return $march_name_out
}

proc get_platform_name {platformid} {
	set platform_name_out [format "%c%c%d%d%d" [expr ($platformid>>24)&0xff] [expr ($platformid>>16)&0xff] [expr ($platformid>>12)&0xf] [expr ($platformid>>8)&0xf] [expr ($platformid>>4)&0xf]]
	return $platform_name_out
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

parsing_targets

set time_start [clock seconds]
set time_end   [clock seconds]
while {[expr $time_end-$time_start] < $time_target_sec} {
	set test_dm_operate_pass "NG"
	set test_dtm_connect_pass "NG"
	set test_memory_access_pass "NG"
	set test_reset_and_debug_pass "NG"
	set test_frequency_pass "NG"

	init
	set targetcount $NDS_TARGETS_COUNT
	for {set i 0} {$i < $targetcount} {incr $i} {
		targets $NDS_TARGETS_NAME($i)
		scan [nds jtag_tap_name] "%s" NDS_TAP
		puts [format "target-%d = %s 0x%x 0x%x" $i $NDS_TARGETS_NAME($i) $NDS_TARGETS_COREID($i) $NDS_TARGETS_CORENUMS($i)]

		puts "Initializing Debug Module Interface"
		init_dmi $NDS_TAP

		puts "Initializing Debug Module"
		reset_dm $NDS_TAP
		reset_ndm $NDS_TAP
		set test_dm_operate_pass "PASS"

		puts "Scanning number of harts"
		set scan_hart_nums [scan_harts $NDS_TAP]
		puts [format "scan_hart_nums=0x%x" $scan_hart_nums]
		if [ expr $scan_hart_nums > 0 ] {
			set test_dtm_connect_pass "PASS"
		} else {
			exit
		}

		set test_frequency_pass [test_frequency $NDS_TAP]

		set hartcount $NDS_TARGETS_CORENUMS($i)
		set hartmax [expr $NDS_TARGETS_COREID($i) + $hartcount]
		for {set hartsel $NDS_TARGETS_COREID($i)} {$hartsel < $hartmax} {incr $hartsel} {
			if {[select_single_hart $NDS_TAP $hartsel]} {
				break
			}
			set dmstatus [read_dmi_dmstatus $NDS_TAP]
			set dmstatus_anyunavail [expr ($dmstatus>>12)&0x1]
			if {$dmstatus_anyunavail} {
					puts [format "core%d is unavailable" $hartsel]
					break;
			}
			puts [format "Halting Hart %d" $hartsel]
			halt_hart $NDS_TAP $hartsel

			scan [nds target_xlen] "%x" hartxlen
			puts [format "core%d: target_xlen = 0x%x" $hartsel $hartxlen]

			set mvendorid [read_register $NDS_TAP $hartxlen $CSR_MVENDORID]
			puts [format "core%d: mvendorid=0x%x" $hartsel $mvendorid]
			set marchid [read_register $NDS_TAP $hartxlen $CSR_MARCHID]
			set march_name [get_march_name $marchid]
			puts [format "core%d: marchid=0x%x %s" $hartsel $marchid $march_name]

			set mimpid [read_register $NDS_TAP $hartxlen $CSR_MIMPID]
			puts [format "core%d: mimpid=0x%x" $hartsel $mimpid]
			set misa [read_register $NDS_TAP $hartxlen $CSR_MISA]
			puts [format "core%d: misa=0x%x" $hartsel $misa]
			set dcsr [read_register $NDS_TAP $hartxlen $CSR_DCSR]
			puts [format "core%d: dcsr=0x%x" $hartsel $dcsr]
			set mnvec [read_register $NDS_TAP $hartxlen $CSR_MNVEC]
			puts [format "core%d: mnvec=0x%x" $hartsel $mnvec]
			set pc [read_dpc $NDS_TAP $hartxlen]
			puts [format "core%d: pc = 0x%x" $hartsel $pc]

			set abstractcs [read_dmi_abstractcs $NDS_TAP]
			set debug_buffer_size [expr ($abstractcs>>24)&0x1f]
			puts [format "core%d: debug_buffer_size=0x%x" $hartsel $debug_buffer_size]

			set regaddr 0xF0100000
			if [ expr $debug_buffer_size > 7 ] {
				set rdata [read_memory_word $NDS_TAP $regaddr]
				if {$ABSTRCT_ERR} {
					puts [format "read SMU failed, maybe testing board's SMU register addrress is not 0x%x" $regaddr]
				}
			} else {
				set rdata [expr [abstract_read_memory $NDS_TAP $hartxlen $regaddr] & 0xFFFFFFFF]
			}
			if {$ABSTRCT_ERR == 0} {
				set platform_name [get_platform_name $rdata]
				puts [format "REG_SMU=0x%x %s" $rdata $platform_name]
			}

			if {$NDS_MEM_TEST == 1} {
				if [ expr $debug_buffer_size > 7 ] {
					test_memory_rw $NDS_TAP $NDS_MEM_ADDR
				} else {
					test_abstract_memory_rw $NDS_TAP $NDS_MEM_ADDR $hartxlen
				}
			} else {
				set test_memory_access_pass "SKIP"
			}
		}
		set hartsel $NDS_TARGETS_COREID($i)
		set hartcount $NDS_TARGETS_CORENUMS($i)
		test_reset_and_halt_all_harts $NDS_TAP $hartsel $hartcount
	}

	# dmi_busy_delay_count default value: 3
	set default_delay 3
	set max_delay $default_delay
	if {$NDS_MEM_TEST == 1 && $test_memory_access_pass == "PASS"} {
		puts [format "write 4 words from memory:0x%x to get dmi_busy_delay_count" $NDS_MEM_ADDR]
		nds configure scan_retry_times 3
		nds configure jtag_scans_optimize 4
		nds configure jtag_max_scans 64
		mww $NDS_MEM_ADDR 1 4
		scan [nds get_dmi_delay] "%x" delay_count
		if {$delay_count > $max_delay} {
			set max_delay $delay_count
			print_debug_msg [format "cpu mode dmi_busy_delay_count:%d" $delay_count]
		}
		#bus mode mww
		dma_mww $NDS_MEM_ADDR 1 4
		scan [nds get_dmi_delay] "%x" delay_count
		if {$delay_count > $max_delay} {
			set max_delay $delay_count
			print_debug_msg [format "bus mode dmi_busy_delay_count:%d" $delay_count]
		}
	}

	puts [format "********************"]
	puts [format "Diagnostic Report"]
	puts [format "********************"]

	puts [format "(%s) check changing the JTAG frequency ..." $test_frequency_pass]
	puts [format "(%s) check JTAG/DTM connectivity ..." $test_dtm_connect_pass]
	puts [format "(%s) check that Debug Module (DM) is operational ..." $test_dm_operate_pass]
	puts [format "(%s) check reset-and-debug ..." $test_reset_and_debug_pass]
	#puts [format "(%s) check that Program buffer and CPU domain are operational ..." $test_pbuf_work_pass]
	puts [format "(%s) check accessing memory through CPU ..." $test_memory_access_pass]
	if {$max_delay > $default_delay} {
		puts [format "suggest starting ICEman with --dmi_busy_delay_count %d" $max_delay]
	}
	puts [format "********************"]

	set time_end [clock seconds]
}

exit

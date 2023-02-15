#
# Andes
#
# http://www.andestech.com
#

# TAP_ARCH: 1->v3, 2->v5, 3->v3_sdm, 4->others
set target_arch_name {"unknown" "nds32_v3" "nds_v5" "nds32_v3_sdm" "others"}
set tap_arch_list {1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0}
set target_arch_list {1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0}
set target_group {1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0}

# for multi-core(N8_N25)
#set tap_arch_list {1 2 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0}
#set target_arch_list {1 2 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0}

# user can specify irlen & expected_id here,
# otherwise the default is V3(4, 0x1000063D), V5(5, 0x1000563D) V3(4, 0x1000163D)
set tap_irlen {0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0}
set tap_expected_id {0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0}

#Create a new TAP instance named basename.tap_type => dotted_name
set TAP_BASE "nds"
set TAP_TYPE "tap"

# user can specify core_id & tap_position here,
# otherwise is in order
set target_core_id {0xff 0xff 0xff 0xff 0xff 0xff 0xff 0xff 0xff 0xff 0xff 0xff 0xff 0xff 0xff 0xff 0xff 0xff 0xff 0xff 0xff 0xff 0xff 0xff 0xff 0xff 0xff 0xff 0xff 0xff 0xff 0xff}
set target_tap_position {0xff 0xff 0xff 0xff 0xff 0xff 0xff 0xff 0xff 0xff 0xff 0xff 0xff 0xff 0xff 0xff 0xff 0xff 0xff 0xff 0xff 0xff 0xff 0xff 0xff 0xff 0xff 0xff 0xff 0xff 0xff 0xff}
# for multi-core(N8_N25)
#set target_core_id {0 0 0xff 0xff 0xff 0xff 0xff 0xff 0xff 0xff 0xff 0xff 0xff 0xff 0xff 0xff 0xff 0xff 0xff 0xff 0xff 0xff 0xff 0xff 0xff 0xff 0xff 0xff 0xff 0xff 0xff 0xff}

# user can specify smp setting here,
# TARGET_SMP: 0->non-smp, 1->smp
set target_smp_list {0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0}
set target_smp_core_nums {0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0}

# user can specify target_name here,
set user_def_target_name 1

set max_of_tap 32
set max_of_target 32
set number_of_tap 0
set number_of_target 0
set tap_position 0
set target_coreid 0

for {set i 0} {$i < $max_of_tap} {incr i} {
	set TAP_ARCH($i) [lindex $tap_arch_list $i]
	if [ expr $TAP_ARCH($i) == 0x0 ] {
		break;
	}
	set number_of_tap [expr $number_of_tap + 1]
	if [ expr $TAP_ARCH($i) == 0x1 ] {
		set TAP_IRLEN($i) 4
		set TAP_EXP_CPUID($i)  0x1000063D
	} elseif [ expr $TAP_ARCH($i) == 0x02 ] {
		set TAP_IRLEN($i) 5
		set TAP_EXP_CPUID($i)  0x1000563D
	} else if [ expr $TAP_ARCH($i) == 0x3 ] {
		set TAP_IRLEN($i) 4
		set TAP_EXP_CPUID($i)  0x1000163D
	} else {
		set TAP_IRLEN($i) 4
		set TAP_EXP_CPUID($i)  0x1000063D
	}
	set user_irlen [lindex $tap_irlen $i]
	if ![ expr $user_irlen == 0 ] {
		set TAP_IRLEN($i) $user_irlen
	}
	set user_expected_id [lindex $tap_expected_id $i]
	if ![ expr $user_expected_id == 0 ] {
		set TAP_EXP_CPUID($i)  $user_expected_id
	}
}

for {set i 0} {$i < $max_of_target} {incr i} {
	set TARGET_ARCH($i) [lindex $target_arch_list $i]
	if [ expr $TARGET_ARCH($i) == 0x0 ] {
		break;
	}
	set number_of_target [expr $number_of_target + 1]
	set TARGET_ARCH_NAME($i) [lindex $target_arch_name $TARGET_ARCH($i)]
	set CORE_ID($i) [lindex $target_core_id $i]
	if [ expr $CORE_ID($i) == 0xff ] {
		set CORE_ID($i) $target_coreid
	}

	set IF_SMP($i) [lindex $target_smp_list $i]
	if [ expr $TARGET_ARCH($i) == 0x1 ] {
		set TARGET_NAME($i) "nds32.cpu$i"
	} elseif [ expr $TARGET_ARCH($i) == 0x02 ] {
		set TARGET_NAME($i) "nds_v5.cpu$i"
	} else {
		set TARGET_NAME($i) "nds.cpu$i"
	}
	set position [lindex $target_tap_position $i]
	if [ expr $position == 0xff ] {
		set position $tap_position
	}
	if [ expr $user_def_target_name == 0x1 ] {
		set target_name_1st "tap$position"
		set target_name_2nd "_target_$CORE_ID($i)"
		set TARGET_NAME($i) "$target_name_1st$target_name_2nd"
	}
	set CHAIN_POSITION($i) $TAP_BASE.$TAP_TYPE$position
	set tap_position [expr $tap_position + 1]
	set target_coreid [expr $target_coreid + 1]
}

for {set i 0} {$i < $number_of_tap} {incr i} {
    jtag newtap $TAP_BASE $TAP_TYPE$i -expected-id $TAP_EXP_CPUID($i) -irlen $TAP_IRLEN($i)
}

for {set i 0} {$i < $number_of_target} {incr i} {
	#target create $TARGET_NAME($i) $TARGET_ARCH_NAME($i) -endian little -chain-position $CHAIN_POSITION($i) -coreid $CORE_ID($i) -group [expr {$CORE_ID($i)+1}]
	target create $TARGET_NAME($i) $TARGET_ARCH_NAME($i) -endian little -chain-position $CHAIN_POSITION($i) -coreid $CORE_ID($i) -group [lindex $target_group $i]
	if [ expr $IF_SMP($i) == 0x1 ] {
		set CORE_NUMS($i) [lindex $target_smp_core_nums $i]
		$TARGET_NAME($i) configure -rtos riscv -corenums $CORE_NUMS($i)
	}
}

reset_config srst_only

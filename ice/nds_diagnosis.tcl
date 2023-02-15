source [find dmi.tcl]
source [find nds_diagnosis_funcs.tcl]
#set NDS_TAP nds_v5.cpu
set NDS_TAP "123"

scan [jtag names] "%s" NDS_TAP
puts [format "get jtag name = %s" $NDS_TAP]

puts [format "********************"]
puts [format "Diagnostic Report"]
puts [format "********************"]

# 1. "check changing the JTAG frequency ..."
set test_frequency_pass [test_frequency $NDS_TAP]

# 2. "check JTAG/DTM connectivity ..."
set test_dtm_connect_pass [test_dtm_connect $NDS_TAP]

# 3. "check that Debug Module (DM) is operational ..."
set test_dm_operate_pass [test_dm_operate $NDS_TAP]

# 4. "check reset-and-debug ..."
set test_reset_and_debug_pass [test_reset_and_debug $NDS_TAP]

# 5. "check that Program buffer and CPU domain are operational ..."
set test_pbuf_work_pass [test_pbuf_work $NDS_TAP]

# 6. "check accessing memory through CPU ..."
set test_memory_access_pass [test_memory_access $NDS_TAP]


puts [format "********************"]
#set nds_if_pass "PASS"
#set nds_if_pass "NG"
#set nds_if_pass "UNTESTED"

puts [format "(%s) check changing the JTAG frequency ..." $test_frequency_pass]
puts [format "(%s) check JTAG/DTM connectivity ..." $test_dtm_connect_pass]
puts [format "(%s) check that Debug Module (DM) is operational ..." $test_dm_operate_pass]
puts [format "(%s) check reset-and-debug ..." $test_reset_and_debug_pass]
puts [format "(%s) check that Program buffer and CPU domain are operational ..." $test_pbuf_work_pass]
puts [format "(%s) check accessing memory through CPU ..." $test_memory_access_pass]
puts [format "********************"]



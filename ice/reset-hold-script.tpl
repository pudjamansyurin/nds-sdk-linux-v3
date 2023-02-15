
set_current_target tap0_target_0
t_write_misc 0 2 4
delay 500
t_write_misc 0 2 0
delay 500
t_write_misc 0 2 5
delay 500

set_current_target tap1_target_0
reset_and_halt_current_hart 0
#set_srst 256
#set_dbgi 72
#clear_dbgi 10
#clear_srst 500

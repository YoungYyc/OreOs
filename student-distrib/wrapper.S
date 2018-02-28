.data

	LOWEST_SYS_CALL_NUM = 1

	HIGHEST_SYS_CALL_NUM = 10


.text

.globl keyboard_handler_wrapper, mouse_handler_wrapper, rtc_handler_wrapper, system_call_handler_wrapper, pit_handler_wrapper

# wrapper function for keyboard handler
# reason for wrapper: https://www-s.acm.illinois.edu/sigops/2007/roll_your_own/i386/idt.html
keyboard_handler_wrapper:
	pushfl # push all flags

	# save all registers
	pushl %eax
	pushl %ebx
	pushl %ecx
	pushl %edx
	pushl %esi
	pushl %edi
	pushl %ebp
	pushl %esp
	
	call keyboard_handler
	
	# restore all registers
	popl %esp
	popl %ebp
	popl %edi
	popl %esi
	popl %edx
	popl %ecx
	popl %ebx
	popl %eax

	popfl # pop all flags

	iret

mouse_handler_wrapper:
	pushfl # push all flags

	# save all registers
	pushl %eax
	pushl %ebx
	pushl %ecx
	pushl %edx
	pushl %esi
	pushl %edi
	pushl %ebp
	pushl %esp
	
	call mouse_handler
	
	# restore all registers
	popl %esp
	popl %ebp
	popl %edi
	popl %esi
	popl %edx
	popl %ecx
	popl %ebx
	popl %eax

	popfl # pop all flags

	iret

# wrapper function for rtc handler
# reason for wrapper: https://www-s.acm.illinois.edu/sigops/2007/roll_your_own/i386/idt.html
pit_handler_wrapper:
	pushfl # push all flags
	
	# save all registers
	pushl %eax
	pushl %ebx
	pushl %ecx
	pushl %edx
	pushl %esi
	pushl %edi
	pushl %ebp
	pushl %esp
	
	call pit_handler
	
	# restore all registers
	popl %esp
	popl %ebp
	popl %edi
	popl %esi
	popl %edx
	popl %ecx
	popl %ebx
	popl %eax

	popfl # pop all flags

	iret


# wrapper function for rtc handler
# reason for wrapper: https://www-s.acm.illinois.edu/sigops/2007/roll_your_own/i386/idt.html
rtc_handler_wrapper:
	pushfl # push all flags
	
	# save all registers
	pushl %eax
	pushl %ebx
	pushl %ecx
	pushl %edx
	pushl %esi
	pushl %edi
	pushl %ebp
	pushl %esp
	
	call rtc_handler
	
	# restore all registers
	popl %esp
	popl %ebp
	popl %edi
	popl %esi
	popl %edx
	popl %ecx
	popl %ebx
	popl %eax

	popfl # pop all flags

	iret


system_call_handler_wrapper:

	cmpl $LOWEST_SYS_CALL_NUM, %eax
	jl invalid_sys_call_number # invalid sys call arg in eax

	cmpl $HIGHEST_SYS_CALL_NUM, %eax
	jg invalid_sys_call_number # invalid sys call arg in eax

	# otherwise, continue with syscall:

	pushfl # push all flags

	# save all registers except eax
	pushl %ebx
	pushl %ecx
	pushl %edx
	pushl %esi
	pushl %edi
	pushl %ebp
	pushl %esp

	# push 3 arguments to sys call
	pushl %edx
	pushl %ecx
	pushl %ebx

	call *system_call_jumptable(,%eax,4) # call appropriate sys call using jumptable
	
	# discard 3 arguments to sys call
	addl $12, %esp # 12 is just 4 * 3: no need to define magic constant for this probably

	jmp finish_sys_call

invalid_sys_call_number:
	
	movl $-1, %eax # return error if invalid sys call number

	jmp done_sys_call # iret

finish_sys_call:

	# restore all registers except eax
	popl %esp
	popl %ebp
	popl %edi
	popl %esi
	popl %edx
	popl %ecx
	popl %ebx

	popfl # pop all flags

done_sys_call:
	
	iret

system_call_jumptable:
	.long 0x0, sys_halt, sys_execute, sys_read, sys_write, sys_open, sys_close, sys_getargs, sys_vidmap, sys_set_handler, sys_sigreturn

rtc_jumptable:
	.long 0x0, read_rtc, write_rtc, open_rtc, close_rtc

file_jumptable:
	.long 0x0, file_read, file_write, file_open, file_close

dir_jmptable:
	.long 0x0, dir_read, dir_write, dir_open, dir_close



global int_ignore
global int_fatal
extern cpu_halt
section .text
int_ignore:
;;;  XXX two functions from interrupt.h, temporary
        iret
int_fatal:
;;;  XXX temporary solution
	call	cpu_halt

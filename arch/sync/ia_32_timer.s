global timer_disable
global timer_restore
section	.text
timer_disable:
	cli
lock	inc	word [timer_count]
	;; disable the timer here also to be sure
	sti
	ret
timer_restore:
lock	dec	word [timer_count]
	jnz	.end
	;; restore the timer
.end
	ret
section	.data
timer_count:
	dd	0,0,0,0
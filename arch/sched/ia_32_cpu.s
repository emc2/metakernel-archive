global current_cpu
global cpu_halt
global ipi_send
global ipi_stop
global ipi_resched
global ipi_restart
global ipi_restart_tlbinv
global ipi_restart_kern_tlbinv
global ipi_halt
section	.text
current_cpu:
	xor	eax,eax
	mov	al,byte [local_apic_id]
	ret
ipi_stop:
	push	eax
	xor	eax,eax
.wait
	cmp	eax,0
	je	.wait
	pop	eax
	iret
ipi_restart:
	;; NOT idempotent
	mov	eax,1
	iret
ipi_restart_tlbinv:
	;; feel the sting...
	mov	eax,cr3
	mov	cr3,eax
	mov	eax,1
	iret
ipi_restart_kern_tlbinv:
	;; feel the sting even worse...
	mov	eax,cr0
	and	eax,0xffffff7f
	mov	cr0,eax
	mov	eax,cr3
	mov	cr3,eax
	mov	eax,cr0
	or	eax,0x00000080
	mov	cr0,eax
	mov	eax,1
	iret
cpu_halt:
ipi_halt:
	cli
.halt
	hlt
	jmp	.halt
section	.bss
local_apic_id:
	resd	1
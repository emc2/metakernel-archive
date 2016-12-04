global context_load
global addr_spc_load
section	.text
addr_spc_load:
	push	eax
	mov	eax,[esp+12]
	add	eax,0x1000
	mov	cr3,eax
	pop	eax
	ret
	;; The IPI or timer interrupt handler
	;; shoves everything onto the stack,
	;; and handles return redirection.
context_load:
	;; ok to kill the registers, we're never coming back...
	mov	esi,[esp+12]
	mov	eax,[esi]
	mov	esp,[esi+4]
	jmp	eax
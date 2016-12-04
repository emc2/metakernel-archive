global	round_down
global	round_up
global	addr_power
global	addr_size
global	ilog
section	.text
addr_size:
	push	ebx
	push	edx
	mov	edx,[esp+12]
	bsf	ebx,edx
	shl	ebx,2
	mov	eax,[size_tab+ebx]
	pop	edx
	pop	ebx
	ret
round_down:
	push	ebx
	push	edx
	mov	edx,[esp+12]
	bsr	ebx,edx
	shl	ebx,2
	mov	eax,[size_tab+ebx]
	pop	edx
	pop	ebx
	ret
round_up:
	push	ebx
	push	edx
	mov	edx,[esp+12]
	sub	edx,1
	bsr	ebx,edx
	add	ebx,1
	shl	ebx,2
	mov	eax,[size_tab+ebx]
	pop	edx
	pop	ebx
	ret
ilog:
addr_power:
	push	edx
	mov	edx,[esp+8]
	bsr	eax,edx
	pop	edx
	ret
section	.rodata
size_tab:
	dd	0x00000001
	dd	0x00000002
	dd	0x00000004
	dd	0x00000008
	dd	0x00000010
	dd	0x00000020
	dd	0x00000040
	dd	0x00000080
	dd	0x00000100
	dd	0x00000200
	dd	0x00000400
	dd	0x00000800
	dd	0x00001000
	dd	0x00002000
	dd	0x00004000
	dd	0x00008000
	dd	0x00010000
	dd	0x00020000
	dd	0x00040000
	dd	0x00080000
	dd	0x00100000
	dd	0x00200000
	dd	0x00400000
	dd	0x00800000
	dd	0x01000000
	dd	0x02000000
	dd	0x04000000
	dd	0x08000000
	dd	0x10000000
	dd	0x20000000
	dd	0x40000000
	dd	0x80000000
;; Copyright (c) 2004 Eric McCorkle.  All rights reserved.
global thread_start
global mp_entry
global idt_desc
extern kern_page_dir
extern idt
extern gdt
extern tss_array
extern static_alloc
extern thread_sched_static_init
section	.text
thread_start:
	add	esp,-40
	mov	[esp+24],eax
	mov	[esp+28],ebx
	mov	[esp+32],ecx
	mov	[esp+36],edx
	;; First, I need the APIC, so modify the interim
	;; page table.
	mov	[0x100fec],dword 0xfec00093
	;; install a trampoline at 0x1000
	mov	[0x1000],dword 0x007c00ea
	mov	[0x1004],dword 0x00000000
	;; install a gdt descriptor at 0x1008
	mov	[0x100a],word 39
	mov	[0x100c],dword 0x2000
	;; copy mp_init to 0x7c00
	mov	ebx,mp_init
	xor	ecx,ecx
	mov	edx,mp_init.end
	sub	edx,ebx
.copy
	mov	al,[ebx+ecx]
	mov	[0x7c00+ecx],al
	add	ecx,1
	cmp	ecx,edx
	jne	.copy
	;; send an sipi interrupt to everyone but me
	mov	[0xfee00310],dword 0xff000000
	mov	[0xfee00300],dword 0x000c4601
	;; change my APIC ID
	;; XXX omitted because it causes problems.  Suspect bug in bochs
;	mov	[0xfee00020],dword 0
	;; give'em about a million cycles to take
	;; care of business...
	mov	ecx,0x100000
.wait
	add	ecx,-1
	jnz	.wait
	;; now continue and set up the permanent gdt
	mov	edx,[cpu_count]
	mov	ebx,edx
	add	ebx,5
	shl	ebx,3
	mov	[esp],ebx
	mov	[esp+4],dword 16
	mov	[esp+8],eax
	mov	[esp+12],ebx
	mov	[esp+16],ecx
	mov	[esp+20],edx
	call	static_alloc
	mov	ebx,[esp+12]
	mov	ecx,[esp+16]
	mov	edx,[esp+20]
	mov	[gdt],eax
	mov	[eax],dword 0x00000000
	mov	[eax+4],dword 0x00000000
	mov	[eax+8],dword 0x0000ffff
	mov	[eax+12],dword 0x00cf9a00
	mov	[eax+16],dword 0x0000ffff
	mov	[eax+20],dword 0x00cf9200
	mov	[eax+24],dword 0x0000ffff
	mov	[eax+28],dword 0x00cffa00
	mov	[eax+32],dword 0x0000ffff
	mov	[eax+36],dword 0x00cff200
	;; Allocate TSS segments.  The segments are
	;; actually 0x80 in size, so they fit nicely
	;; into pages.
	mov	eax,edx
	shl	eax,7
	mov	[esp],eax
	mov	[esp+4],dword 0x1000
	mov	[esp+8],eax
	mov	[esp+12],ebx
	mov	[esp+16],ecx
	mov	[esp+20],edx
	call	static_alloc
	mov	ebx,[esp+12]
	mov	ecx,[esp+16]
	mov	edx,[esp+20]
	;; get their physical address
	mov	[tss_array],eax	
	mov	ecx,[cpu_count]
	shl	ecx,7
	mov	edx,[cpu_count]
	shl	edx,3
	add	edx,[gdt]
	add	edx,40
	;; eax now contains the physical addresses
.tss_loop
	sub	edx,8
	sub	ecx,0x80
	mov	ebx,eax
	add	ebx,ecx
	shl	ebx,16
	or	ebx,0x67
	mov	[edx],ebx
	mov	[edx+4],dword 0x00008900
	;; XXX assumes TSS table is no larger than
	;; 64k (no more than 512 CPUs)
	mov	ebx,eax
	shr	ebx,16
	mov	[edx+4],bl
	mov	[edx+7],bh
	cmp	ecx,0
	jnz	.tss_loop
	;; set up the gdt descriptors
	mov	edx,[cpu_count]
	add	edx,5
	shl	edx,3
	add	edx,-1
	mov	[gdt_desc],dx
	mov	eax,[gdt]
	mov	[gdt_desc+2],eax
	;; tss is now up and running, load my task register
	;; and gdt register
	lgdt	[gdt_desc]
	mov	ax,0x0028
	ltr	ax
	;; send an IPI_RESTART signal
	mov	[0xfee00310],dword 0xff000000
	mov	[0xfee00300],dword 0x000c4032
	mov	ebx,[cpu_count]
	mov	[esp],ebx
	call	thread_sched_static_init
	ret
mp_init:
	;; This code goes to 0x7c00 its sole purpose is
	;; to jump to mp_start
	xor	bx,bx
	mov	ds,bx
	mov	es,bx
	mov	fs,bx
	mov	gs,bx
;;;  turn on A20
.clear1
	in	al,0x64	;  wait for it to be ready
	test	al,0x2
	jnz	.clear1
	mov	al,0xd1	;  write command
	out	0x64,al
.clear2
	in	al,0x64	;  wait...
	test	al,0x2
	jnz	.clear2
	mov	al,0xdf
	out	0x60,al	;  A20 on
.clear3
	in	al,0x64
	test	al,0x2
	jnz	.clear3
[BITS 16]
	mov	di,0x100a
	;; protected mode on
	lgdt	[di]
	mov	eax,cr0
	or	eax,1
	mov	cr0,eax
	jmp	dword 0x0008:0x00007c00+(.prot-mp_init)
[BITS 32]
.prot
	mov	bx,0x0010
	mov	ds,bx
	mov	es,bx
	mov	fs,bx
	mov	gs,bx
	mov	ss,bx
	;; load page directory register
	mov	ebx,0x100000
	mov	cr3,ebx
	;; superpages on
	mov	ebx,cr4
	or	ebx,0x10
	mov	cr4,ebx
	;; paging on
	or	eax,0x80000000
	mov	cr0,eax
	lea	edi,[mp_start]
	jmp	edi
.end
mp_start:
	lidt	[idt_desc]
	sti
	;; now time to reprogram the APIC ID's
	mov	ebx,1
lock	xadd	[cpu_count],ebx
	shl	ebx,24
	;; XXX see above.
; 	mov	[0xfee00020],ebx
	;; get me a stack based on my APIC ID
	shr	ebx,8
	mov	esp,ebx
	add	esp,0x20fffc
	shr	ebx,16
	;; simulate an ipi_stop
	mov	eax,0
.stop1
	cmp	eax,0
	je	.stop1
	lgdt	[gdt_desc]
	mov	eax,ebx
	add	ax,5
	shl	ax,3
	;; load up the task register
	ltr	ax
	;; stop again
	mov	eax,0
	mov	ebx,[0xfee00020]
.stop2
	cmp	eax,0
	je	.stop2
section .rodata
idt_desc:
	dw	0x1fff
	dd	idt
section	.data
cpu_count:
	dd	1
section	.bss
gdt_desc:
	resw	1
	resd	1

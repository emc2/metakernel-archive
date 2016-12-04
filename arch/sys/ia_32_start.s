extern kern_end
extern static_alloc_init
extern thread_start
extern kern_page_dir
extern idt
extern gdt
extern idt_init
extern idt_desc
extern cpuid_parse
extern mem_static_init
extern bss_start
extern bss_end
global start
section	.text
	;; Life is looking pretty bleak at this point...
	;; Low memory is a mess, and we're on a temporary
	;; stack and page table.  We've got a BIOS memory
	;; map at 0x7e00, a temporary page table at 0x100000
	;; and a stack running from 0x10000 to 0x20000
start:
	;; save out some basic data
	mov	[bios_map],eax
	mov	[bios_map_size],ebx
	;; zero the bss
	lea	esi,[bss_start]
.zero
	mov	[esi],byte 0
	inc	esi
	cmp	esi,bss_end
	jne	.zero
	;; get a stack
	mov	esp,0x20fffc
	;; bring up the static allocator
	add	esp,-8
	lea	eax,[kern_end]
	mov	[esp],eax
	and	eax,0xffc00000
	add	eax,0x00400000
	mov	[esp+4],eax
	call	idt_init
	lidt	[idt_desc]
	pushfd
	or	[esp],dword 0x200000
	popfd
	call	cpuid_parse
	call	static_alloc_init
	;; The synchronization system has no state,
	;; so it is operational from the start
	;; The scheduler needs to know how many cpus I have.
	;; Its startup procedure is different depending on
	;; if I am running a single or multi cpu system
	call	thread_start
	;; The physical memory system needs to build the
	;; inverse page table and the page allocator tables.
	;; It needs the cache size and memory map to do this.
	mov	eax,[bios_map]
	mov	ebx,[bios_map_size]
	mov	[esp],eax
	mov	[esp+4],ebx
	call	mem_static_init
	;; The last static initialization is the dynamic
	;; allocator

	;; The "initialization" of the IO system is
	;; activating device drivers, which takes place
	;; just before initial thread creation
	call	halt
halt:
	hlt
	jmp	halt
section .bss
bios_map:
	resd	1
bios_map_size:
	resd	1
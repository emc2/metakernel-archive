global	current_addr_spc
global	arch_addr_spc_init
global	page_map
global	page_unmap
global	page_remap
global	kern_page_map
global	kern_page_unmap
extern	kern_addr_spc
section	.text
current_addr_spc:
	mov	eax,cr3
	and	eax,0xffffe000
	ret
arch_addr_spc_init:
	add	esp,-8
	mov	[esp+4],eax
	mov	[esp],ebx
	mov	eax,[esp+12]
	add	eax,0x1000
	mov	ebx,eax
	add	eax,0x2000
.loop1
	mov	[ebx],dword 0
	cmp	eax,ebx
	jne	.loop1
	mov	ebx,[esp]
	mov	eax,[esp+4]
	add	esp,8
	ret
page_map:
	add	esp,-12
	mov	[esp+8],eax
	mov	[esp+4],ebx
	mov	[esp],ecx
	mov	eax,[esp+16]
	mov	ebx,[esp+20]
	invlpg	[eax]
	add	eax,0x2000
	mov	ecx,ebx
	shr	ebx,20
	and	ebx,0x00000ffc
	add	eax,ebx
	shr	ecx,10
	and	ecx,0x00000ffc
	add	ecx,[eax]
	mov	eax,[esp+28]
	mov	ebx,[esp+24]
	and	eax,0xfffff000
	and	ebx,0x00000002
	or	eax,ebx
	or	eax,0x00000005
	mov	[ecx],eax
	mov	ecx,[esp]
	mov	ebx,[esp+4]
	mov	eax,[esp+8]
	add	esp,12
	ret
page_unmap:
	add	esp,-12
	mov	[esp+8],eax
	mov	[esp+4],ebx
	mov	[esp],ecx
	mov	eax,[esp+16]
	mov	ebx,[esp+20]
	invlpg	[eax]
	add	eax,0x2000
	mov	ecx,ebx
	shr	ebx,20
	and	ebx,0x00000ffc
	add	eax,ebx
	shr	ecx,10
	and	ecx,0x00000ffc
	add	ecx,[eax]
	mov	[ecx],dword 0
	mov	ecx,[esp]
	mov	ebx,[esp+4]
	mov	eax,[esp+8]
	add	esp,12
	ret
page_remap:
        add	esp,-12
	mov	[esp+8],eax
	mov	[esp+4],ebx
	mov	[esp],ecx
	mov	eax,[esp+16]
	mov	ebx,[esp+20]
	invlpg	[eax]
	add	eax,0x2000
	mov	ecx,ebx
	shr	ebx,20
	and	ebx,0x00000ffc
	add	eax,ebx
	shr	ecx,10
	and	ecx,0x00000ffc
	add	ecx,[eax]
	mov	ebx,[esp+24]
	mov	eax,[ecx]
	and	ebx,0x00000002
	and	eax,0xfffffffd
	or	eax,ebx
	mov	[ecx],eax
	mov	ecx,[esp]
	mov	ebx,[esp+4]
	mov	eax,[esp+8]
	add	esp,12
	ret
	;; XXX actually implement this
kern_page_map:
	ret
	
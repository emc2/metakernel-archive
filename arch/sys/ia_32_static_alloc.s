global static_alloc_expand
section .text
static_alloc_expand:
	add	esp,-8
	mov	[esp],eax
	mov	[esp+4],ebx
	mov	eax,[esp+8]
	mov	ebx,eax
	shr	ebx,20
	or	eax,0x00000083
	mov	[0x100000+ebx],eax
	mov	eax,[esp]
	mov	ebx,[esp+4]
	add	esp,8
	ret

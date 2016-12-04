global	parport_write_char
parport_write_char:
	add	esp,-8
	mov	[esp+4],edx
	mov	[esp],ecx
	mov	dx,[esp+16]
	add	dx,1
	mov	ecx,1000
.loop1
        in	al,dx
        and	al,0x80
        sub	ecx,1
        cmp	ecx,0
        je	.error
        cmp	al,0
	je	.loop1
	sub	dx,1
	mov	al,[esp+20]
	out	dx,al
	add	dx,2
	mov	al,0xd
	out	dx,al
	mov	al,0xc
	out	dx,al
	xor	eax,eax
.exit
	mov	ecx,[esp]
	mov	edx,[esp+4]
	add	esp,12
	ret
.error
	mov	eax,-1
	jmp	.exit
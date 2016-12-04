global cpuid_store
section	.text
cpuid_store:
	add	esp,-20
	mov	[esp],eax
	mov	[esp+4],ebx
	mov	[esp+8],ecx
	mov	[esp+12],edx
	mov	[esp+16],edi
	mov	eax,[esp+24]
	mov	edi,[esp+28]
	cpuid
	mov	[edi],eax
	mov	[edi+4],ebx
	mov	[edi+8],ecx
	mov	[edi+12],edx
	mov	eax,[esp]
	mov	ebx,[esp+4]
	mov	ecx,[esp+8]
	mov	edx,[esp+12]
	mov	edi,[esp+16]
	add	esp,20
	ret
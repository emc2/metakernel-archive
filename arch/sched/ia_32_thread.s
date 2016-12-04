global	current_thread
section	.text
current_thread:
	mov	eax,esp
	and	eax,0xffffc000
	ret
global	spin_lock
global	spin_unlock
global	spin_trylock
section	.text
spin_lock:
	mov	eax,[esp+8]
.loop
	bt	word [eax],0
	jc	.loop
lock	bts	word [eax],0
	pause
	jc	.loop
	ret
spin_unlock:
	mov	eax,[esp+8]
	mov	[eax],byte 0
	ret
spin_trylock:
	mov	eax,[esp+8]
lock	bts	word [eax],byte 0
	jc	.fail
	mov	eax,1
	ret
.fail
	xor	eax,eax
	ret
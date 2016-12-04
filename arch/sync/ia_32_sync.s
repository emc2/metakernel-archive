global wait_reentry
global lock_reentry
section	.text
wait_reentry:
lock_reentry:
	add	esp,4
	ret

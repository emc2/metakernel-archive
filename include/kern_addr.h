#ifndef KERN_ADDR_H
#define KERN_ADDR_H

/* Copyright (c) Eric McCorkle.  All rights reserved. */
/* pointers for the kernel address space */
#define kern_size ((char*)kern_addr_end -\
		   (char*)kern_addr_start)
#define kern_pages (kern_size / PAGE_SIZE)

extern void* kern_addr_start;
extern void* kern_addr_end;
extern void* kern_page_tables;
extern void* kern_heap_start;
extern void* kern_heap_end;

#endif

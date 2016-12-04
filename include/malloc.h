#ifndef MALLOC_H
#define MALLOC_H

/* Copyright (c) 2004 Eric McCorkle.  All rights reserved. */

void* kern_malloc(unsigned int* size, unsigned int pri);
void* kern_realloc(void* ptr, unsigned int* size,
		   unsigned int oldsize, unsigned int pri);
void kern_free(void* ptr, unsigned int size);
void kern_heap_init(void* start, unsigned int size);

void* malloc(unsigned int size);
void* realloc(void* ptr, unsigned int size);
void free(void* ptr);

extern unsigned int heap_size;

#endif

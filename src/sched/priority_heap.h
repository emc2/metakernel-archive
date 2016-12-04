#ifndef PRIORITY_HEAP_H
#define PRIORITY_HEAP_H

/* Copyright (c) 2004 Eric McCorkle.  All rights reserved. */

#include <thread.h>

#define priority_heap_remove(x) priority_heap_delete(x, 1)

typedef struct {

  unsigned int size;
  unsigned int max;
  thread_t** data_name;

} priority_heap;

priority_heap* priority_heap_init(unsigned int);
void priority_heap_grow(priority_heap*, unsigned int);
void priority_heap_insert(priority_heap*, thread_t*);
thread_t* priority_heap_head(priority_heap*);
thread_t* priority_heap_delete(priority_heap*, unsigned int);
void priority_heap_set_key(priority_heap*, unsigned int, unsigned int);

#endif

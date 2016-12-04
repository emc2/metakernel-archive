#ifndef AFFINITY_HEAP_H
#define AFFINITY_HEAP_H

/* Copyright (c) 2004 Eric McCorkle.  All rights reserved. */

#include <thread.h>

#ifdef MULTITHREAD

#define affinity_heap_remove(x) affinity_heap_delete(x, 1)

typedef struct {

  unsigned int size;
  unsigned int max;
  thread_t** data_name;

} affinity_heap;

affinity_heap* affinity_heap_init(unsigned int);
void affinity_heap_insert(affinity_heap*, thread_t*);
void affinity_heap_grow(affinity_heap*, unsigned int);
thread_t* affinity_heap_head(affinity_heap*);
thread_t* affinity_heap_delete(affinity_heap*, unsigned int);
void affinity_heap_set_key(affinity_heap*, unsigned int, unsigned int);

#endif

#endif

#include <definitions.h>
#include <inode.h>

/* Copyright (c) 2004 Eric McCorkle.  All rights reserved. */
/* STATUS swap.c: not begun */

#ifndef NO_SWAP

static mutex_t mutex;
static unsigned int num_swappers;
static unsigned int max_swappers;
static u_64* swappers;

static inline void resize_table () {

  unsigned int i;
  unsigned int new_size;

  new_size = size * 2;
  swappers = kern_realloc(swappers, &new_size, size, 0);

  for(i = size; i < new_size; i++) {

    (void*)(swappers[i]) = first_free;
    first_free = (void*)(swappers[i]);

  }

  max_swappers = new_size;

}

#endif

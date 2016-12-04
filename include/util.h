#ifndef UTIL_H
#define UTIL_H

/* Copyright (c) 2004 Eric McCorkle.  All rights reserved. */

#include <definitions.h>
#include <phys_mem.h>

#define memcpy(x, y, z) (memcpy_lo_hi((x), (y), (z)))
#define memmove(x, y, z) {\
  if((x) < (y))\
    memcpy_lo_hi((x), (y), (z));\
\
  else\
    memcpy_hi_lo((x), (y), (z));\
}

void memcpy_hi_lo(void*, const void*, unsigned int);
void memcpy_lo_hi(void*, const void*, unsigned int);
void memset(void*, unsigned char, unsigned int);
void phys_memcpy(phys_addr_t, phys_addr_t, unsigned int);
void phys_memset(phys_addr_t, unsigned char, unsigned int);
void strcpy(unichar* dst, const unichar* src);
int strcmp(const unichar* a, const unichar* b);
unsigned int strhash(const unichar* s);
u_64 rand_64();
u_32 rand_32();
unsigned int rand();

#endif

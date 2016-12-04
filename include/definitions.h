#ifndef DEFINITIONS_H
#define DEFINITIONS_H

/* Copyright (c) 2004 Eric McCorkle.  All rights reserved. */

#define NULL ((void*)0)
#define ia_32 0
#define x86_64 1
#define ia_64 2
#define ppc 3
#define alpha 4
#define arm 5

#define PTR_TO_UINT(x) ((char*)(x) - (char*)NULL)
#define UINT_TO_PTR(x) ((char*)NULL + (x))

#if ARCH == ia_32

#define CONTEXT_WORDS 2
#define PAGE_SIZE 0x1000
#define PAGE_MASK 0xfff
#define PAGE_POW 12
#define WORD_SIZE 4
#define CACHE_LINE_SIZE 16

typedef unsigned int word;
typedef unsigned char u_8;
typedef signed char s_8;
typedef unsigned short u_16;
typedef signed short s_16;
typedef unsigned int u_32;
typedef signed int s_32;
typedef unsigned long long u_64;
typedef signed long long s_64;

#ifdef PAE
typedef unsigned long long phys_addr_t;
#define phys_to_page(x) ((u_32)((x) / PAGE_SIZE))
#define NULL_PHYS_PAGE 0xffffffffffffffff
#else
typedef unsigned int phys_addr_t;
#define phys_to_page(x) ((x) / PAGE_SIZE)
#define NULL_PHYS_PAGE 0xffffffff
#endif

typedef unsigned int context_t[2];

#elif ARCH == aa_64

#error "AA-64 support not implemented"

#elif ARCH == ia_64

#error "Itanium support not implemented"

#elif ARCH == ppc

#error "Power PC support not implemented"

#elif ARCH == alpha

#error "Alpha support not implemented"

#elif ARCH == arm

#error "Arm support not implemented"

#else

#error "Unsupported architecture specification"

#endif

#ifdef OFFSET_64
typedef u_64 offset_t;
#else
typedef unsigned int offset_t;
#endif

typedef char page[PAGE_SIZE];
typedef char cache_line[CACHE_LINE_SIZE];
/* unicode character */
typedef u_16 unichar;

#endif

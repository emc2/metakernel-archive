#ifndef PAGE_ALLOC_H
#define PAGE_ALLOC_H

/* Copyright (c) 2004 Eric McCorkle.  All rights reserved. */

#include <definitions.h>
#include <phys_mem.h>

#ifdef PAGE_COLOR

extern unsigned int num_page_colors;

#define page_color(x) (((unsigned int)(x) / PAGE_SIZE) % num_page_colors)

phys_addr_t phys_page_alloc(unsigned int* color_tab);
phys_addr_t kern_page_alloc(unsigned int* color_tab);
void phys_page_free(unsigned int num, unsigned int* color_tab,
		    const phys_addr_t* addrs);
void change_color_count(unsigned int* color_tab,
			unsigned int color, int num);
void page_alloc_static_init(unsigned int cache_size);
#else
phys_addr_t phys_page_alloc();
phys_addr_t kern_page_alloc();
void phys_page_free(unsigned int num, const phys_addr_t* addrs);
void page_alloc_static_init(void);
#error "Non-coloring page allocator not implemented"
#endif

void phys_page_insert(unsigned int num, const phys_addr_t* addrs);

#endif

#ifndef PAGE_CACHE_H
#define PAGE_CACHE_H

/* Copyright (c) 2004 Eric McCorkle.  All rights reserved. */
phys_addr_t page_cache_lookup(unsigned int id, offset_t offset);
void page_cache_insert(phys_addr_t, unsigned int id, offset_t offset);
/* XXX page cache delete is not yet used properly,
 * see virt_mem.c
 */
void page_cache_delete(phys_addr_t page);

#endif

#ifndef ARCH_VMEM_H
#define ARCH_VMEM_H

/* Copyright (c) 2004 Eric McCorkle.  All rights reserved. */

#include <definitions.h>
#include <virt_mem.h>
#include <phys_mem.h>

void page_map(addr_spc_t* spc, void* addr,
	      unsigned int perm, phys_addr_t phys);
void page_unmap(addr_spc_t* spc, void* addr);
void page_remap(addr_spc_t* spc, void* addr, unsigned int perm);
phys_addr_t page_lookup_addr(addr_spc_t* src, void* addr);
void page_table_alloc(addr_spc_t* spc, void* start, void* end);
void kern_page_map(void* addr, unsigned int perm, phys_addr_t phys);
void kern_page_unmap(void* addr);
void arch_addr_spc_init(addr_spc_t* addr_spc);

#endif

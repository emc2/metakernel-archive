#include <definitions.h>
#include <data_structures.h>
#include <virt_mem.h>
#include <phys_mem.h>
#include <static_alloc.h>
#include "inv_page_table.h"

/* Copyright (c) 2004 Eric McCorkle.  All rights reserved. */
/* STATUS inv_page_table.c: compiles */

#define next(x) ((x)->inv_next)
#define un_eq(x, y) ((x) == (y))
#define un_id(x) ((x)->obj->id)
#define hash(x) (x)

STAT_HASH_TABLE(inv_map, , inv_map_t*, offset_t, next, NULL,
		un_eq, un_id, hash, INV_MAP_HASH_TABLE_SIZE)

#ifndef HASH_INV_PAGE

static inv_page_t* inv_page_table;

inv_page_t* inv_page_lookup(phys_addr_t addr) {

  return inv_page_table + phys_to_page(addr);

}


phys_addr_t inv_page_addr(inv_page_t* inv_page) {

  return (inv_page - inv_page_table) * PAGE_SIZE;

}


void inv_page_static_init(unsigned int pages) {

  inv_page_table = static_alloc(pages * sizeof(inv_page_t),
				sizeof(inv_page_t));

}

#else
#error "Inverse page table hashing not implemented"
#endif

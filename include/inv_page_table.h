#ifndef INV_PAGE_TABLE_H
#define INV_PAGE_TABLE_H

/* Copyright (c) 2004 Eric McCorkle.  All rights reserved. */
/* this file is the inverse page table interface */

#include <definitions.h>
#include <virt_mem.h>
#include <phys_mem.h>
#include <sync.h>

#define PAGE_UNUSED 1
#define PAGE_USED 2
#define PAGE_RESERVED 3
#define PAGE_KERNEL 4
#define INV_MAP_HASH_TABLE_SIZE 29

typedef struct inv_map_t {

  vm_obj_t* obj;
  unsigned int offset;
  struct inv_map_t* inv_next;

} inv_map_t;

STAT_HASH_TABLE_HEADERS(inv_map, , inv_map_t*, offset_t,
                        INV_MAP_HASH_TABLE_SIZE)

struct inode_page {

  unsigned int inode;
  offset_t offset;

};

typedef struct inv_page_t {

  /* This mutex does NOT protect src or next.  Those are
   * protected by the page-cache's subsystem mutex.
   */
  mutex_t mutex;
  unsigned int type;
  unsigned int num_mappers;
  unsigned int age;
  struct inode_page src;
  struct inv_page_t* next;
  struct inv_page_t* lnext;
  struct inv_page_t* llast;
  inv_map_hash_table mappers;

} inv_page_t;

inv_page_t* inv_page_lookup(phys_addr_t);
phys_addr_t inv_page_addr(inv_page_t*);
void inv_page_static_init(unsigned int pages);

#endif

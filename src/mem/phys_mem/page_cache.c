#include <definitions.h>
#include <data_structures.h>
#include <phys_mem.h>
#include <inv_page_table.h>
#include <sync.h>
#include "page_cache.h"

/* Copyright (c) 2004 Eric McCorkle.  All rights reserved. */
/* page_cache.c STATUS: complete */

#define un_eq(x, y) ((x.inode) == (y.inode) &&\
                     (x.offset) == (y.offset))
#define un_id(x) ((x)->src)
#define hash(x) ((x).inode * ((x).offset - 1))
#define next(x) ((x)->next)
#define PAGE_HASH_TABLE_SIZE 65535

STAT_HASH_TABLE_HEADERS(phys_page, static inline, inv_page_t*,
			struct inode_page, PAGE_HASH_TABLE_SIZE)
STAT_HASH_TABLE(phys_page, static inline, inv_page_t*, struct inode_page,
		next, NULL, un_eq, un_id, hash, PAGE_HASH_TABLE_SIZE)

static mutex_t mutex;
static phys_page_hash_table hash_table;

phys_addr_t page_cache_lookup(unsigned int inode, offset_t offset) {

  struct inode_page id;
  phys_addr_t out;
  inv_page_t* page;

  id.inode = inode;
  id.offset = offset;
  mutex_lock(&mutex);

  if(NULL != (page = phys_page_hash_table_lookup(hash_table, id))) {

    mutex_unlock(&mutex);
    out = inv_page_addr(page);

  }

  else {

    mutex_unlock(&mutex);
    out = NULL_PHYS_PAGE;

  }

  return out;

}


void page_cache_insert(phys_addr_t addr, unsigned int inode, offset_t offset) {

  inv_page_t* page;

  page = inv_page_lookup(addr);
  mutex_lock(&(page->mutex));
  page->type = PAGE_USED;
  page->src.inode = inode;
  page->src.offset = offset;
  mutex_unlock(&(page->mutex));
  mutex_lock(&mutex);
  phys_page_hash_table_insert(hash_table, page);
  mutex_unlock(&mutex);

}


void page_cache_delete(phys_addr_t addr) {

  inv_page_t* page;

  page = inv_page_lookup(addr);
  mutex_lock(&(page->mutex));
  page->type = PAGE_UNUSED;
  mutex_unlock(&(page->mutex));
  mutex_lock(&mutex);
  phys_page_hash_table_delete(hash_table, page->src);
  mutex_unlock(&mutex);

}

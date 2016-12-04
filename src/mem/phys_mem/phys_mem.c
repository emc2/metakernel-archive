#include <definitions.h>
#include <data_structures.h>
#include <malloc.h>
#include <phys_mem.h>
#include <inode.h>
#include <util.h>
#include <sync.h>
#include <inv_page_table.h>
#include "page_alloc.h"
#include "page_cache.h"
#include "swap.h"

/* Copyright (c) 2004 Eric McCorkle.  All rights reserved. */
/* STATUS phys_mem.c: complete */

#define ANON_SWAPPED 0
#define ANON_PRESENT 1

#define hash_next(x) ((x)->next)
#define un_eq(a, b) ((a) == (b))
#define un_id(x) ((x)->id)
#define null_hash(x) (x)
#define ANON_HASH_TABLE_SIZE ((4 * 1042 * 1042) - 1)

typedef struct anon_page_t {

  unsigned int id;
  struct anon_page_t* next;
  unsigned int status;

  union {

#ifndef NO_SWAP

    struct {

      inode_t* inode;
      offset_t page;

    } swap;

#endif

    phys_addr_t addr;

  } src;

} anon_page_t;

STAT_HASH_TABLE_HEADERS(anon_page, static inline,
			anon_page_t*, unsigned int,
			ANON_HASH_TABLE_SIZE)
STAT_HASH_TABLE(anon_page, static inline, anon_page_t*,
		unsigned int, hash_next, NULL, un_eq,
		un_id, null_hash, ANON_HASH_TABLE_SIZE)

/* Be aware: since page_alloc is only ever called
 * inside a lock of this mutex, it effectively
 * synchronizes eviction of pages.  If this is ever
 * changed, and new mutex MUST be created to synchronize
 * eviction!
 */
/* XXX it changed */
static anon_page_hash_table hash_table;
static mutex_t mutex;

static inline phys_addr_t get_anon_page(unsigned int* spc_ct,
					unsigned int* obj_ct,
					unsigned int shared) {

  phys_addr_t out;

  if(shared)
    out = phys_page_alloc(obj_ct);

  else
    out = phys_page_alloc(spc_ct);

  change_color_count(obj_ct, page_color(out), 1);

  return out;

}


static inline phys_addr_t get_page(unsigned int* spc_ct,
				   unsigned int* obj_ct,
				   unsigned int shared) {

  phys_addr_t out;

  if(shared)
    out = phys_page_alloc(obj_ct);

  else
    out = phys_page_alloc(spc_ct);

  change_color_count(spc_ct, page_color(out), 1);
  change_color_count(obj_ct, page_color(out), 1);

  return out;

}



/* Important note: anon_page_create is called when we are
 * INITIALIZING a new page, and are about to place into a
 * page table.  Uninitialized anonymous pages don't get an
 * entry in this table (to save memory).
 */

unsigned int anon_page_create(page_src_t src, unsigned int* spc_ct,
			      unsigned int* obj_ct, unsigned int shared) {

  unsigned int id;
  anon_page_t* new;
  anon_page_t* anon;
  phys_addr_t addr;

  new = malloc(sizeof(anon_page_t));
  new->status = ANON_PRESENT;
  id = rand();
  mutex_lock(&mutex);

  /* we want a unique identifier */
  while(NULL != anon_page_hash_table_lookup(hash_table, id)) {

    mutex_unlock(&mutex);
    id = rand();
    mutex_lock(&mutex);

  }

  new->id = id;

  switch(src.type) {

  case PSRC_NULL:

    new->src.addr = get_anon_page(spc_ct, obj_ct, shared);
    phys_memset(new->src.addr, 0, PAGE_SIZE);
    anon_page_hash_table_insert(hash_table, new);
    break;

  case PSRC_INODE:

    new->src.addr = get_anon_page(spc_ct, obj_ct, shared);

    if(NULL_PHYS_PAGE != 
       (addr = page_cache_lookup(src.inode.inode->stat.id,
				 src.inode.offset)))
      phys_memcpy(new->src.addr, addr, PAGE_SIZE);

    else
      inode_page_read(src.inode.inode,
		      src.inode.offset,
		      new->src.addr);

    anon_page_hash_table_insert(hash_table, new);
    break;

  case PSRC_ANON:

    new->src.addr = get_anon_page(spc_ct, obj_ct, shared);

    if(NULL != (anon = anon_page_hash_table_lookup(hash_table,
                                                   src.anon_page))) {

      switch(anon->status) {

      case ANON_PRESENT:

        phys_memcpy(new->src.addr, anon->src.addr, PAGE_SIZE);
        break;

#ifndef NO_SWAP

      case ANON_SWAPPED:

	/* do a page read on the swap device to prevent
	 * killing the swap entry
	 */
        inode_page_read(anon->src.swap.inode,
			anon->src.swap.page,
			new->src.addr);
        break;

#endif

      default:

        /* XXX kernel panic here */

        break;

      }

    }

    /* XXX kernel panic if no such anon page exists */

    break;

  case PSRC_KERN:

    new->src.addr = kern_page_alloc(obj_ct);
    anon_page_hash_table_insert(hash_table, new);
    break;

  default:

    /* XXX kernel panic here */

    break;

  }

  mutex_unlock(&mutex);

  return id;

}


void anon_page_free(unsigned int id) {

  anon_page_t* page;
  inv_page_t* inv_page;

  mutex_lock(&mutex);

  if(NULL != (page = anon_page_hash_table_delete(hash_table, id))) {

    mutex_unlock(&mutex);

    switch(page->status) {

#ifndef NO_SWAP

    case ANON_SWAPPED:

      /* delete the swap device entry */
      swap_free(page->src.swap.inode, page->src.swap.page);
      free(page);
      break;

#endif

    case ANON_PRESENT:

      inv_page = inv_page_lookup(page->src.addr);
      mutex_lock(&(inv_page->mutex));
      /* XXX kernel panic if inv_page->mappers != 0 */
      inv_page->type = PAGE_UNUSED;
      mutex_unlock(&(inv_page->mutex));
      phys_page_insert(1, &(page->src.addr));
      free(page);
      break;

    default:

      /* XXX kernel panic here */

      break;

    }

  }

  /* XXX kernel panic if page does not exist */

}


phys_addr_t phys_page_fetch(page_src_t src, unsigned int* spc_ct,
			    unsigned int* obj_ct, unsigned int shared) {

  anon_page_t* anon;
  phys_addr_t out;

  mutex_lock(&mutex);

  switch(src.type) {

  case PSRC_INODE:

    if(NULL_PHYS_PAGE != (out = page_cache_lookup(src.inode.inode->stat.id,
						  src.inode.offset)))
      change_color_count(spc_ct, page_color(out), 1);

    else {

      out = get_page(spc_ct, obj_ct, shared);
      inode_page_read(src.inode.inode, src.inode.offset, out);
      page_cache_insert(out, src.inode.inode->stat.id, src.inode.offset);

    }

    break;

  case PSRC_ANON:

    if(NULL != (anon = anon_page_hash_table_lookup(hash_table,
						   src.anon_page))) {

      switch(anon->status) {

      case ANON_PRESENT:

	out = anon->src.addr;
	change_color_count(spc_ct, page_color(out), 1);
	break;

#ifndef NO_SWAP

      case ANON_SWAPPED:

	out = get_page(spc_ct, obj_ct, shared);
	swap_in(out, anon->src.swap.inode, anon->src.swap.page);
	break;

#endif

      default:

	/* XXX kernel panic here */

	break;

      }

    }

    /* XXX kernel panic if no such anon page exists */

    break;

  case PSRC_NULL:
  case PSRC_KERN:
  default:

    /* XXX kernel panic here */

    break;

  }

  mutex_unlock(&mutex);

  return out;

}

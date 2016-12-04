#include <definitions.h>
#include <malloc.h>
#include <data_structures.h>
#include <sync.h>
#include <phys_mem.h>
#include "cow_table.h"

/* Copyright (c) 2004 Eric McCorkle.  All rights reserved. */
/* STATUS cow_table.c: compiles */

/* COW pages are reference counted and garbage collected.
 * They are kept in a hash table by their source.  When the
 * page is actually copied, an anonymous page is created
 * and the corresponding COW entry is decremented.  After
 * copying, the page becomes anonymous and thus dissociated
 * from its source.
 */

#define COW_TABLE_SIZE 32767

#define next(x) ((x)->next)
#define un_id(x) ((x)->page)

typedef struct cow_entry_t {

  page_src_t page;
  unsigned int refs;
  struct cow_entry_t* next;

} cow_entry_t;


static inline unsigned int hash(page_src_t s) {

  switch (s.type) {

  case PSRC_NULL:
  case PSRC_KERN:

    return 0;

  case PSRC_ANON:

    return (unsigned int)(s.anon_page);

  case PSRC_INODE:

    return (unsigned int)(s.inode.inode->stat.id) *
      (unsigned int)(s.inode.offset);

  default:

    /* XXX kernel panic */

    return 0;

  }

}

static inline int comp(page_src_t a, page_src_t b) {

  if(a.type == b.type) {

    switch(a.type) {

    case PSRC_NULL:

      return 0;

    case PSRC_INODE:

      if(PSRC_INODE == b.type)
	return a.inode.inode == b.inode.inode &&
	  a.inode.offset == a.inode.offset;

      else
	return 0;

    case PSRC_ANON:

      if(PSRC_ANON == b.type)
	return a.anon_page == b.anon_page;

      else
	return 0;

    case PSRC_KERN:

      return 0;

    default:

      /* XXX kernel panic */

      break;

    }

  }

  return 0;

}

STAT_HASH_TABLE_HEADERS(cow, static inline, cow_entry_t*,
			page_src_t, COW_TABLE_SIZE)
STAT_HASH_TABLE(cow, static inline, cow_entry_t*, page_src_t,
		next, NULL, comp, un_id, hash, COW_TABLE_SIZE)

static cow_hash_table hash_table = { NULL };
static mutex_t hash_table_mutex;

void cow_inc(page_src_t page_src) {

  cow_entry_t* entry;

  mutex_lock(&hash_table_mutex);
  entry = cow_hash_table_lookup(hash_table, page_src);

  if(NULL == entry) {

    entry = malloc(sizeof(cow_entry_t));
    entry->page = page_src;
    entry->refs = 2;
    cow_hash_table_insert(hash_table, entry);

  }

  else
    entry->refs++;

  mutex_unlock(&hash_table_mutex);

}


unsigned int cow_lookup(page_src_t page_src) {

  unsigned int out;
  cow_entry_t* entry;

  mutex_lock(&hash_table_mutex);
  entry = cow_hash_table_lookup(hash_table, page_src);

  if(NULL == entry)
    out = 0;

  else
    out = entry->refs;

  mutex_unlock(&hash_table_mutex);

  return out;

}


void cow_dec(page_src_t page_src) {

  cow_entry_t* entry;

  mutex_lock(&hash_table_mutex);
  entry = cow_hash_table_lookup(hash_table, page_src);

  if(NULL != entry) {

    if(1 != entry->refs)
      entry->refs--;

    else {

      cow_hash_table_delete(hash_table, page_src);
      free(entry);

    }

  }

  mutex_unlock(&hash_table_mutex);

}

#include <definitions.h>
#include <data_structures.h>
#include <sync.h>
#include <malloc.h>
#include <inode.h>
#include <filesys.h>
#include "inode_cache.h"

/* Copyright (c) 2004 Eric McCorkle.  All rights reserved. */
/* STATUS inode_cache.c: compiles */

#define INODE_CACHE_TABLE_SIZE 32767

struct fs_ref {

  filesys_t* filesys;
  u_64 filesys_id;

};

typedef struct cache_entry_t {

  struct cache_entry_t* hash_next;
  struct cache_entry_t* rev_next;
  struct cache_entry_t* list_next;
  struct cache_entry_t* list_last;
  struct fs_ref src;
  inode_t* inode;

} cache_entry_t;

#define hash_next(x) ((x)->hash_next)
#define rev_next(x) ((x)->rev_next)
#define list_next(x) ((x)->list_next)
#define list_last(x) ((x)->list_last)
#define hash(x) (((x).filesys->src->stat.id) * (unsigned int)((x).filesys_id))
#define rev_hash(x) (x->stat.id)
#define un_id(x) ((x)->src)
#define rev_un_id(x) ((x)->inode)
#define un_eq(x, y) ((x).filesys == (y).filesys &&\
                     (x).filesys_id == (y).filesys_id)
#define rev_un_eq(x, y) ((x) == (y))

CIRCULAR_LIST_HEADERS(cache, static inline, cache_entry_t*)
CIRCULAR_LIST(cache, static inline, cache_entry_t*,
	      list_next, list_last, NULL)
STAT_HASH_TABLE_HEADERS(cache, static inline, cache_entry_t*,
			struct fs_ref, INODE_CACHE_TABLE_SIZE)
STAT_HASH_TABLE(cache, static inline, cache_entry_t*,
		struct fs_ref, hash_next, NULL, un_eq,
		un_id, hash, INODE_CACHE_TABLE_SIZE)
STAT_HASH_TABLE_HEADERS(rev, static inline, cache_entry_t*,
			inode_t*, INODE_CACHE_TABLE_SIZE)
STAT_HASH_TABLE(rev, static inline, cache_entry_t*,
		inode_t*, rev_next, NULL, rev_un_eq,
		rev_un_id, rev_hash, INODE_CACHE_TABLE_SIZE)

static mutex_t mutex;
static cache_hash_table cache_table;
static rev_hash_table rev_table;
static cache_entry_t* queue;

inode_t* inode_cache_lookup(filesys_t* filesys, u_64 filesys_id) {

  inode_t* inode;
  struct fs_ref key;
  cache_entry_t* entry;

  key.filesys = filesys;
  key.filesys_id = filesys_id;
  mutex_lock(&mutex);

  if(NULL != (entry = cache_hash_table_lookup(cache_table, key))) {

    /* we already have it */
    if(NULL != entry->inode) {

      if(NULL != entry->list_next) {

	/* have to reset the front pointer
	 * if we delete the front
	 */
	if(queue != entry) {

	  entry->list_next->list_last = entry->list_last;
	  entry->list_last->list_next = entry->list_next;

	}

	else
	  queue = cache_clist_delete(queue);

	entry->list_next = NULL;
	entry->list_last = NULL;

      }

      mutex_lock(&(entry->inode->mutex));
      entry->inode->stat.ref_count++;
      mutex_unlock(&(entry->inode->mutex));
      mutex_unlock(&mutex);

      return entry->inode;

    }

    /* its there, but deleted */
    else
      return NULL;

  }

  /* it's not here, pull it into memory */
  else if(NULL != (inode = filesys->inode_read(filesys, filesys_id))) {

    entry = malloc(sizeof(entry));
    entry->list_next = NULL;
    entry->list_last = NULL;
    entry->src = key;
    mutex_init(&(inode->mutex));
    cond_init(&(inode->cond));
    entry->inode = inode;
    inode->stat.ref_count = 1;
    cache_hash_table_insert(cache_table, entry);
    rev_hash_table_insert(rev_table, entry);
    mutex_unlock(&mutex);

    return inode;

  }

  /* XXX should kernel panic here instead */
  else {

    mutex_unlock(&mutex);

    return NULL;

  }

}


void inode_cache_free(inode_t* inode) {

  cache_entry_t* entry;

  mutex_lock(&mutex);

  if(NULL != (entry = rev_hash_table_lookup(rev_table, inode)))
    queue = cache_clist_insert(queue, entry);

  /* XXX kernel panic if it doesn't exist */

  mutex_unlock(&mutex);

}


inode_t* inode_cache_create(filesys_t* filesys) {

  inode_t* inode;
  cache_entry_t* entry;
  u_64 filesys_id;

  mutex_lock(&mutex);

  if(NULL != (inode = filesys->inode_create(filesys, &filesys_id))) {

    entry = malloc(sizeof(entry));
    entry->list_next = NULL;
    entry->list_last = NULL;
    entry->src.filesys = filesys;
    entry->src.filesys_id = filesys_id;
    mutex_init(&(inode->mutex));
    cond_init(&(inode->cond));
    entry->inode = inode;
    inode->stat.ref_count = 1;
    cache_hash_table_insert(cache_table, entry);
    rev_hash_table_insert(rev_table, entry);
    mutex_unlock(&mutex);

    return inode;

  }

  else {

    mutex_unlock(&mutex);

    return NULL;

  }

}


void inode_cache_delete(inode_t* inode) {

  cache_entry_t* entry;

  mutex_lock(&mutex);

  if(NULL != (entry = rev_hash_table_lookup(cache_table, inode))) {

    entry->inode = NULL;
    queue = cache_clist_insert(queue, entry);

  }

  /* XXX kernel panic if its not here */
  mutex_unlock(&mutex);

}


static void inode_cache_sync_entry(cache_entry_t* entry) {

  filesys_t* fs = entry->src.filesys;

  if(NULL != entry->inode) {

    mutex_lock(&(entry->inode->mutex));
    fs->inode_write(fs, entry->inode, entry->src.filesys_id);
    mutex_unlock(&(entry->inode->mutex));

  }

  else
    fs->inode_delete(fs, entry->src.filesys_id);

}

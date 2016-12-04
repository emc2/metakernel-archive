#include <definitions.h>
#include <data_structures.h>
#include <thread.h>
#include "thread_table.h"

/* Copyright (c) 2004 Eric McCorkle.  All rights reserved. */
#define hash_next(x) ((x)->hash_next)
#define un_id(x) ((x)->id)
#define un_eq(x, y) ((x) == (y))
#define hash(x) (x)
#define THREAD_HASH_TABLE_SIZE 65535

STAT_HASH_TABLE_HEADERS(thread, static inline, thread_t*,
			unsigned int, THREAD_HASH_TABLE_SIZE)
STAT_HASH_TABLE(thread, static inline, thread_t*,
		unsigned int, hash_next, NULL, un_eq,
		un_id, hash, THREAD_HASH_TABLE_SIZE)

static mutex_t mutex;
static thread_hash_table hash_table;

thread_t* thread_lookup(unsigned int id) {

  thread_t* out;

  lock(mutex);
  out = thread_hash_table_lookup(hash_table, id);
  unlock(mutex);

  return out;

}


void thread_insert(thread_t* thread) {

  unsigned int id;

  id = rand();
  lock(mutex);

  while(NULL != thread_hash_table_lookup(hash_table, id)) {

    unlock(mutex);
    id = rand();
    lock(mutex);

  }

  thread->id = id;
  thread_hash_table_insert(hash_table, thread);
  unlock(mutex);

  return out;

}


void thread_delete(unsigned int id) {

  thread_t* thread;

  lock(mutex);
  thread = thread_hash_table_lookup(hash_table, id);
  /* XXX need to do more cleaning house here */
  free(thread);
  unlock(mutex);

}

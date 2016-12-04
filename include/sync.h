#ifndef SYNC_H
#define SYNC_H

/* Copyright (c) 2004 Eric McCorkle.  All rights reserved */

#include <definitions.h>
#include <thread.h>

#define WAIT_NONE 0
#define WAIT_MUTEX 1
#define WAIT_COND 2

typedef struct {

  cache_line mutex;
  cache_line num_mutex;
  cache_line data_mutex;
  unsigned int num_spin;
  unsigned int pri_count;
  thread_t* queue;
  thread_t* holder;

} mutex_t;

typedef struct {

  cache_line mutex;
  cache_line num_mutex;
  cache_line queue_mutex;
  cache_line type_mutex;
  unsigned int num_spin;
  unsigned int broadcast;
  thread_t* queue;

} cond_t;

/* syncronization system calls */

/* Fundamental sync primitives.  Good for pretty much anything.
 * Mutexes will avoid priority inversion.  Conds will NOT (cannot tell
 * who will signal...).  The kern_* variants are internal to the
 * kernel and bypass all permissions checking.
 */

#define mutex_init(x) (memset(x, 0, sizeof(mutex_t)))
#define cond_init(x) (memset(x, 0, sizeof(cond_t)))

void mutex_lock(mutex_t*);
void mutex_unlock(mutex_t*);
int mutex_trylock(mutex_t*);
void cond_wait(cond_t*, mutex_t*);
void cond_signal(cond_t*);
void cond_broadcast(cond_t*);

/* asynchronous versions */
void async_mutex_lock(mutex_t*);
void async_mutex_unlock(mutex_t*);
int async_mutex_trylock(mutex_t*);
void async_cond_wait(cond_t*, mutex_t*);
void async_cond_signal(cond_t*);
void async_cond_broadcast(cond_t*);

/* misc functions */
void sync_check(void);

#endif

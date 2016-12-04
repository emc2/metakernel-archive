#ifndef THREAD_H
#define THREAD_H

/* Copyright (c) 2004 Eric McCorkle.  All rights reserved. */

/* threads and address spaces are mutually recursive */
#include <definitions.h>
#include <types.h>

/* scheduling parameters */
#define SCHED_RUNNABLE 0

#ifdef SCHED_SPECIAL_MODES

#define SCHED_NO_RESCHED 1
#define SCHED_EXCLUSIVE 2
#define SCHED_ATOMIC 3

#endif

#define SCHED_STOPPED 8
#define SCHED_INT_SLEEP 9
/* can I implement the kernel without this? */
#define SCHED_UNINT_SLEEP 10
#define SCHED_DEAD 11

/* flags */
#define THREAD_USE_REENTRY 0x1
#define THREAD_FPU 0x2

struct sigaction;

typedef struct {

  unichar name[256];
  unsigned int id;
  unsigned int perm;
  unsigned int flags;
  int remaining;
  unsigned int status;
  unsigned int run_status;
  unsigned int hard_priority;
  unsigned int soft_priority;
  void* reentry;
  void* stk_ptr;
  unsigned int num_inodes;
  inode_t* inodes;
  addr_spc_t* addr_spc;

} thread_stat_t;

struct thread_t {

  thread_stat_t stat;
  cache_line sync_mutex;
  unsigned int epoch;
#ifdef MULTITHREAD
  unsigned int cpu_id;
  unsigned int affinity;
  unsigned int affinity_heap_pos;
#endif
  unsigned int priority_heap_pos;
  unsigned int flags;
  struct thread_t* queue_next;
  struct thread_t* queue_last;
  struct thread_t* hash_next;
  unsigned int wait_type;
  void* wait_obj;
  context_t context;

};

thread_t thread_create(const thread_stat_t* state);
void thread_sigaction(unsigned int sig, const struct sigaction* new,
		     struct sigaction* old);
unsigned int thread_wait(unsigned int* ids, unsigned int num);
void thread_signal(unsigned int sig, unsigned int which,
		  unsigned int id, unsigned int size,
		  const void* data);
int thread_get(unsigned int, int, thread_stat_t*);
int thread_set(unsigned int, int, const thread_stat_t*);
void thread_yield(void);
void thread_exit(unsigned int code);
void thread_destroy(thread_t* thread);

#endif

#ifndef SCHEDULER_H
#define SCHEDULER_H

/* Copyright (c) 2004 Eric McCorkle.  All rights reserved. */

#include "priority_heap.h"
#include "affinity_heap.h"
#include <thread.h>

#ifdef SCHED_SPECIAL_MODES

#define HOLDING_EXCLUSION 0x1
#define EXCLUDED 0x2

#endif

typedef struct {

  priority_heap* running_queue;
  priority_heap* finished_queue;
  int running_count;
  int total_count;
#ifdef MULTITHREAD
  affinity_heap* affinity_queue;
  unsigned int affinity_count;
  unsigned int cpu_id;
  cache_line mutex;
#endif
  int flags;
  int current_remaining;
  thread_t* current_thread;
  unsigned int epoch;

} scheduler_t;

void sched_init(scheduler_t*, unsigned int);
thread_t* sched_resched(scheduler_t*);
thread_t* sched_remove_resched(scheduler_t*);
void sched_insert_thread(scheduler_t*, thread_t*);
void sched_remove_thread(scheduler_t*, thread_t*);
void sched_restart_thread(scheduler_t*, thread_t*);
void sched_reassign_thread(scheduler_t*, thread_t*);
void sched_change_remaining(scheduler_t*, thread_t*, unsigned int);

#ifdef MULTITHREAD
thread_t* sched_eschew_thread(scheduler_t*);
thread_t* sched_exchange_thread(scheduler_t*, thread_t*);
#endif

#endif

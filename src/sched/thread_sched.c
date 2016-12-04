#include <thread.h>
#include <malloc.h>
#include <sync.h>
#include <thread_sched.h>
#include <spin_lock.h>
#include <timer.h>
#include <context.h>
#include <cpu.h>
#include <static_alloc.h>
#include "scheduler.h"
#include "exclusion.h"
#include "multithread_sched.h"

/* Copyright(c) 2004 Eric McCorkle.  All rights reserved */

#define LOAD_DIFF_TOLERANCE 0

/* STATUS thread_sched.c: compiles */

/* an architecture to run the scheduler on an
 * arbitrary number of CPUS
 */

static unsigned int cpu_num;
static unsigned int global_epoch = 0;
static cache_line global_epoch_mutex;
static scheduler_t* schedulers;

unsigned int update_epoch(unsigned int n) {

  spin_lock(global_epoch_mutex);

  if(n > global_epoch)
    global_epoch = n;

  spin_unlock(global_epoch_mutex);

  return global_epoch;

}


void thread_sched_static_init(unsigned int num) {

  int i;

  cpu_num = num;
  cpus = static_alloc(num * sizeof(cpu_t),
		      CACHE_LINE_SIZE);
  schedulers = static_alloc(num * sizeof(scheduler_t),
			    CACHE_LINE_SIZE);

  for(i = 0; i < num; i++)
    sched_static_init(schedulers + i, i);

}


void thread_sched_dynamic_init(void) {

  int i;

  for(i = 0; i < cpu_num; i++)
    sched_dynamic_init(schedulers + i);

}


static inline unsigned int lowest_load(void) {

  int i;
  unsigned int min = 0;
  int min_count;

  spin_lock(schedulers[0].mutex);
  min_count = schedulers[0].total_count;
  spin_unlock(schedulers[0].mutex);

  /* find the cpu with the lowest load */
  for(i = 1; i < cpu_num; i++) {

    spin_lock(schedulers[i].mutex);
    min = min_count > schedulers[i].total_count ? i : min;
    min_count = min_count > schedulers[i].total_count ?
      schedulers[i].total_count : min_count;
    spin_unlock(schedulers[i].mutex);

  }

  return min;

}


static inline unsigned int highest_load(void) {

  int i;
  unsigned int max = 0;
  int max_count;

  spin_lock(schedulers[0].mutex);
  max_count = schedulers[0].total_count;
  spin_unlock(schedulers[0].mutex);

  for(i = 1; i < cpu_num; i++) {

    spin_lock(schedulers[i].mutex);
    max = max_count < schedulers[i].total_count ? i : max;
    max_count = max_count < schedulers[i].total_count ?
      schedulers[i].total_count : max_count;
    spin_unlock(schedulers[i].mutex);

  }

  return max;

}

void thread_insert(thread_t* thread) {

  int min;

  min = lowest_load();  
  thread->stat.status = SCHED_RUNNABLE;
  thread->stat.run_status = SCHED_RUNNABLE;
  /* add to the lowest load cpu */
  spin_lock(schedulers[min].mutex);
  sched_insert_thread(schedulers + min, thread);
  spin_unlock(schedulers[min].mutex);

}


void thread_restart(thread_t* thread) {

  unsigned int min;
  thread_t* swap;

  min = lowest_load();
  thread->stat.status = thread->stat.run_status;

  if(min == thread->cpu_id) {

    spin_lock(schedulers[min].mutex);
    sched_restart_thread(schedulers + min, thread);
    spin_unlock(schedulers[min].mutex);

  }

  else {

    spin_lock(schedulers[thread->cpu_id].mutex);
    swap = sched_exchange_thread(schedulers + thread->cpu_id, thread);
    spin_unlock(schedulers[thread->cpu_id].mutex);
    spin_lock(schedulers[min].mutex);
    /* add to the lowest load cpu */
    sched_restart_thread(schedulers + min, swap);
    spin_unlock(schedulers[min].mutex);

  }

}


void thread_remove(thread_t* thread) {

  timer_disable();
  spin_lock(schedulers[thread->cpu_id].mutex);
  sched_remove_thread(schedulers + thread->cpu_id, thread);
  spin_unlock(schedulers[thread->cpu_id].mutex);
  timer_restore();

}


void thread_set_status(thread_t* thread, unsigned int status) {

  switch(status) {

  case SCHED_RUNNABLE:

#ifdef SCHED_SPECIAL_MODES

  case SCHED_NO_RESCHED:
  case SCHED_EXCLUSIVE:
  case SCHED_ATOMIC:

#endif

    thread->stat.run_status = status;

    /* fall-through intentional */

  case SCHED_INT_SLEEP:
  case SCHED_UNINT_SLEEP:
  case SCHED_STOPPED:

    thread->stat.status = status;

    break;

  case SCHED_DEAD:
  default:

    break;
    /* XXX KERNEL EXCEPTION HERE */

  }

}

/* Used to apply temporary boosts in priority.
 * Does not change true_priority field
 */

void thread_set_priority(thread_t* thread, unsigned int priority) {

  int diff;
  mutex_t* mutex;

  timer_disable();
  spin_lock(thread->sync_mutex);
  diff = thread->stat.soft_priority - priority;
  thread->stat.soft_priority = priority;

  /* increase priority on the holder of the
   * mutex if waiting
   */
  if(WAIT_MUTEX == thread->wait_type &&
     !RUNNABLE_STATE(thread->stat.status)) {

    mutex = thread->wait_obj;
    spin_lock(mutex->data_mutex);
    mutex->pri_count += diff;
    thread_adjust_priority(mutex->holder, diff);
    spin_unlock(mutex->data_mutex);

  }

  if(thread->stat.remaining < 0)
    thread->stat.remaining = 0;

  if(RUNNABLE_STATE(thread->stat.status)) {

    spin_lock(schedulers[thread->cpu_id].mutex);
    sched_change_remaining(schedulers + thread->cpu_id, thread,
			   thread->stat.remaining + diff);
    spin_unlock(schedulers[thread->cpu_id].mutex);

  }

  spin_unlock(thread->sync_mutex);
  timer_restore();

}


void thread_adjust_priority(thread_t* thread, int priority) {

  mutex_t* mutex;

  timer_disable();
  spin_lock(thread->sync_mutex);
  thread->stat.soft_priority += priority;

  /* increase priority on the holder of the
   * mutex if waiting
   */
  if(WAIT_MUTEX == thread->wait_type &&
     !RUNNABLE_STATE(thread->stat.status)) {

    mutex = thread->wait_obj;
    spin_lock(mutex->data_mutex);
    mutex->pri_count += priority;
    thread_adjust_priority(mutex->holder, priority);
    spin_unlock(mutex->data_mutex);

  }

  if(thread->stat.remaining < 0)
    thread->stat.remaining = 0;

  if(RUNNABLE_STATE(thread->stat.status)) {

    spin_lock(schedulers[thread->cpu_id].mutex);
    sched_change_remaining(schedulers + thread->cpu_id, thread,
                           thread->stat.remaining + priority);
    spin_unlock(schedulers[thread->cpu_id].mutex);

  }

  spin_unlock(thread->sync_mutex);
  timer_restore();

}


/* used for permanent changes to a thread's priority */

void thread_set_hard_priority(thread_t* thread, unsigned int priority) {

  mutex_t* mutex;
  int diff;

  timer_disable();
  spin_lock(thread->sync_mutex);
  diff = thread->stat.hard_priority - priority;
  thread->stat.hard_priority = priority;
  thread->stat.soft_priority += diff;

  /* increase priority on the holder of the
   * mutex if waiting
   */
  if(WAIT_MUTEX == thread->wait_type &&
     !RUNNABLE_STATE(thread->stat.status)) {

    mutex = thread->wait_obj;
    spin_lock(mutex->data_mutex);
    mutex->pri_count += diff;
    thread_adjust_priority(mutex->holder, diff);
    spin_unlock(mutex->data_mutex);

  }

  if(thread->stat.remaining < 0)
    thread->stat.remaining = 0;

  if(RUNNABLE_STATE(thread->stat.status)) {

    spin_lock(schedulers[thread->cpu_id].mutex);
    sched_change_remaining(schedulers + thread->cpu_id, thread,
			   thread->stat.remaining + diff);
    spin_unlock(schedulers[thread->cpu_id].mutex);

  }

  spin_unlock(thread->sync_mutex);
  timer_restore();

}


/* this attempts to steal the lowest affinity
 * thread from the highest load CPU
 */

void rebalance_load(unsigned int sched) {

  unsigned int max;
  int load_diff;
  thread_t* thread;

  /* we enter holding lock on sched */
  load_diff = schedulers[sched].total_count;
  spin_unlock(schedulers[sched].mutex);
  max = highest_load();

  if(max != sched) {

    load_diff = schedulers[max].total_count - load_diff;

    if(load_diff > LOAD_DIFF_TOLERANCE) {

      spin_lock(schedulers[max].mutex);
      thread = sched_eschew_thread(schedulers + max);
      spin_unlock(schedulers[max].mutex);

      if(NULL != thread) {

	spin_lock(schedulers[sched].mutex);
	sched_reassign_thread(schedulers + sched, thread);

	return;

      }

    }

  }

  spin_lock(schedulers[sched].mutex);

}


void timer_interrupt(unsigned int cpu_id) {

  thread_t* curr_thread;
  thread_t* new_thread;

  curr_thread = current_thread();
  /* put myself to sleep if I must */
  sync_check();
  new_thread = sched_resched(schedulers + cpu_id);

  /* Remember, the curr_thread pointer is always
   * valid, because even if sync_check "kills" a
   * it really only delivers a SIGDLOCK
   */
  if(curr_thread == new_thread) {
    /* XXX deliver signals */
    return;

  }

  else if(NULL != new_thread) {

    /* XXX deliver signals */

    /* feel that sting... */
    if(cpus[cpu_id].addr_spc != new_thread->stat.addr_spc &&
       0 != new_thread->stat.addr_spc->id) {

      cpus[cpu_id].addr_spc = new_thread->stat.addr_spc;
      addr_spc_load(new_thread->stat.addr_spc);

    }

    context_load(new_thread->context);

  }

  /* if timer_interrupt actually returns,
   * we're idle
   */

}

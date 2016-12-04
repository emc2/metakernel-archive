#include <thread.h>
#include <malloc.h>
#include <cpu.h>
#include "scheduler.h"
#include "priority_heap.h"
#include "affinity_heap.h"
#include "multithread_sched.h"
#include "rendezvous.h"
#include "exclusion.h"

/* Copyright (c) 2004 Eric McCorkle.  All rights reserved. */
/* STATUS scheduler.c: compiles */

#define PRIORITY_BOOST 5


void sched_static_init(scheduler_t* sched, unsigned int id) {

#ifdef MULTITHREAD
  sched->cpu_id = id;
  sched->affinity_count = 0;
#endif
  sched->epoch = 0;
  sched->current_thread = NULL;
  sched->running_count = 0;
  sched->total_count = 0;

}


void sched_dynamic_init(scheduler_t* sched) {

  sched->running_queue = priority_heap_init(20);
  sched->finished_queue = priority_heap_init(20);
#ifdef MULTITHREAD
  sched->affinity_queue = affinity_heap_init(20);
#endif

}


static inline unsigned int cpu_page_faults(void) {

  return cpus[current_cpu()].page_faults;

}


static inline void cpu_clear_page_faults(void) {

  cpus[current_cpu()].page_faults = 0;

}


static inline int check_for_epoch(scheduler_t* sched) {

  return 0 == sched->running_queue->size;

}


/* Checks to see if the current thread is preempted
 * and updates sched accordingly
 */

static inline void keep_current_thread(scheduler_t* sched) {

  /* replace the current process if it is overwhelmed by priority */
  if(sched->current_remaining + PRIORITY_BOOST <
     priority_heap_head(sched->running_queue)->stat.remaining &&
     priority_heap_head(sched->running_queue) !=
     sched->current_thread) {
#ifdef MULTITHREAD
    affinity_heap_set_key(sched->affinity_queue,
			  sched->current_thread->affinity_heap_pos,
			  cpu_page_faults());
#endif
    priority_heap_set_key(sched->running_queue,
			  sched->current_thread->priority_heap_pos,
			  sched->current_remaining);
    sched->current_thread = priority_heap_head(sched->running_queue);
    sched->current_remaining = sched->current_thread->stat.remaining;

  }

}


static inline void retire_current_thread(scheduler_t* sched) {

  /* retire the current thread */
  sched->current_thread->epoch = sched->epoch + 1;
  sched->current_thread->stat.remaining =
    (sched->current_thread->stat.remaining +
     sched->current_thread->stat.soft_priority) / 2;
  sched->running_count += sched->current_thread->stat.remaining;
#ifdef MULTITHREAD
  affinity_heap_set_key(sched->affinity_queue,
			sched->current_thread->affinity_heap_pos,
			sched->affinity_count);
#endif
  priority_heap_delete(sched->running_queue,
		       sched->current_thread->priority_heap_pos);
  priority_heap_insert(sched->finished_queue,
		       sched->current_thread);

}


static inline void do_epoch(scheduler_t* sched) {

  priority_heap* tmp;

  /* swap the queues and counts and increment the counter */
  tmp = sched->finished_queue;
  sched->finished_queue = sched->running_queue;
  sched->running_queue = tmp;
  sched->epoch = update_epoch(sched->epoch + 1);
  sched->total_count = sched->running_count;
  sched->running_count = 0;

}


static inline void remove_current(scheduler_t* sched) {

  sched->total_count -= sched->current_thread->stat.remaining;
  sched->current_thread->epoch = sched->epoch;
  sched->current_thread->stat.remaining = sched->current_remaining;
#ifdef MULTITHREAD
  sched->affinity_count += cpu_page_faults();
  cpu_clear_page_faults();
  affinity_heap_delete(sched->affinity_queue,
                       sched->current_thread->affinity_heap_pos);
#endif
  priority_heap_delete(sched->running_queue,
                       sched->current_thread->priority_heap_pos);
  rebalance_load(sched->cpu_id);
  sched->current_thread = priority_heap_head(sched->running_queue);

}



static inline void check_current_thread(scheduler_t* sched) {


  if(NULL != sched->current_thread)

#ifdef SCHED_SPECIAL_MODES
    /* check to see if we cannot reschedule */
    if(SCHED_ATOMIC == sched->current_thread->stat.status ||
       SCHED_NO_RESCHED == sched->current_thread->stat.status)
      return;
#endif

  /* if the current process needs to be retired, do the bookeeping */
  if(NULL == sched->current_thread || 0 >= sched->current_remaining) {

    /* sometimes we have no current process */
    if(NULL != sched->current_thread)
      retire_current_thread(sched);

    /* do the epoch here */
    if(check_for_epoch(sched))
      do_epoch(sched);

    /* get new thread */
    sched->current_thread = priority_heap_head(sched->running_queue);
    sched->current_remaining = sched->current_thread->stat.remaining;

  }

  /* otherwise we want to keep the current
   * thread, so don't do the epoch yet
   */
  else
    keep_current_thread(sched);

}


/* Removes current thread if it is marked
 * unrunnable.
 */

static inline void test_runnable(scheduler_t* sched) {

  if(sched->current_thread != NULL &&
     !RUNNABLE_STATE(sched->current_thread->stat.status)) {

    remove_current(sched);
    sched->current_thread = NULL;

  }

}


/* This function does anything and everything
 * concerning exclusion.
 */

#ifdef SCHED_SPECIAL_MODES

static inline int do_exclusion(scheduler_t* sched) {

  /* if we are atomic and holding exclusion, we get out early */
  if(SCHED_ATOMIC != sched->current_thread->stat.status ||
     (sched->flags & HOLDING_EXCLUSION)) {

    /* clear if we held the exclusion and no longer need it */
    if(SCHED_ATOMIC != sched->current_thread->stat.status &&
       sched->flags & HOLDING_EXCLUSION)
      clear_exclusion();

    /* try to set exclusions */
    if(!(sched->flags & HOLDING_EXCLUSION)) {

      if(SCHED_EXCLUSIVE != sched->current_thread->stat.status &&
	 SCHED_ATOMIC != sched->current_thread->stat.status)
	set_exclusion(-1);

      else
	set_exclusion(sched->cpu_id);

    }

    /* now check it */
    if(check_exclusion(sched->cpu_id)) {

      /* set the holding flags if needed */
      if(SCHED_EXCLUSIVE == sched->current_thread->stat.status ||
	 SCHED_ATOMIC == sched->current_thread->stat.status)
	sched->flags |= HOLDING_EXCLUSION;

      return 1;

    }

    else {

      sched->flags |= EXCLUDED;

      return 0;

    }

  }

  else
    return 1;

}

#endif


thread_t* sched_resched(scheduler_t* sched) {

#ifdef MULTITHREAD
  sched->affinity_count += cpu_page_faults();
  cpu_clear_page_faults();
#endif

  /* Get rid of the current if it is no longer runnable */
  test_runnable(sched);

  if(NULL == priority_heap_head(sched->running_queue) &&
     NULL == priority_heap_head(sched->finished_queue))
    return sched->current_thread;

  /* don't charge a thread if we were excluded from running */
  if(NULL != sched->current_thread)
    sched->current_remaining--;

  check_current_thread(sched);

  /* Reset counters if we run idle */
  if(NULL == sched->current_thread) {

    sched->total_count = 0;
    sched->running_count = 0;

  }

#ifdef SCHED_SPECIAL_MODES

  if(do_exclusion(sched))
    return sched->current_thread;

  /* go idle if we don't get the CPU */
  else {

    sched->current_thread->stat.remaining =
      sched->current_remaining;
    sched->current_thread = NULL;

    return NULL;

  }

#else

  return sched->current_thread;

#endif

}


/* this function is to be used to remove and reschedule
 * during operating time
 */

thread_t* sched_remove_resched(scheduler_t* sched) {

  /* XXX NEED TO STOP THE CPU IF IT IS NOT US */

  remove_current(sched);

  /* do the epoch here */
  if(NULL == sched->current_thread) {

    /* we're ripping out the last one here... */
    if(NULL == priority_heap_head(sched->finished_queue))
      sched->current_thread = NULL;

    else {

      do_epoch(sched);
      sched->current_thread = priority_heap_head(sched->running_queue);

    }

  }

  if(NULL != sched->current_thread)
    sched->current_remaining = sched->current_thread->stat.remaining;

  else {

    sched->total_count = 0;
    sched->running_count = 0;

  }

#ifdef SCHED_SPECIAL_MODES

  if(check_exclusion_specific(sched->cpu_id))
    return sched->current_thread;

  else {

    sched->current_thread->stat.remaining = sched->current_remaining;
    sched->current_thread = NULL;

    return NULL;

  }

#else

  return sched->current_thread;

#endif

}


void sched_restart_thread(scheduler_t* sched, thread_t* thread) {

  unsigned int queue_size;
  unsigned int increase;
  int diff;
  unsigned int pow;

  if(sched->epoch > thread->epoch) {

    pow = sched->epoch - thread->epoch;
    pow = pow > 30 ? 30 : pow;
    diff = thread->stat.soft_priority - thread->stat.remaining;
    diff = diff < 0 ? 0 : diff;
    increase = diff - (diff / (2 << pow));
    thread->stat.remaining += increase;

  }

  /* In a multithread kernel, the affinity queue
   * holds every thread in a scheduler.  Otherwise,
   * sum the sizes of the priority queues
   */
#ifdef MULTITHREAD
  queue_size = sched->affinity_queue->size + 1;
  thread->cpu_id = sched->cpu_id;
  thread->affinity = 0;
  affinity_heap_grow(sched->affinity_queue, queue_size);
  affinity_heap_insert(sched->affinity_queue, thread);
#else
  queue_size = sched->running_queue->size +
    sched->finished_queue->size + 1;
#endif

  sched->total_count += thread->stat.soft_priority;
  priority_heap_grow(sched->running_queue, queue_size);
  priority_heap_grow(sched->finished_queue, queue_size);
  priority_heap_insert(sched->running_queue, thread);
  thread->epoch = sched->epoch;

}


/* This function expects to only be called within 
 * rebalance_load, which is called after removing
 * a thread, so we know there's space available.
 * It does not attempt to grow the heaps.
 */

void sched_reassign_thread(scheduler_t* sched, thread_t* thread) {

#ifdef MULTITHREAD
  if(thread->cpu_id != sched->cpu_id) {

    thread->cpu_id = sched->cpu_id;
    thread->affinity = 0;

  }

  affinity_heap_insert(sched->affinity_queue, thread);
#endif

  sched->total_count += thread->stat.remaining;
  priority_heap_insert(sched->running_queue, thread);
  thread->epoch = sched->epoch;

}



void sched_insert_thread(scheduler_t* sched, thread_t* thread) {

  unsigned int queue_size;

  sched->total_count += thread->stat.soft_priority;
  thread->stat.remaining = thread->stat.soft_priority / 2;

  /* In a multithread kernel, the affinity queue
   * holds every thread in a scheduler.  Otherwise,
   * sum the sizes of the priority queues
   */
#ifdef MULTITHREAD
  queue_size = sched->affinity_queue->size + 1;
  thread->cpu_id = sched->cpu_id;
  thread->affinity = 0;
  thread->epoch = sched->epoch;
  affinity_heap_grow(sched->affinity_queue, queue_size);
  affinity_heap_insert(sched->affinity_queue, thread);
#else
  queue_size = sched->running_queue->size +
    sched->finished_queue->size + 1;
#endif

  thread->epoch = sched->epoch;
  priority_heap_grow(sched->running_queue, queue_size);
  priority_heap_grow(sched->finished_queue, queue_size);
  priority_heap_insert(sched->running_queue, thread);

}


static inline void delete_from_queue(scheduler_t* sched, thread_t* thread) {

  if(sched->running_queue->size >= thread->priority_heap_pos &&
     sched->running_queue->data_name[thread->priority_heap_pos - 1] == thread)
    priority_heap_delete(sched->running_queue,
			 thread->priority_heap_pos);

  else
    priority_heap_delete(sched->finished_queue,
			 thread->priority_heap_pos);

#ifdef MULTITHREAD
  affinity_heap_delete(sched->affinity_queue,
		       thread->affinity_heap_pos);
#endif

}



void sched_remove_thread(scheduler_t* sched, thread_t* thread) {

  sched->total_count -= thread->stat.remaining;
  thread->epoch = sched->epoch;

  if(sched->cpu_id != thread->cpu_id)
    return;

  else if(sched->current_thread == thread)
    sched_remove_resched(sched);

  else {

    delete_from_queue(sched, thread);
    rebalance_load(sched->cpu_id);

  }

}


void sched_change_remaining(scheduler_t* sched, thread_t* thread,
			    unsigned int remaining) {

  /* if it's the current thread, update current_remaining */
  if(sched->current_thread != thread) {

    sched->total_count += (remaining - thread->stat.remaining);

    if(sched->running_queue->size >= thread->priority_heap_pos &&
       sched->running_queue->data_name[thread->priority_heap_pos - 1] == thread)
      priority_heap_set_key(sched->running_queue,
			    thread->priority_heap_pos,
			    remaining);

    else
      priority_heap_set_key(sched->finished_queue,
			    thread->priority_heap_pos,
			    remaining);

  }

  else {

    sched->total_count += (sched->current_remaining -
			   thread->stat.remaining);
    sched->current_remaining = remaining;

  }

}

#ifdef MULTITHREAD
static inline thread_t* get_lowest_affinity(scheduler_t* sched) {

  thread_t* out;

  if(NULL != (out = affinity_heap_head(sched->affinity_queue))) {

    if(sched->current_thread == out) {

      if(1 == sched->affinity_queue->size)
	out = NULL;

      else if(2 == sched->affinity_queue->size) {

	out = sched->affinity_queue->data_name[1];
	delete_from_queue(sched, out);

      }

      else {

	out= sched->affinity_queue->data_name[1]->affinity < 
	  sched->affinity_queue->data_name[2]->affinity ?
	  sched->affinity_queue->data_name[1] :
	  sched->affinity_queue->data_name[2];
	delete_from_queue(sched, out);

      }

    }

    else
      delete_from_queue(sched, out);

  }

  return out;

}


/* both of these functions are guaranteed not 
 * to return the currently running thread
 */

thread_t* sched_eschew_thread(scheduler_t* sched) {

  thread_t* out;

  out = get_lowest_affinity(sched);

  if(NULL != out) {

    sched->total_count -= out->stat.remaining;
    out->epoch = sched->epoch;

  }

  return out;

}


thread_t* sched_exchange_thread(scheduler_t* sched, thread_t* thread) {

  thread_t* out;

  out = affinity_heap_head(sched->affinity_queue);

  if(thread->cpu_id == sched->cpu_id ||
     NULL == out)
    return thread;

  if(out != sched->current_thread) {

    if(out->affinity < thread->affinity) {

      delete_from_queue(sched, out);
      sched_restart_thread(sched, thread);

      return out;

    }

  }

  else {

    out = sched->affinity_queue->data_name[1]->affinity <
      sched->affinity_queue->data_name[2]->affinity ?
      sched->affinity_queue->data_name[1] :
      sched->affinity_queue->data_name[2];

    if(out->affinity < thread->affinity) {

      delete_from_queue(sched, out);
      sched_restart_thread(sched, thread);

      return out;

    }

  }

  return thread;

}
#endif

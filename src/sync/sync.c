#include <thread_sched.h>
#include <data_structures.h>
#include <spin_lock.h>
#include <timer.h>
#include <context.h>
#include <sync.h>
#include <int_thread.h>
#include "arch_sync.h"

/* Copyright (c) 2004 Eric McCorkle.  All rights reserved. */
/* STATUS sync.c: compiles */

#define thread_queue_insert(x, y) thread_clist_insert(x, y)
#define thread_queue_remove(x) thread_clist_delete(x->queue_last);
#define thread_next(x) ((x)->queue_next)
#define thread_last(x) ((x)->queue_last)

CIRCULAR_LIST_HEADERS(thread, static inline, thread_t*)
CIRCULAR_LIST(thread, static inline, thread_t*,
	      thread_next, thread_last, NULL)

/* we have to set some data fields before reentering */

static inline void awaken_thread(thread_t* thread) {

  thread->wait_type = WAIT_NONE;
  thread_restart(thread);

}


#ifdef MULTITHREAD

static inline void get_priority_boost(mutex_t* mutex, thread_t* current) {

  int priority;

  spin_lock(mutex->data_mutex);
  priority = mutex->pri_count;
  mutex->holder = current;
  spin_unlock(mutex->data_mutex);
  current->wait_obj = WAIT_NONE;
  thread_adjust_priority(current, priority);

}


void mutex_lock(mutex_t* mutex) {

  thread_t* current;

  if(!spin_trylock(mutex->mutex)) {

    current = current_thread();
    current->wait_obj = mutex;
    timer_disable();
    current->wait_type = WAIT_MUTEX;
    spin_lock(mutex->num_mutex);
    mutex->num_spin++;
    spin_unlock(mutex->num_mutex);

    /* try once without spinning */
    if(!spin_trylock(mutex->mutex)) {

      /* Ok to disable timer, because we've already
       * registered ourselves as spinning.
       */
      timer_restore();
      spin_lock(mutex->mutex);
      /* The timer is disabled for us by whomever
       * unlocks the mutex.
       */

    }

    /* grab all the priorities before they start posting
     * to me directly
     */
    get_priority_boost(mutex, current);

    /* Technically this is a race, also I now own the mutex
     * but the count does not yet show this.  But, the
     * following make these OK:
     *
     * No one trying to LOCK will make a decision based
     * on num_spin.
     *
     * Only someone trying to UNLOCK will, and since that
     * should only be me since I own the mutex, there is no
     * race.
     *
     * Timer is disabled, so lock_sleep() will not be called.
     */
    mutex->num_spin--;
    timer_restore();

  }

  else
    get_priority_boost(mutex, current);

}


/* Interrupt lock.  Creates an interrupt thread
 * if it would block.
 */

void async_mutex_lock(mutex_t* mutex) {

  thread_t* thread;

  /* No timer disabling, and no priority boosts.
   * Interrupt handlers will run atomic with regard
   * to the clock, so we don't have to worry.
   */
  if(!spin_trylock(mutex->mutex)) {

    /* create an interrupt thread and put it in
     * the queue and in the sleep state
     */
    /*    thread = int_thread_create(lock_reentry, SCHED_INT_SLEEP);*/
    spin_lock(mutex->data_mutex);
    mutex->queue = thread_queue_insert(mutex->queue, thread);
    mutex->pri_count += thread->stat.soft_priority;
    spin_unlock(mutex->data_mutex);
    thread->stat.reentry = lock_reentry;
    thread->stat.flags |= THREAD_USE_REENTRY;
    spin_lock(thread->sync_mutex);
    thread_adjust_priority(mutex->holder, thread->stat.soft_priority);
    spin_unlock(thread->sync_mutex);

  }

  else
    mutex->holder = NULL;

}


int mutex_trylock(mutex_t* mutex) {

  thread_t* current;
  unsigned int priority;
  int out;

  timer_disable();

  if(spin_trylock(mutex->mutex)) {

    current = current_thread();
    spin_lock(mutex->data_mutex);
    priority = mutex->pri_count;
    mutex->holder = current;
    spin_unlock(mutex->data_mutex);
    thread_adjust_priority(current, priority);
    out = 1;

  }

  else
    out = 0;

  timer_restore();

  return out;

}


int async_mutex_trylock(mutex_t* mutex) {

  if(spin_trylock(mutex->mutex)) {

    spin_lock(mutex->data_mutex);
    mutex->holder = NULL;
    spin_unlock(mutex->data_mutex);

    return 1;

  }

  else
    return 0;

}


void mutex_unlock(mutex_t* mutex) {

  thread_t* wakeup_thread;
  thread_t* current;

  current = current_thread();
  timer_disable();
  spin_lock(mutex->num_mutex);
  /* go back to normal priority */
  thread_adjust_priority(current, -(mutex->pri_count));

  /* Timer stays disabled in this case.  It is
   * reactivated by whomever acquires the mutex.
   */
  if(0 != mutex->num_spin) {

    spin_unlock(mutex->num_mutex);
    spin_unlock(mutex->mutex);

  }

  /* One of two cases is in effect:
   *
   * No one else wants our mutex.
   *
   * Other people want it, but they're all asleep.
   */
  else {

    spin_unlock(mutex->num_mutex);
    spin_lock(mutex->data_mutex);

    if(NULL != mutex->queue) {

      /* get the wakeup thread from the queue */
      wakeup_thread = mutex->queue;
      mutex->queue = thread_queue_remove(mutex->queue);
      spin_lock(wakeup_thread->sync_mutex);
      /* remove the priority of the thread being awakened
       * from the priority count of the mutex
       */
      mutex->pri_count -= wakeup_thread->stat.soft_priority;
      /* it is no longer waiting */
      wakeup_thread->wait_obj = WAIT_NONE;
      mutex->holder = wakeup_thread;
      spin_unlock(wakeup_thread->sync_mutex);
      /* boost its priority */
      thread_adjust_priority(wakeup_thread, mutex->pri_count);
      spin_unlock(mutex->data_mutex);
      /* cut it loose */
      thread_restart(wakeup_thread);

    }

    /* nobody gets the mutex */
    else {

      spin_unlock(mutex->data_mutex);
      mutex->holder = NULL;
      spin_unlock(mutex->mutex);

    }

    timer_restore();

  }

}


/* the only real difference betweet this and
 * iunlock is the lack of timer disabling and
 * priority adjustment of the unlocking thread
 */
void async_mutex_unlock(mutex_t* mutex) {

  thread_t* wakeup_thread;

  spin_lock(mutex->num_mutex);

  if(0 != mutex->num_spin) {

    /* I have to disable the global timer */
    timer_disable();
    spin_unlock(mutex->num_mutex);
    spin_unlock(mutex->mutex);

  }

  else {

    spin_unlock(mutex->num_mutex);
    spin_lock(mutex->data_mutex);

    if(NULL != mutex->queue) {

      wakeup_thread = mutex->queue;
      mutex->queue = thread_queue_remove(mutex->queue);
      spin_lock(wakeup_thread->sync_mutex);
      /* remove the priority of the thread being awakened
       * from the priority count of the mutex
       */
      mutex->pri_count -= wakeup_thread->stat.soft_priority;
      /* it is no longer waiting */
      wakeup_thread->wait_obj = WAIT_NONE;
      mutex->holder = wakeup_thread;
      spin_unlock(wakeup_thread->sync_mutex);
      /* boost its priority */
      thread_adjust_priority(wakeup_thread, mutex->pri_count);
      spin_unlock(mutex->data_mutex);
     /* cut it loose */
      thread_restart(wakeup_thread);

    }

    else {

      spin_unlock(mutex->data_mutex);
      spin_unlock(mutex->mutex);

    }

  }

}


/* These ALWAYS block, so they are bereft of some
 * of the dirty optimizations of mutexes.  Conditions
 * ultimately boil down to a spin lock that is continually
 * in a locked state.
 */

static inline void do_wait(cond_t* cond, mutex_t* mutex) {

  thread_t* current;

  current = current_thread();
  current->wait_obj = cond;
  current->wait_type = WAIT_COND;
  timer_disable();
  spin_lock(cond->num_mutex);
  cond->num_spin++;
  spin_unlock(cond->num_mutex);
  timer_restore();
  /* I'm already registered as waiting, so I can call this */
  mutex_unlock(mutex);
  spin_lock(cond->mutex);
  /* Again, the timer is disabled when I get here */
  current->wait_obj = WAIT_NONE;

  /* If I was awakened by a broadcast, I don't reset
   * the timer since other threads are also waking up.
   */
  if(!cond->broadcast)
    timer_restore();

  spin_unlock(cond->type_mutex);

}


void cond_wait(cond_t* cond, mutex_t* mutex) {

  do_wait(cond, mutex);
  mutex_lock(mutex);

}


/* This is guaranteed to block... */

static inline void do_async_wait(cond_t* cond, mutex_t* mutex) {

  thread_t* thread;

  /* create an interrupt thread and put it in the sleep
   * state
   */
  /*  thread = int_thread_create(lock_reentry, SCHED_INT_SLEEP);*/
  thread->stat.reentry = wait_reentry;
  thread->stat.flags |= THREAD_USE_REENTRY;
  thread->stat.status = SCHED_INT_SLEEP;
  spin_lock(cond->queue_mutex);
  cond->queue = thread_queue_insert(cond->queue, thread);
  spin_unlock(cond->queue_mutex);
  async_mutex_unlock(mutex);

}


void async_cond_wait(cond_t* cond, mutex_t* mutex) {

  do_async_wait(cond, mutex);
  /* I'm no longer in an asynchronous state here... */
  mutex_lock(mutex);

}


void cond_signal(cond_t* cond) {

  thread_t* wakeup_thread;

  timer_disable();
  spin_lock(cond->num_mutex);

  /* If someone is spinning, unlock the mutex
   * and let them go.
   */
  if(0 != cond->num_spin) {

    cond->num_spin--;
    spin_unlock(cond->num_mutex);
    spin_lock(cond->type_mutex);
    cond->broadcast = 0;
    spin_unlock(cond->mutex);
    /* leave the timer disabled */

  }

  else {

    spin_unlock(cond->num_mutex);
    spin_lock(cond->queue_mutex);

    /* Black arts.  See above in the do_unlock() function */
    if(NULL != cond->queue) {
    
      wakeup_thread = cond->queue;
      cond->queue = thread_queue_remove(cond->queue);

    }

    spin_unlock(cond->queue_mutex);
    awaken_thread(wakeup_thread);
    timer_restore();

  }

}


/* identical to regular, sans timer disables */

void async_cond_signal(cond_t* cond) {

  thread_t* wakeup_thread;

  spin_lock(cond->num_mutex);

  if(0 != cond->num_spin) {

    cond->num_spin--;
    spin_unlock(cond->num_mutex);
    spin_lock(cond->type_mutex);
    cond->broadcast = 0;
    spin_unlock(cond->mutex);

  }

  else {

    spin_unlock(cond->num_mutex);
    spin_lock(cond->queue_mutex);

    if(NULL != cond->queue) {

      wakeup_thread = cond->queue;
      cond->queue = thread_queue_remove(cond->queue);

    }

    spin_unlock(cond->queue_mutex);
    awaken_thread(wakeup_thread);

  }

}



void cond_broadcast(cond_t* cond) {

  int i;
  thread_t* wakeup_thread;

  timer_disable();
  spin_lock(cond->num_mutex);

  /* release all spinners */
  for(i = 0; i < cond->num_spin; i++) {

    /* The fact that the awakened thread unlocks this
     * mutex also causes us to wait until it is awake.
     * This prevents a potential race condition that
     * could arise from unlocking the core mutex twice
     * before any thread gets it.
     */
    spin_lock(cond->type_mutex);
    cond->broadcast = 1;
    spin_unlock(cond->mutex);

  }

  cond->num_spin = 0;
  spin_unlock(cond->num_mutex);
  spin_lock(cond->queue_mutex);

  if(NULL != cond->queue) {

    /* release all sleepers */
    while(NULL != cond->queue) {

      wakeup_thread = cond->queue;
      cond->queue = thread_queue_remove(cond->queue);
      awaken_thread(wakeup_thread);

    }

    spin_unlock(cond->queue_mutex);

  }

  timer_restore();

}


/* also identical, mit keine timer disables */

void async_cond_broadcast(cond_t* cond) {

  int i;
  thread_t* wakeup_thread;

  spin_lock(cond->num_mutex);

  for(i = 0; i < cond->num_spin; i++) {

    spin_lock(cond->type_mutex);
    cond->broadcast = 1;
    spin_unlock(cond->mutex);

  }

  cond->num_spin = 0;
  spin_unlock(cond->num_mutex);
  spin_lock(cond->queue_mutex);

  if(NULL != cond->queue) {

    while(NULL != cond->queue) {

      wakeup_thread = cond->queue;
      cond->queue = thread_queue_remove(cond->queue);
      awaken_thread(wakeup_thread);

    }

    spin_unlock(cond->queue_mutex);

  }

}


/* this function detects deadlocks */

static inline int detect_deadlock(thread_t* thread, mutex_t* mutex) {

  mutex_t* new_mutex;
  thread_t* curr;

  spin_lock(mutex->data_mutex);
  curr = mutex->queue;

  /* make sure originator is not in the wait queue */
  if(NULL != curr)
    do {

      if(curr == thread) {

	spin_unlock(mutex->data_mutex);

	return 1;

      }

      curr = curr->queue_next;

    } while(curr != mutex->queue);

  /* make sure we are not the holder */
  if(mutex->holder != thread) {

    /* assumption: holder is not NULL or
     * we wouldn't be waiting
     */
    if(WAIT_MUTEX != mutex->holder->wait_type) {

      spin_unlock(mutex->data_mutex);

      return 0;

    }

    else {

      new_mutex = mutex->holder->wait_obj;
      spin_unlock(mutex->data_mutex);

      /* assumption: the "tree" of waiting threads
       * has no cycles, as every addition to it has
       * passed this check
       */
      return detect_deadlock(thread, new_mutex);

    }

  }

  /* if we already hold the mutex, a deadlock occurs */
  else {

    spin_unlock(mutex->data_mutex);

    return 1;

  }

}


/* lock_sleep and wait_sleep put a thread to
 * sleep to wait on a lock.
 */

static inline void lock_sleep(thread_t* current) {

  mutex_t* mutex;

  mutex = current->wait_obj;

  /* Not a race condition.  The only one that makes a
   * decision on num_spin is unlock, which works atomically
   */
  mutex->num_spin--;
  spin_lock(mutex->data_mutex);
  /* XXX maybe I should insert to the back of the queue... */
  mutex->queue = thread_queue_insert(mutex->queue, current);
  mutex->pri_count += current->stat.soft_priority;
  spin_unlock(mutex->data_mutex);
  spin_lock(current->sync_mutex);
  current->stat.reentry = lock_reentry;
  current->stat.flags |= THREAD_USE_REENTRY;
  current->stat.status = SCHED_INT_SLEEP;
  spin_unlock(current->sync_mutex);

  /* interrupt locks dont do priority adjustment */
  if(NULL != mutex->holder)
    thread_adjust_priority(mutex->holder, current->stat.soft_priority);

}


static inline void wait_sleep(thread_t* current) {

  cond_t* cond;

  cond = current->wait_obj;
  spin_lock(current->sync_mutex);
  current->stat.reentry = wait_reentry;
  current->stat.flags |= THREAD_USE_REENTRY;
  current->stat.status = SCHED_INT_SLEEP;
  spin_unlock(current->sync_mutex);

  /* Not a race condition.  The only ones that makes a
   * decision on num_spin are signal and broadcast, which
   * work atomically with respect to the timer.
   */
  cond->num_spin--;
  spin_lock(cond->queue_mutex);
  cond->queue = thread_queue_insert(cond->queue, current);
  spin_unlock(cond->queue_mutex);

}


/* This must be called ONLY within the timer interrupt, or
 * when a context-switch is inevitable.
 */

void sync_check() {

  thread_t* current;

  current = current_thread();

  switch(current->wait_type) {

  case WAIT_NONE:

    break;

  case WAIT_MUTEX:

    if(detect_deadlock(current, current->wait_obj)) {

      /* XXX HANDLE DEADLOCK */

    }

    else
      lock_sleep(current);

    break;

  case WAIT_COND:

    wait_sleep(current);

    break;

  default:

    /* XXX KERNEL EXCEPTION HERE */

    break;

  }

}

#else
#error "Uniprocessor synchronization not implemented."
#endif

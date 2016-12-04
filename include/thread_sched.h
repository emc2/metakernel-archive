#ifndef THREAD_SCHED_H
#define THREAD_SCHED_H

/* Copyright (c) 2004 Eric McCorkle.  All rights reserved. */

#include <thread.h>

/* these must all be called atomically in operation time */

void thread_insert(thread_t*);
void thread_remove(thread_t*);
void thread_restart(thread_t*);
void thread_set_status(thread_t*, unsigned int);
void thread_set_priority(thread_t*, unsigned int);
void thread_adjust_priority(thread_t*, int);
void thread_set_hard_priority(thread_t*, unsigned int);
void thread_sched_static_init(unsigned int);
void thread_sched_dynamic_init(void);
void timer_interrupt(unsigned int);

#endif

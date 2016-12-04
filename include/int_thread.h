#ifndef INT_THREAD_H
#define INT_THREAD_H

/* Copyright (c) 2004 Eric McCorkle.  All rights reserved */

/* This is a simple wrapper for thread_create.  It creates a new,
 * high-priority, garbage-collected thread that runs in address space
 * 0.  It starts with a new, empty stack.
 */
thread_t* int_thread_create(void* func, unsigned int status);

#endif

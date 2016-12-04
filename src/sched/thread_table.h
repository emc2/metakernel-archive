#ifndef THREAD_TABLE_H
#define THREAD_TABLE_H

/* Copyright (c) 2004 Eric McCorkle.  All rights reserved. */

thread_t* thread_lookup(unsigned int id);
unsigned int thread_insert(thread_t* thread);
void thread_delete(unsigned int id);

#endif

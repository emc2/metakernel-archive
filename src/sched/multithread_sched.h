#ifndef MULTITHREAD_SCHED_H
#define MULTITHREAD_SCHED_H

/* Copyright (c) 2004 Eric McCorkle.  All rights reserved. */

#ifdef MULTITHREAD
void rebalance_load(unsigned int);
unsigned int update_epoch(unsigned int);
#else
#define rebalance_load(x)
#define update_epoch(x) ((x) + 1)
#endif

#endif

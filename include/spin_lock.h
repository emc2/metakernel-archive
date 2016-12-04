#ifndef SPIN_LOCK_H
#define SPIN_LOCK_H

/* Copyright (c) 2004 Eric McCorkle.  All rights reserved. */

#include <definitions.h>

/* IMPORTANT!!  If a spinlock is being used as a
 * sync primitive, it MUST be wrappered in
 * global_disable_timer() and global_restore_timer()
 * calls to avoid a thread holding a spinlock from
 * being rescheduled.
 *
 * Do not use these locks except as a tool for
 * implementing real mutexes, the scheduler, or
 * anything on which those two depend.  Use real
 * mutexes from sync.h
 */

void spin_lock(cache_line mutex);
void spin_unlock(cache_line mutex);
int spin_trylock(cache_line mutex);

#endif

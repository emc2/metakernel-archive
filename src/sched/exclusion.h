#ifndef EXCLUSION_H
#define EXCLUSION_H

/* Copyright (c) 2004 Eric McCorkle.  All rights reserved. */

#ifdef SCHED_SPECIAL_MODES

void clear_exclusion(void);
void set_exclusion(int);
int check_exclusion(int);
int check_exclusion_specific(int);

#define RUNNABLE_STATE(x) (0 == ((x) & (~0x7)))

#else

#define RUNNABLE_STATE(x) ((x) == SCHED_RUNNABLE)

#endif

#endif

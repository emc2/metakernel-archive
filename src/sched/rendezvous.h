#ifndef RENDEZVOUS_H
#define RENDEZVOUS_H

/* Copyright (c) 2004 Eric McCorkle.  All rights reserved. */

#ifdef SCHED_SPECIAL_MODES

#include <definitions.h>

typedef struct {

  cache_line cond;
  cache_line mutex;
  unsigned int count;
  unsigned int max;
  unsigned int reset;

} rendezvous_t;

/* Used to implement the two rendezvous points
 * needed by the scheduler to do exclusion.
 */

void rendezvous_init(rendezvous_t*, unsigned int);
void rendezvous_join(rendezvous_t*);

#endif

#endif

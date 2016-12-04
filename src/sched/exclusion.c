#include <spin_lock.h>
#include "exclusion.h"
#include "rendezvous.h"

/* Copyright (c) 2004 Eric McCorkle.  All rights reserved. */
/* STATUS exclusion.c: compiles */

/* Used for special modes only.  Be advised, this code
 * has serious adverse affects on the efficiency of the
 * scheduler.
 */

#ifdef SCHED_SPECIAL_MODES

static rendezvous_t first;
static rendezvous_t second;
static int exclusion;
static cache_line exclusion_mutex;

void clear_exclusion() {

  if(-1 != exclusion)
    exclusion = -1;

  rendezvous_join(&first);

}


void set_exclusion(int cpu) {

  if(-1 != cpu) {

    spin_lock(exclusion_mutex);

    if(-1 == exclusion)
      exclusion = cpu;

    spin_unlock(exclusion_mutex);

  }

  rendezvous_join(&second);

}


int check_exclusion(int cpu) {

  /* Another ugly hack:
   * Assume any writes to exclusion have already
   * taken place.  (Valid with the rendezvous point).
   */

  return (cpu == exclusion || -1 == exclusion);

}


/* Designed to check to see if someone can run
 * if they start in the middle of a time slice
 */

int check_exclusion_specific(int cpu) {

  return exclusion == cpu;

}

#endif

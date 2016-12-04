#include <util.h>
#include <spin_lock.h>
#include "rendezvous.h"

/* Copyright (c) 2004 Eric McCorkle.  All rights reserved. */
/* STATUS rendezvous.c: compiles */

#ifdef SCHED_SPECIAL_MODES

#define CHECK_COND(x) ((x)[0])
#define SET_COND(x) ((x)[0] = 1)
#define CLEAR_COND(x) ((x)[0] = 0)

/* THIS CODE IS NOT SAFE FOR GENERAL CASES! IT ONLY
 * WORKS BECAUSE TWO RENDEZVOUS POINTS ARE USED IN
 * SUCCESSION
 */

void rendezvous_init(rendezvous_t* rend, unsigned int max) {

  memset(rend, 0, sizeof(rendezvous_t));
  rend->max = max;

}


void rendezvous_join(rendezvous_t* rend) {

  spin_lock(rend->mutex);

  if(!rend->reset) {

    rend->count++;

    if(rend->max == rend->count) {

      rend->reset = 1;
      spin_unlock(rend->mutex);
      SET_COND(rend->cond);

    }

    else {

      spin_unlock(rend->mutex);
      while(!CHECK_COND(rend->cond));

    }

  }

  else {

    rend->reset = 0;
    CLEAR_COND(rend->cond);
    rend->count = 1;
    spin_unlock(rend->mutex);
    while(!CHECK_COND(rend->cond));

  }

}

#endif

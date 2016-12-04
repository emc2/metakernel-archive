#include <rw_lock.h>
#include <sync.h>

/* Copyright (c) 2004 Eric McCorkle.  All rights reserved. */
/* STATUS rw_lock.c: compiles */

void rw_read_lock(rw_lock* rw) {

  mutex_lock(&(rw->mutex));

  while(-1 == rw->count)
    cond_wait(&(rw->cond), &(rw->mutex));

  rw->count++;
  mutex_unlock(&(rw->mutex));

}


void rw_read_unlock(rw_lock* rw) {

  mutex_lock(&(rw->mutex));
  rw->count--;

  if(0 == rw->count)
    cond_broadcast(&(rw->cond));

  mutex_unlock(&(rw->mutex));

}


void rw_write_lock(rw_lock* rw) {

  mutex_lock(&(rw->mutex));

  while(0 != rw->count)
    cond_wait(&(rw->cond), &(rw->mutex));

  rw->count = -1;
  mutex_unlock(&(rw->mutex));

}


void rw_write_unlock(rw_lock* rw) {

  mutex_lock(&(rw->mutex));
  rw->count = 0;
  cond_broadcast(&(rw->cond));
  mutex_unlock(&(rw->mutex));

}


void rw_upgrade_lock(rw_lock* rw) {

  mutex_lock(&(rw->mutex));
  rw->count--;

  while(0 != rw->count)
    cond_wait(&(rw->cond), &(rw->mutex));

  rw->count = -1;
  mutex_unlock(&(rw->mutex));

}


void rw_downgrade_lock(rw_lock* rw) {

  mutex_lock(&(rw->mutex));
  rw->count = 1;
  cond_broadcast(&(rw->cond));
  mutex_unlock(&(rw->mutex));

}

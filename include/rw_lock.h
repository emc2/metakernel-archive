#ifndef RW_LOCK_H
#define RW_LOCK_H

/* Copyright (c) 2004 Eric McCorkle.  All rights reserved. */
/* basic read/write lock for kernel applications
 * will not be exposed to outside because of simplicity
 */

#include <sync.h>

typedef struct {

  unsigned int count;
  cond_t cond;
  mutex_t mutex;

} rw_lock;

void rw_read_lock(rw_lock*);
void rw_read_unlock(rw_lock*);
void rw_write_lock(rw_lock*);
void rw_write_unlock(rw_lock*);
void rw_upgrade_lock(rw_lock*);
void rw_downgrade_lock(rw_lock*);

#endif

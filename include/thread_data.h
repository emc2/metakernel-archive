#ifndef THREAD_DATA_H
#define THREAD_DATA_H

/* Copyright (c) 2004 Eric McCorkle.  All rights reserved. */

#include <definitions.h>

/* only used to allocate threads */
typedef struct {

  page thread_data;
  page low_unmapped;
  page stack[5];
  page high_unmapped;

} thread_data;

#endif

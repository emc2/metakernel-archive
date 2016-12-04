#ifndef STATIC_ALLOC_H
#define STATIC_ALLOC_H

/* Copyright (c) 2004 Eric McCorkle.  All rights reserved. */

void* static_alloc(unsigned int size, unsigned int align);
void static_alloc_init(void* start);

#endif

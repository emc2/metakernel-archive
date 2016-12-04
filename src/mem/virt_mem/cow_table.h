#ifndef COW_TABLE_H
#define COW_TABLE_H

#include <definitions.h>
#include <phys_mem.h>

/* Copyright (c) 2004 Eric McCorkle.  All rights reserved. */
/* Increment inserts if the entry doesn't exist.
 * Decrement deletes if the count reaches 1.
 * Lookup returns 0 if entry is not found.
 */

void cow_inc(page_src_t src);
unsigned int cow_lookup(page_src_t src);
void cow_dec(page_src_t src);

#endif

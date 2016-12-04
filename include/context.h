#ifndef CONTEXT_H
#define CONTEXT_H

/* Copyright (c) 2004 Eric McCorkle.  All rights reserved */

#include <definitions.h>
#include <thread.h>
#include <virt_mem.h>

/* We can execute addr_spc_load in kernel mode, because
 * the kernel is in all address spaces.
 */
void addr_spc_load(addr_spc_t*);

/* Obviously, this never returns */
void context_load(context_t);

thread_t* current_thread(void);
addr_spc_t* current_addr_spc(void);

#endif

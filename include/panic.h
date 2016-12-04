#ifndef PANIC_H
#define PANIC_H

/* Copyright (c) 2004 Eric McCorkle.  All rights reserved. */

#include <types.h>

/* The almighty kernel panic.  It should print out
 * whatever message it is given and attempt
 * varying levels of graceful shutdown as follows:
 * unmount filesystems, core dump, vomit its
 * intestines all over everywhere...  Depending on
 * how bad the kernel choked, some of the first actions
 * might not make it...
 */

void kernel_panic(unichar* s);
void kernel_setlog(inode_t* inode);
void kernel_setdump(inode_t* inode);

#endif

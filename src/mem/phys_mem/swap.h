#ifndef SWAP_H
#define SWAP_H

#ifndef NO_SWAP

#include <inode.h>

/* Copyright (c) 2004 Eric McCorkle.  All rights reserved. */
void swap_in(phys_addr_t page, inode_t* id,
	     offset_t offset);
int swap_out(phys_addr_t page, inode_t** id,
	     offset_t* offset);
void swap_free(inode_t* inode, offset_t offset);
void swap_dev_insert(inode_t* inode, unsigned int pri);
void swap_dev_delete(inode_t* inode);

#endif

#endif

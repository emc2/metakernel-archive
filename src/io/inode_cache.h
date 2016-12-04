#ifndef INODE_CACHE_H
#define INODE_CACHE_H

/* Copyright (c) 2004 Eric McCorkle.  All rights reserved. */

#include <definitions.h>
#include <filesys.h>
#include <inode.h>

inode_t* inode_cache_lookup(filesys_t* filesys, u_64 filesys_id);
void inode_cache_free(inode_t* inode);
inode_t* inode_cache_create(filesys_t* filesys);
void inode_cache_delete(inode_t* inode);

#endif

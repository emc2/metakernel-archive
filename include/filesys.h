#ifndef FILESYS_H
#define FILESYS_H

/* Copyright (c) 2004 Eric McCorkle.  All rights reserved. */

#include <definitions.h>
#include <types.h>

#define FILESYS_READ_ONLY 0x1

/* represents a single mounted filesystem */

struct filesys_t {

  unsigned int flags;
  struct filesys_t* hash_next;
  inode_t* src;
  inode_t* (*inode_read)(struct filesys_t*, u_64);
  void (*inode_write)(struct filesys_t*, inode_t*, u_64);
  inode_t* (*inode_create)(struct filesys_t*, u_64*);
  void (*inode_delete)(struct filesys_t*, u_64);
  void (*stat)(const struct filesys_t*, void*);
  int (*unmount)(struct filesys_t*);
  int (*spec_op)(struct filesys_t*, unsigned int, ...);
  char metadata[];

};

typedef struct {

  unichar* name;
  filesys_t* (*mount)(inode_t*);
  inode_t* (*inode_read)(struct filesys_t*, u_64);
  void (*inode_write)(struct filesys_t*, inode_t*, u_64);
  inode_t* (*inode_create)(struct filesys_t*, u_64*);
  void (*inode_delete)(struct filesys_t*, u_64);
  void (*stat)(const struct filesys_t*, void*);
  void (*unmount)(struct filesys_t*);
  int (*spec_op)(struct filesys_t*, unsigned int, ...);

} fs_def_t;

filesys_t* filesys_mount(unichar* fsname, inode_t* inode);
int filesys_unmount(filesys_t* filesys);
filesys_t* filesys_lookup(inode_t* inode);
void filesys_register(fs_def_t* def);

#endif

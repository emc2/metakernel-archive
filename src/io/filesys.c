#include <definitions.h>
#include <util.h>
#include <sync.h>
#include <data_structures.h>
#include <inode.h>
#include <filesys.h>

/* Copyright (c) 2004 Eric McCorkle.  All rights reserved. */
/* STATUS filesys.c: compiles */

#define FS_HASH_TABLE_SIZE 127
#define MOUNT_HASH_TABLE_SIZE 511

#define next(x) ((x)->hash_next)
#define un_id(x) ((x)->fs->name)
#define mnt_id(x) ((x)->src)
#define hash(x) (x)
#define un_eq(x, y) ((x) == (y))
#define mnt_hash(x) ((x)->stat.id)

typedef struct fs_node {

  struct fs_node* hash_next;
  fs_def_t* fs;

} fs_node;

STAT_HASH_TABLE_HEADERS(fs, static inline, fs_node*,
			unichar*, FS_HASH_TABLE_SIZE)
STAT_HASH_TABLE(fs, static inline, fs_node*, unichar*,
		next, NULL, !strcmp, un_id, strhash,
		FS_HASH_TABLE_SIZE)
STAT_HASH_TABLE_HEADERS(mount, static inline, filesys_t*,
			inode_t*, MOUNT_HASH_TABLE_SIZE)
STAT_HASH_TABLE(mount, static inline, filesys_t*,
		inode_t*, next, NULL, un_eq, mnt_id,
		mnt_hash, MOUNT_HASH_TABLE_SIZE)

static mutex_t fstab_mutex;
static mutex_t mount_mutex;
static fs_hash_table fs_table;
static mount_hash_table mount_table;

filesys_t* filesys_mount(unichar* restrict fsname,
			 inode_t* restrict inode) {

  filesys_t* restrict out;
  fs_def_t* restrict fs_def;

  mutex_lock(&fstab_mutex);

  if(NULL != (fs_def = fs_hash_table_lookup(fs_table, fsname)->fs)) {

    mutex_unlock(&fstab_mutex);
    out = fs_def->mount(inode);

    if(NULL != out) {

      out->src = inode;
      mutex_lock(&mount_mutex);
      mount_hash_table_insert(mount_table, out);
      mutex_unlock(&mount_mutex);

    }

    return out;

  }

  else
    return NULL;

}


int filesys_unmount(filesys_t* restrict filesys) {

  int out;

  /* XXX there's a whole lot of shit
   * that needs to happen here...
   */

  mutex_lock(&mount_mutex);
  out = filesys->unmount(filesys);

  if(!out)
    mount_hash_table_delete(mount_table, filesys->src);

  mutex_unlock(&mount_mutex);

  return out;

}


filesys_t* filesys_lookup(inode_t* restrict inode) {

  filesys_t* restrict filesys;

  mutex_lock(&mount_mutex);
  filesys = mount_hash_table_lookup(mount_table, inode);
  mutex_unlock(&mount_mutex);

  return filesys;

}


void filesys_register(fs_def_t* restrict def) {

  fs_node* restrict node;

  node = malloc(sizeof(fs_node));
  node->fs = def;
  mutex_lock(&fstab_mutex);
  fs_hash_table_insert(fs_table, node);
  mutex_unlock(&fstab_mutex);

}

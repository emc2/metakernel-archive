#include <definitions.h>
#include <data_structures.h>
#include <filesys.h>
#include <inode.h>
#include <sync.h>
#include "inode_cache.h"

/* Copyright (c) 2004 Eric McCorkle.  All rights reserved. */
/* STATUS inode.c: compiles */
/* the last one... */

#define INODE_HASH_TABLE_SIZE 131071

#define un_id(x) ((x)->stat.id)
#define un_eq(x, y) ((x) == (y))
#define hash(x) (x)
#define next(x) ((x)->hash_next)

STAT_HASH_TABLE_HEADERS(inode, static inline, inode_t*,
                        unsigned int, INODE_HASH_TABLE_SIZE)
STAT_HASH_TABLE(inode, static inline, inode_t*,
		unsigned int, next, NULL, un_eq,
		un_id, hash, INODE_HASH_TABLE_SIZE)

static mutex_t mutex;
static inode_hash_table hash_table;

static inline unsigned int get_unique_id(void) {

  unsigned int id;

  id = rand();

  while(NULL != inode_hash_table_lookup(hash_table, id) &&
        0 != id)
    id = rand();

  return id;

}


inode_t* inode_create(const inode_stat_t* stat) {

  filesys_t* filesys;
  inode_t* fs_inode;
  inode_t* out;
  unsigned int id;

  mutex_lock(&mutex);
  id = get_unique_id();

  if(NULL != (fs_inode = inode_hash_table_lookup(hash_table, stat->filesys))) {

    if(NULL != (filesys = filesys_lookup(fs_inode))) {

      out = inode_cache_create(filesys);
      out->stat.id = id;
      /* XXX initialize other fields */
      inode_hash_table_insert(hash_table, out);      

    }

    else
      out = NULL;

  }

  else
    out = NULL;

  mutex_unlock(&mutex);

  return out;

}


int inode_close(inode_t* inode) {

  mutex_lock(&(inode->mutex));
  inode->stat.ref_count--;

  if(0 == inode->stat.ref_count) {

    mutex_lock(&mutex);
    inode_hash_table_delete(hash_table, inode->stat.id);
    mutex_unlock(&mutex);

    if(0 == inode->stat.hard_ref_count)
      inode_cache_delete(inode);

    else {

      inode_cache_free(inode);
      mutex_unlock(&(inode->mutex));

    }

  }

  return 0;

}


/* XXX implement get and set when semantics are clear */

int inode_page_read(inode_t* inode, offset_t offset,
		    phys_addr_t addr) {

  if(NULL != inode->funcs->page_read)
    return inode->funcs->page_read(inode, offset, addr);

  else
    return -1;

}

 
int inode_page_write(inode_t* inode, offset_t offset,
		     phys_addr_t addr) {

  if(NULL != inode->funcs->page_write)
    return inode->funcs->page_write(inode, offset, addr);

  else
    return -1;

}


int inode_index_read(inode_t* inode, offset_t offset,
		     offset_t size, void* ptr,
		     unsigned int mode) {

  if(NULL != inode->funcs->index_read)
    return inode->funcs->index_read(inode, offset, size, ptr, mode);

  else
    return -1;

}


int inode_index_write(inode_t* inode, offset_t offset,
		      offset_t size, const void* ptr,
		      unsigned int mode) {

  if(NULL != inode->funcs->index_write)
    return inode->funcs->index_write(inode, offset, size, ptr, mode);

  else
    return -1;

}


int inode_seq_read(inode_t* inode, offset_t size,
		   void* ptr, unsigned int mode) {

  if(NULL != inode->funcs->seq_read)
    return inode->funcs->seq_read(inode, size, ptr, mode);

  else
    return -1;

}


int inode_seq_write(inode_t* inode, offset_t size,
		    const void* ptr, unsigned int mode) {

  if(NULL != inode->funcs->seq_write)
    return inode->funcs->seq_write(inode, size, ptr, mode);

  else
    return -1;

}


int inode_pack_read(inode_t* inode, offset_t size,
		    void* ptr, unsigned int mode) {

  if(NULL != inode->funcs->pack_read)
    return inode->funcs->pack_read(inode, size, ptr, mode);

  else
    return -1;

}


int inode_pack_write(inode_t* inode, offset_t size,
		     const void* ptr, unsigned int mode) {

  if(NULL != inode->funcs->pack_write)
    return inode->funcs->pack_write(inode, size, ptr, mode);

  else
    return -1;

}


int inode_stat(inode_t* inode, inode_stat_t* stat) {

  if(NULL != inode->funcs->stat)
    return inode->funcs->stat(inode, stat);

  else
    return -1;

}


inode_t* inode_dir_lookup(inode_t* dir, const unichar* name) {

  if(NULL != dir->funcs->dir_lookup)
    return dir->funcs->dir_lookup(dir, name);

  else
    return NULL;

}


int inode_dir_link(inode_t* dir, const unichar* name, inode_t* inode) {

  int out;

  if(NULL != dir->funcs->dir_link &&
     inode->stat.filesys == dir->stat.filesys) {

    out = dir->funcs->dir_link(dir, name, inode);

    if(!out)
      inode->stat.hard_ref_count++;

    return out;

  }

  else
    return -1;

}


int inode_dir_vlink(inode_t* dir, const unichar* name, inode_t* inode) {

  int out;

  if(NULL != dir->funcs->dir_link) {

    /* XXX create vlink */

    inode->stat.ref_count++;

    return out;

  }

  else
    return -1;

}


int inode_dir_unlink(inode_t* dir, const unichar* name) {

  inode_t* file;
  int out;

  if(NULL != dir->funcs->dir_unlink) {

    file = dir->funcs->dir_lookup(dir, name);
    out = dir->funcs->dir_unlink(dir, name);

    if(!out)
      file->stat.hard_ref_count--;

    return out;

  }

  else
    return -1;

}


int inode_dir_list(inode_t* inode, unichar* buf) {

  if(NULL != inode->funcs->dir_list)
    return inode->funcs->dir_list(inode, buf);

  else
    return -1;

}


int inode_dir_count(inode_t* inode) {

  if(NULL != inode->funcs->dir_count)
    return inode->funcs->dir_count(inode);

  else
    return -1;

}


int inode_slink_read(inode_t* inode, unichar* buf) {

  if(NULL != inode->funcs->slink_read)
    return inode->funcs->slink_read(inode, buf);

  else
    return -1;

}


int inode_slink_write(inode_t* inode, const unichar* buf) {

  if(NULL != inode->funcs->slink_write)
    return inode->funcs->slink_write(inode, buf);

  else
    return -1;

}


int inode_spec_op(inode_t* inode, unsigned int op,
                  unsigned int mode, ...) {

  /* XXX implement when we figure out varargs */

}

/* demv("It is done."); // Deus Ex Machina voice
 */

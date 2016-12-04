#include <definitions.h>
#include <malloc.h>
/*
#include <fund_inode.h>
*/
#include <sync.h>

/* Copyright (c) 2004 Eric McCorkle.  All rights reserved. */
/* STATUS fund_inode.c: complete */

/* The almighty fundamental inode table...
 * The Great Mother of all data in the system.
 *
 * This thing shouldn't get resized or changed
 * that much...  Damn, how am I going to do NFS??
 * Not with these, that's the damn truth.
 *
 * first free is a pointer to the first free space in
 * the array.
 */

static fund_inode_t** fund_inode_tab;
static u_64 mutex;
static void* first_free;
static unsigned int size;

static inline void resize_table () {

  unsigned int i;
  unsigned int new_size;

  new_size = size * 2;
  fund_inode_tab = kern_realloc(fund_inode_tab, &new_size,
				size, 0);
  for(i = size; i < new_size; i++) {

    (void*)(fund_inode_tab[i]) = first_free;
    first_free = (void*)(fund_inode_tab[i]);

  }

  size = new_size;

}


unsigned int fund_inode_insert(fund_inode_t* inode) {

  u_64 out;
  fund_inode_t* addr;

  lock(mutex);
  addr = first_free;
  first_free = (void*)(*first_free);
  addr = inode;
  inode->flags &= INODE_FUNDAMENTAL;
  out = (addr - fund_inode_tab) / sizeof(fund_inode_t*);
  unlock(mutex);

  return out;

}


fund_inode_t* fund_inode_lookup(unsigned int index) {

  fund_inode_t* out;

  /* Eeeeeeww!  Hack alert!  If the address
   * found at the index is within the array, then
   * the spot is empty.
   */
  lock(mutex);
  out = fund_inode_tab[index];

  if(out > fund_inode_tab &&
     out < fund_inode_tab + size)
    out = NULL;

  unlock(mutex);

  return out;

}

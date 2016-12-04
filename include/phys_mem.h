#ifndef PHYS_MEM_H
#define PHYS_MEM_H

/* Copyright (c) 2004 Eric McCorkle.  All rights reserved */

#include <definitions.h>
#include <types.h>
#include <inode.h>

/* PSRC_KERN is used only to allocate kernel pages */
#define PSRC_NULL 0
#define PSRC_INODE 1
#define PSRC_ANON 2
#define PSRC_KERN 3

struct page_src_t {

  unsigned int type;

  union {

    /* Something must be specified here.  This system
     * assumes no inode aliasing.  If aliasing is taking
     * place, then concurrent writes and reads may not be
     * reflected correctly.  Filesystem and device
     * should prevent aliasing.
     */
    struct {

      inode_t* inode;
      offset_t offset;

    } inode;

    unsigned int anon_page;

  };

};

/* If page coloring is turned on, the functions take two
 * color tables.  If the vm object is shared, then the object's
 * table is used to decide on the color.  Otherwise the address
 * space's is used.
 */
#ifdef PAGE_COLOR
phys_addr_t phys_page_fetch(page_src_t src, unsigned int* spc_ct,
			    unsigned int* obj_ct, unsigned int shared);
unsigned int anon_page_create(page_src_t src, unsigned int* spc_ct,
			      unsigned int* obj_ct, unsigned int shared);
void anon_page_free(unsigned int page_id);
#else
phys_addr_t phys_page_fetch(page_src_t src);
unsigned int anon_page_create(page_src_t src);
void anon_page_free(unsigned int page_id);
#endif

#endif

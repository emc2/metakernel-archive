#ifndef VIRT_MEM_H
#define VIRT_MEM_H

/* Copyright (c) 2004 Eric McCorkle.  All rights reserved. */

#include <definitions.h>
#include <data_structures.h>
#include <types.h>
#include <sync.h>

/* vm_obj_t flags */
#define VMOBJ_PERM 0xf
#define VMOBJ_READ 0x1
#define VMOBJ_WRITE 0x2
#define VMOBJ_EXEC 0x4
#define VMOBJ_REF_COUNT 0x10
#define VMOBJ_COW 0x20
#define VMOBJ_COW_SRC 0x40
#define VMOBJ_RESERVED 0xffffff80

/* vm_obj creation types */
#define VMOBJ_INODE 1
#define VMOBJ_ANON 2
#define VMOBJ_COPY 3

typedef struct vmap_t {

  unsigned int flags;
  void* addr;
  vm_obj_t* obj;
  addr_spc_t* spc;
  unsigned int color;
  struct vmap_t* next;
  struct vmap_t* up;
  struct vmap_t* left;
  struct vmap_t* right;

} vmap_t;

struct addr_spc_t {

  unsigned int id;
  addr_spc_t* hash_next;
  unsigned int num_mappers;
  thread_t** mappers;
  vmap_t* objs;
  mutex_t mutex;
#ifdef PAGE_COLOR
  unsigned int* color_table;
#endif

};

union vm_src_t {

  struct {

    inode_t* inode;
    offset_t offset;

  } inode;

};

#define VMAP_HASH_TABLE_SIZE 31

STAT_HASH_TABLE_HEADERS(mappers, , vmap_t*, unsigned int,
			VMAP_HASH_TABLE_SIZE);

struct vm_obj_t {

  unsigned int id;
  unsigned int flags;
  unsigned int size;
  unsigned int num_mappers;
  mutex_t mutex;
  mappers_hash_table mappers;
  unsigned int src_type;
  vm_obj_t* cow_src;
  vm_obj_t* cow_next;
  vm_obj_t* cow_prev;
  vm_obj_t* hash_next;
  unsigned int cow_count;
#ifdef PAGE_COLOR
  unsigned int* color_table;
#endif
  union vm_src_t src;
  unsigned int anon_pages[];

};

extern vm_obj_t* kern_vm_obj;
extern addr_spc_t* kern_addr_spc;

addr_spc_t* addr_spc_create(void);
int addr_spc_destroy(addr_spc_t* spc);
vm_obj_t* vmobj_create(unsigned int type, unsigned int perm,
		       unsigned int size, void* a, offset_t b);
int vmobj_destroy(vm_obj_t* obj);
int vmobj_map(addr_spc_t* spc, vm_obj_t* obj,
	      void* addr, unsigned int flags);
int vmobj_unmap(addr_spc_t* spc, vm_obj_t* obj);
int vmobj_remap(addr_spc_t* spc, vm_obj_t* obj,
		void* addr, unsigned int flags);
void vm_invalidate_page(phys_addr_t);
phys_addr_t vm_evict_page(void);
void vm_page_fault(void* addr, unsigned int access);
void vm_protect_fault(void* addr, unsigned int access);
void kern_page_fault(void* addr, unsigned int access);
void kern_protect_fault(void* addr, unsigned int access);

#endif

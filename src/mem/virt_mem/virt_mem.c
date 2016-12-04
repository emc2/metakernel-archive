#include <definitions.h>
#include <kern_addr.h>
#include <virt_mem.h>
#include <arch_vmem.h>
#include <malloc.h>
#include <thread.h>
#include <util.h>
#include <sync.h>
#include <context.h>
#include "vm_fault.h"
#include "lru_page.h"
#include "cow_table.h"
#include "inv_page_table.h"

/* Copyright (c) 2004 Eric McCorkle.  All rights reserved. */
/* STATUS virt_mem: compiles */

#define ADDR_HASH_TABLE_SIZE 4093
#define VMOBJ_HASH_TABLE_SIZE 16383
#define PAGE_ADDR(x) (UINT_TO_PTR(PTR_TO_UINT(x) & ~PAGE_MASK))

#define hash_next(x) ((x)->hash_next)
#define un_eq(x, y) ((x) == (y))
#define un_id(x) ((x)->id)
#define null_hash(x) (x)
#define comp(x, y) ((x) > (y) ? 1 : (x) != (y) ? -1 : 0)
#define left(x) ((x)->left)
#define right(x) ((x)->right)
#define up(x) ((x)->up)
#define color(x) ((x)->color)
#define map_next(x) ((x)->next)
#define next(x) ((x)->cow_next)
#define last(x) ((x)->cow_prev)
#define map_id(x) ((x)->spc->id)

/* Ahh yes, the virtual memory system.  Where
 * else can you find five data structures all
 * entwined like some kind of Kama Sutra madness?
 */
STAT_HASH_TABLE_HEADERS(addr_spc, static inline, addr_spc_t*,
			unsigned int, ADDR_HASH_TABLE_SIZE)
STAT_HASH_TABLE(addr_spc, static inline, addr_spc_t*,
		unsigned int, hash_next, NULL, un_eq, un_id,
		null_hash, ADDR_HASH_TABLE_SIZE)
STAT_HASH_TABLE_HEADERS(vm_obj, static inline, vm_obj_t*,
			unsigned int, VMOBJ_HASH_TABLE_SIZE)
STAT_HASH_TABLE(vm_obj, static inline, vm_obj_t*,
		unsigned int, hash_next, NULL, un_eq, un_id,
		null_hash, VMOBJ_HASH_TABLE_SIZE)
RED_BLACK_TREE_HEADERS(vmap, static inline, vmap_t*)
RED_BLACK_TREE(vmap, static inline, vmap_t*, comp,
	       NULL, left, right, up, color)
CIRCULAR_LIST_HEADERS(vm_obj, static inline, vm_obj_t*)
CIRCULAR_LIST(vm_obj, static inline, vm_obj_t*,
	      next, last, NULL)
STAT_HASH_TABLE_HEADERS(vmap, static inline, vmap_t*,
			unsigned int, VMAP_HASH_TABLE_SIZE)
STAT_HASH_TABLE(vmap, static inline, vmap_t*, unsigned int,
		map_next, NULL, un_eq, map_id, null_hash,
		VMAP_HASH_TABLE_SIZE)
/* Doesn't that expand to like, 1000+ lines? */

static unsigned int addr_id_count = 1;
static unsigned int vm_id_count = 1;
static mutex_t addr_count_mutex;
static mutex_t vm_count_mutex;
static addr_spc_hash_table addr_tab = { NULL };
static vm_obj_hash_table vm_obj_tab = { NULL };
static mutex_t addr_tab_mutex;
static mutex_t vm_tab_mutex;
addr_spc_t kern_addr_spc;
vm_obj_t kern_vm_obj;

static inline vmap_t* get_closest_low(vmap_t* tree, void* addr) {

  vmap_t* last_right = NULL;

  while(NULL != tree) {

    if(addr < tree->addr)
      tree = tree->left;

    else if(addr > tree->addr) {

      last_right = tree;
      tree = tree->right;

    }

    /* if we just so happen to nail one, return it */
    else
      return tree;

  }

  return last_right;

}


static inline vmap_t* get_mapping_obj(addr_spc_t* spc, void* addr) {

  vmap_t* out;

  out = get_closest_low(spc->objs, addr);

  if(NULL != out && (char*)addr <
     (char*)(out->addr) + out->obj->size)
    return out;

  else
    return NULL;

}


static inline int collision_test(addr_spc_t* spc, void* addr, 
				 unsigned int size) {

  vmap_t* tree = spc->objs;
  vmap_t* low = NULL;
  vmap_t* high = NULL;

  /* find the low and high bounds on the address range */
  while(NULL != tree) {

    if(addr < tree->addr) {

      low = tree;
      tree = tree->left;

    }

    else if(addr > tree->addr) {

      high = tree;
      tree = tree->right;

    }

    /* if we just so happen to nail one, return */
    else
      return 1;

  }

  /* make sure the address range falls within the bounds */
  return !((char*)(low->addr) + low->obj->size < (char*)addr &&
	   (char*)high->addr > (char*)addr + size);
}


/* deletes the mapping on the object side */
static inline void delete_mapping(vmap_t* map) {

  addr_spc_t* spc = map->spc;
  vm_obj_t* obj = map->obj;

  obj->num_mappers--;
  vmap_hash_table_delete(map->obj->mappers, spc->id);

  /* garbage collect */
  if(0 == obj->mappers && obj->flags & VMOBJ_REF_COUNT)
    vmobj_destroy(obj);

}

/* unmaps all vm objects from an address space
 * prior to deleting the space itself
 */
static inline void unmap_all_objs(addr_spc_t* spc) {

  vmap_t* curr = spc->objs;

  /* this is an iterative postorder traversal */
  for(;;) {

    if(NULL != curr->left)
      curr = curr->left;

    else if(NULL != curr->right)
      curr = curr->right;

    else if(curr != spc->objs) {

      delete_mapping(curr);

      if(curr == curr->up->left) {

        curr = curr->up;
        free(curr->left);
        curr->left = NULL;

      }

      else {

        curr = curr->up;
        free(curr->right);
        curr->right = NULL;

      }

    }

    else {

      delete_mapping(curr);
      free(curr);
      break;

    }

  }

}


addr_spc_t* addr_spc_create(void) {

  unsigned int size = 2 * PAGE_SIZE;
  addr_spc_t* new;

  /* create, initialize, and insert */
  if(NULL != (new = kern_malloc(&size, 0))) {

    mutex_init(&(new->mutex));
    mutex_lock(&addr_count_mutex);
    new->id = addr_id_count++;
    mutex_unlock(&addr_count_mutex);
    new->num_mappers = 0;
    new->mappers = NULL;
    new->objs = NULL;
    mutex_lock(&addr_tab_mutex);
    addr_spc_hash_table_insert(addr_tab, new);
    mutex_unlock(&addr_tab_mutex);

    return new;

  }

  else
    return NULL;

}


int addr_spc_destroy(addr_spc_t* spc) {

  void* target;

  mutex_lock(&(spc->mutex));
  mutex_lock(&addr_tab_mutex);
  target = addr_spc_hash_table_delete(addr_tab, spc->id);
  mutex_unlock(&addr_tab_mutex);

  if(NULL != target) {

    /* It is illegal to destroy an address space
     * that someone is mapping.  Mark it garbage
     * collected instead.
     */

    if(0 != spc->num_mappers) {

      free(spc->mappers);
      unmap_all_objs(target);
      kern_free(target, PAGE_SIZE * 2);

      return 0;

    }

  }

  mutex_unlock(&(spc->mutex));

  return -1;

}


/* These a, b args are a very kludgy way to do var args... */

vm_obj_t* vmobj_create(unsigned int type, unsigned int perm,
		       unsigned int size, void* a, offset_t b) {

  int i;
  inode_t* inode;
  offset_t offset;
  vm_obj_t* new;
  vm_obj_t* copy;
  page_src_t page;

  switch(type) {

  case VMOBJ_INODE:

    inode = a;
    offset = b;

    /* XXX check inode permissions */

    /* get the memory and set everything up */
    if(NULL != (new = malloc(sizeof(vm_obj_t*)))) {

      /* XXX might want to make this randomized */
      mutex_lock(&vm_count_mutex);
      new->id = vm_id_count++;
      mutex_unlock(&vm_count_mutex);
      mutex_init(&(new->mutex));
      new->size = size;
      /* this is ok, since garbage collection checks
       * are done only when the object is unmapped
       */
      new->num_mappers = 0;
      memset(new->mappers, 0, VMAP_HASH_TABLE_SIZE);
      new->flags = perm;
      new->cow_next = NULL;
      new->cow_prev = NULL;
      new->cow_count = 0;
      new->src_type = VMOBJ_INODE;
      new->src.inode.inode = inode;
      new->src.inode.offset = offset;
      mutex_lock(&vm_tab_mutex);
      vm_obj_hash_table_insert(vm_obj_tab, new);
      mutex_unlock(&vm_tab_mutex);

    }

    else
      return NULL;

    break;

  case VMOBJ_ANON:

    if(NULL != (new = malloc(sizeof(vm_obj_t*) +
			     ((size / PAGE_SIZE) * 8)))) {

      /* XXX might want to make this randomized */
      mutex_lock(&vm_count_mutex);
      new->id = vm_id_count++;
      mutex_unlock(&vm_count_mutex);
      new->size = size;
      /* this is ok, since garbage collection checks
       * are done only when the object is unmapped
       */
      new->num_mappers = 0;
      mutex_init(&(new->mutex));
      memset(new->mappers, 0, VMAP_HASH_TABLE_SIZE);
      new->flags = perm;
      new->cow_next = NULL;
      new->cow_prev = NULL;
      new->cow_count = 0;
      new->src_type = VMOBJ_ANON;
      memset(new->anon_pages, 0, size / PAGE_SIZE);
      mutex_lock(&vm_tab_mutex);
      vm_obj_hash_table_insert(vm_obj_tab, new);
      mutex_unlock(&vm_tab_mutex);

    }

    else
      return NULL;

    break;

  case VMOBJ_COPY:

    copy = a;
    mutex_lock(&(copy->mutex));

    if(NULL != copy && perm & VMOBJ_WRITE &&
       NULL != (new = malloc(sizeof(vm_obj_t*) +
			     ((size / PAGE_SIZE) * 8)))) {

      /* XXX check permissions */
      mutex_lock(&vm_count_mutex);
      new->id = vm_id_count++;
      mutex_unlock(&vm_count_mutex);
      new->size = copy->size;
      /* this is ok, since garbage collection checks
       * are done only when the object is unmapped
       */
      new->num_mappers = 0;
      mutex_init(&(new->mutex));
      memset(new->mappers, 0, VMAP_HASH_TABLE_SIZE);
      new->flags = perm | VMOBJ_COW;
      /* The circular list may be eliminated */
      new->cow_next = new;
      new->cow_prev = new;
      new->cow_count = copy->size / PAGE_SIZE;
      copy->cow_count = copy->size / PAGE_SIZE;
      new->cow_src = copy;
      copy->flags &= VMOBJ_COW_SRC;
      /* last use of copy, unlock its mutex */
      mutex_unlock(&(copy->mutex));
      new->src_type = VMOBJ_ANON;

      /* optimization to avoid repeated if inside loop */

      /* increment the count on all cow-mapped pages */
      if(VMOBJ_INODE == new->cow_src->src_type)
	for(i = 0; i < (size / PAGE_SIZE); i++) {

	  /* Put in 0 for all the anonymous pages.  0 is a null
	   * entry, so if 0 is encountered in the page listing,
	   * we use the cow_src entry instead */
	  page.inode.inode = new->cow_src->src.inode.inode;
	  page.inode.offset = new->cow_src->src.inode.offset;
	  new->anon_pages[i] = 0;
	  cow_inc(page);

	}

      else
	for(i = 0; i < (size / PAGE_SIZE); i++) {

          /* Put in 0 for all the anonymous pages.  0 is a null
           * entry, so if 0 is encountered in the page listing,
           * we use the cow_src entry instead
	   */
	  page.anon_page = new->cow_src->anon_pages[i];
	  new->anon_pages[i] = 0;
	  cow_inc(page);

	}

      mutex_lock(&vm_tab_mutex);
      vm_obj_hash_table_insert(vm_obj_tab, new);
      mutex_unlock(&vm_tab_mutex);

    }

    else
      return NULL;

    break;

  default:

    return NULL;

  }

  return new;

}


int vmobj_destroy(vm_obj_t* obj) {

  vm_obj_t* target;

  mutex_lock(&(obj->mutex));
  mutex_lock(&vm_tab_mutex);
  target = vm_obj_hash_table_delete(vm_obj_tab, obj->id);
  mutex_unlock(&vm_tab_mutex);

  /* As with address spaces, it is illegal to destroy a
   * vm object that is mapped.  Use garbage collection
   * instead.
   */
  if(NULL != target || 0 != target->num_mappers) {

    /* this might get wiped out soon */
    vm_obj_clist_delete(target);
    free(target);

    return 0;

  }

  return -1;

}


/* maps a vm object into an address space */
int vmobj_map(addr_spc_t* spc, vm_obj_t* obj,
	      void* addr, unsigned int flags) {

  vmap_t* map;

  mutex_lock(&(spc->mutex));
  mutex_lock(&(obj->mutex));

  /* get the memory and check for address space collisions */
  if(NULL != spc && NULL != obj && 
     !collision_test(spc, addr, obj->size) &&
     NULL != (map = malloc(sizeof(vmap_t)))) {

    map->spc = spc;
    map->obj = obj;
    map->flags = flags & ~VMOBJ_RESERVED;
    map->addr = addr;
    obj->num_mappers++;
    /* XXX why am I locking this here ? */
    mutex_lock(&addr_tab_mutex);
    vmap_hash_table_insert(obj->mappers, map);
    mutex_unlock(&addr_tab_mutex);
    /* XXX ditto */
    mutex_lock(&vm_tab_mutex);
    vmap_tree_insert(&(spc->objs), map);
    mutex_unlock(&vm_tab_mutex);
    mutex_unlock(&(spc->mutex));
    mutex_unlock(&(obj->mutex));

    return 0;

  }

  mutex_unlock(&(obj->mutex));
  mutex_unlock(&(spc->mutex));

  return -1;

}


int vmobj_unmap(addr_spc_t* spc, vm_obj_t* obj) {

  vmap_t* map;

  mutex_lock(&(spc->mutex));
  mutex_lock(&(obj->mutex));

  if(NULL != spc && NULL != obj &&
     NULL != (map = vmap_hash_table_delete(obj->mappers, spc->id))) {

    vmap_tree_delete(&(spc->objs), map);
    obj->num_mappers--;

    /* garbage collect */
    if(0 == obj->mappers && obj->flags & VMOBJ_REF_COUNT)
      vmobj_destroy(obj);

    free(map);
    mutex_unlock(&(obj->mutex));
    mutex_unlock(&(spc->mutex));

    return 0;

  }

  mutex_unlock(&(obj->mutex));
  mutex_unlock(&(spc->mutex));

  return -1;

}


int vmobj_remap(addr_spc_t* spc, vm_obj_t* obj, void* addr,
		unsigned int flags) {

  vmap_t* map;

  mutex_lock(&addr_tab_mutex);
  spc = addr_spc_hash_table_lookup(addr_tab, spc->id);
  mutex_lock(&(spc->mutex));
  mutex_unlock(&addr_tab_mutex);
  mutex_lock(&vm_tab_mutex);
  obj = vm_obj_hash_table_lookup(vm_obj_tab, obj->id);
  mutex_lock(&(obj->mutex));
  mutex_unlock(&vm_tab_mutex);

  if(NULL != spc && NULL != obj &&
     NULL != (map = vmap_hash_table_lookup(obj->mappers, spc->id))) {

    /* XXX check inode permissions against possible new permissions */

    /* A quick exit condition.  If they're just changing
     * permissions or something, then don't muck about
     * with collision checking
     */
    if(map->spc == spc || 
       NULL == spc) {

      map->flags = flags & ~VMOBJ_RESERVED;
      mutex_unlock(&(obj->mutex));
      mutex_unlock(&(spc->mutex));

      return 0;

    }

    else {

      vmap_tree_delete(&(spc->objs), map);

      /* collisions are not allowed, and flags will
       * not be set if a collision occurs
       */
      if(!collision_test(spc, addr, obj->size)) {

	map->flags = flags & ~VMOBJ_RESERVED;
	map->addr = addr;
	mutex_lock(&addr_tab_mutex);
	vmap_hash_table_insert(obj->mappers, map);
	mutex_unlock(&addr_tab_mutex);
	mutex_lock(&vm_tab_mutex);
	vmap_tree_insert(&(spc->objs), map);
	mutex_unlock(&vm_tab_mutex);
	mutex_unlock(&(obj->mutex));
	mutex_unlock(&(spc->mutex));

	return 0;

      }

      /* if there is a collision, put it back */
      else {

	mutex_lock(&addr_tab_mutex);
	vmap_hash_table_insert(obj->mappers, map);
	mutex_unlock(&addr_tab_mutex);
	mutex_lock(&vm_tab_mutex);
	vmap_tree_insert(&(spc->objs), map);
	mutex_unlock(&vm_tab_mutex);

      }

    }

  }

  mutex_unlock(&(obj->mutex));
  mutex_unlock(&(spc->mutex));

  return -1;

}


static void vm_obj_page_unmap(vm_obj_t* obj, unsigned int offset) {

  int i;
  vmap_t* curr;

  mutex_lock(&(obj->mutex));

  for(i = 0; i < VMAP_HASH_TABLE_SIZE; i++) {

    curr = obj->mappers[i];

    while(NULL != curr) {

      mutex_lock(&(curr->spc->mutex));
      page_unmap(curr->spc, (char*)(curr->addr) + offset);
      mutex_unlock(&(curr->spc->mutex));
      curr = curr->next;

    }

  }

  mutex_unlock(&(obj->mutex));

}

void vm_invalidate_page(phys_addr_t phys_addr) {

  int i;
  inv_map_t* curr;
  inv_page_t* inv_page;

  inv_page = inv_page_lookup(phys_addr);
  mutex_lock(&(inv_page->mutex));
  inv_page->type = PAGE_UNUSED;
  inv_page->num_mappers = 0;

  /* cancel all page table entries */
  for(i = 0; i < INV_MAP_HASH_TABLE_SIZE; i++) {

    curr = inv_page->mappers[i];

    while(NULL != curr) {

      vm_obj_page_unmap(curr->obj, curr->offset);
      curr = curr->inv_next;

    }

  }

  mutex_unlock(&(inv_page->mutex));

}


phys_addr_t vm_evict_page() {

  phys_addr_t victim_page;

  victim_page = lru_page();
  vm_invalidate_page(victim_page);
  /* XXX try to swap out the page */
  /* XXX put it back if I fail */

  return victim_page;

}


static inline page_src_t get_page_src(addr_spc_t* spc, vm_obj_t* obj,
				      unsigned int offset) {

  page_src_t out;
  unsigned int anon;

  switch(obj->src_type) {

  case VMOBJ_INODE:

    out.inode.offset = offset + obj->src.inode.offset;
    out.inode.inode = obj->src.inode.inode;
    out.type = PSRC_INODE;

    break;

  case VMOBJ_ANON:

    if(!(obj->flags & VMOBJ_COW)) {

      /* anonymous pages may not have been allocated yet */
      if(0 != obj->anon_pages[offset])
	out.anon_page = obj->anon_pages[offset];

      else {

	/* optimization: use out instead of creating
	 * another page source
	 */
	out.type = PSRC_NULL;
	out.anon_page = obj->anon_pages[offset] =
	  anon_page_create(out, spc->color_table,
			   obj->color_table,
			   obj->num_mappers != 1);

      }

      out.type = PSRC_ANON;

    }

    else {

      anon = obj->anon_pages[offset];

      /* Page has already been copied, use the new version */
      if(0 != anon) {

	out.anon_page = anon;
	out.type = PSRC_ANON;

      }

      /* get the page from the copied object */
      else
	return get_page_src(spc, obj->cow_src, offset);

    }

    break;

  default:

    /* XXX kernel panic here */

    break;

  }

  return out;

}


static inline int check_access(unsigned int perm,
			       unsigned int access) {

  return perm == (perm | access);

}


static inline void update_page_tables(vm_obj_t* obj, unsigned int offset,
				      phys_addr_t phys_addr, void* addr) {

  vmap_t* curr;
  int i;
  inv_map_t* inv_map;
  inv_page_t* inv_page;

  inv_page = inv_page_lookup(phys_addr);
  mutex_lock(&(inv_page->mutex));
  inv_page->type = PAGE_USED;

  for(i = 0; i < VMAP_HASH_TABLE_SIZE; i++) {

    curr = obj->mappers[i];

    while(NULL != curr) {

      page_map(curr->spc, PAGE_ADDR(addr),
	       obj->flags & VMOBJ_PERM, phys_addr);
      /* set up the inverse page table entry */
      inv_map = malloc(sizeof(inv_map_t));
      inv_map->obj = obj;
      inv_map->offset = offset;
      inv_page->num_mappers++;
      inv_map_hash_table_insert(inv_page->mappers, inv_map);
      curr = curr->next;

    }

  }

  mutex_unlock(&(inv_page->mutex));

}


/* this one is abstracted because the page fault also calls it */
static inline void do_protect_fault(void* addr, vm_obj_t* obj,
				    unsigned int access,
				    unsigned int offset,
				    page_src_t orig) {

  addr_spc_t* curr;
  unsigned int anon;
  phys_addr_t phys_addr;
  page_src_t anon_src;
  vm_obj_t* list;

  curr = current_addr_spc();

  /* check copy-on-write first */
  if(obj->flags & (VMOBJ_COW | VMOBJ_WRITE | VMOBJ_COW_SRC) &&
     access & VMOBJ_WRITE) {

    /* Hopefully, this will be the case.  The writer is
     * not the mapper of the original source.
     */
    if(!(obj->flags & VMOBJ_COW_SRC) ||
       (VMOBJ_ANON == obj->src_type &&
	NULL == obj->cow_src)) {

      /* if only we map the page, and its anonymous, 
       * then keep it, otherwise, generate a clone
       */
      if(1 != cow_lookup(orig) ||
	 PSRC_INODE == orig.type) {

	/* clone the page */
	anon = anon_page_create(orig, curr->color_table,
				obj->color_table,
				obj->num_mappers != 1);
	anon_src.anon_page = anon;
	anon_src.type = PSRC_ANON;
	phys_addr = phys_page_fetch(anon_src, curr->color_table,
				    obj->color_table,
				    obj->num_mappers != 1);

      }

      else
        phys_addr = phys_page_fetch(orig, curr->color_table,
                                    obj->color_table,
                                    obj->num_mappers != 1);

      /* update the page tables and vm object
       * accordingly
       */
      /* XXX stop all threads mapping this object first 
       * (except myself)
       */
      obj->anon_pages[offset] = anon;
      update_page_tables(obj, offset, phys_addr, addr);
      /* update bookeeping */
      cow_dec(orig);
      obj->cow_count--;

      /* retire ourselves from the list if cow count hits 0 */
      if(0 == obj->cow_count) {

	obj->flags &= ~(VMOBJ_COW | VMOBJ_COW_SRC);
	vm_obj_clist_delete(obj);

      }

    }

    /* Oh shit...  We map the source...
     * Now we have to copy the page and give
     * the copy to everyone else.
     */
    else  {

      /* We could be a "monkey in the middle", ie,
       * copy the page, then others copy from us.
       * If so, then generate a copy, install it, then
       * continue.
       */
      if(obj->flags & VMOBJ_COW &&
	 0 == obj->anon_pages[offset]) {

	/* if only we map the page, and its anonymous,
	 * then keep it, otherwise, generate a clone
	 */
	if(1 != cow_lookup(orig) ||
	   PSRC_INODE == orig.type) {

	  /* clone the page */
	  anon = anon_page_create(orig, curr->color_table,
				  obj->color_table,
				  obj->num_mappers != 1);
	  anon_src.anon_page = anon;
	  anon_src.type = PSRC_ANON;
	  phys_addr = phys_page_fetch(anon_src, curr->color_table,
				      obj->color_table,
				      obj->num_mappers != 1);

	}

	else
	  phys_addr = phys_page_fetch(orig, curr->color_table,
				      obj->color_table,
				      obj->num_mappers != 1);


	/* update the page tables and vm object
	 * accordingly
	 */
	/* XXX stop all threads mapping this object first
	 * (except myself)
	 */
	obj->anon_pages[offset] = anon;
	update_page_tables(obj, offset, phys_addr, addr);
	/* update bookeeping */
	cow_dec(orig);
	obj->cow_count--;

	/* Now the monkey has its own copy, and
	 * so it is no longer in the middle.  Set
	 * orig to the new copy and continue.
	 */
	orig = anon_src;

      }

      /* generate the copy */
      if(1 != cow_lookup(orig)) {

        /* clone the page */
        anon = anon_page_create(orig, curr->color_table,
				obj->color_table,
				obj->num_mappers != 1);
        anon_src.anon_page = anon;
        anon_src.type = PSRC_ANON;
        phys_addr = phys_page_fetch(anon_src, curr->color_table,
                                    obj->color_table,
                                    obj->num_mappers != 1);

      }

      else
        phys_addr = phys_page_fetch(orig, curr->color_table,
                                    obj->color_table,
                                    obj->num_mappers != 1);

      /* update the page tables and vm object
       * accordingly
       */
      update_page_tables(obj, offset, phys_addr, addr);
      /* XXX change the cow_count bookeeping */

      /* Go through the cow list and give the new page to all
       * that share the original
       */
      list = obj->cow_next;

      while(obj != list) {

	if(VMOBJ_ANON != list->src_type ||
	   obj != list->cow_src ||
	   0 != list->anon_pages[offset])
	  continue;

	else {

	  /* XXX stop all threads mapping this vm object */
	  list->anon_pages[offset] = anon;
	  update_page_tables(list, offset, phys_addr, addr);

	}

	list = list->cow_next;

      }

      obj->cow_count--;

      /* retire ourselves from the list if cow count hits 0 */
      if(0 == obj->cow_count) {

        obj->flags &= ~(VMOBJ_COW | VMOBJ_COW_SRC);
        vm_obj_clist_delete(obj);

      }

    }

  }

  else
    illegal_access(addr, obj->flags & VMOBJ_PERM, access);

}


/* I don't know what kind of nuttiness the architecture
 * might throw at me, so I split the handler into protection
 * fault and page fault.
 */
void vm_page_fault(void* addr, unsigned int access) {

  unsigned int offset;
  vmap_t* map;
  vm_obj_t* obj;
  addr_spc_t* curr;
  phys_addr_t phys_addr;
  page_src_t page_src;
  unsigned int perm;
  inv_map_t* inv_map;
  inv_page_t* inv_page;

  /* XXX might want to disable the timer here for optimization */

  curr = current_addr_spc();

  /* The likely case: a page fault has occured
   * and the page is valid
   */
  if(NULL != (map = get_mapping_obj(curr, addr))) {

    obj = map->obj;
    offset = (PAGE_ADDR(addr) -
	      (char*)(map->addr)) >> PAGE_POW;
    page_src = get_page_src(curr, obj, offset);
    phys_addr = phys_page_fetch(page_src, curr->color_table,
				obj->color_table,
				obj->num_mappers != 1);

    /* check for copy-on-write, set read only if we detect it */
    if(!(obj->flags & (VMOBJ_COW | VMOBJ_COW_SRC)) ||
       !cow_lookup(page_src))
      perm = obj->flags & VMOBJ_PERM;

    else
      perm = obj->flags & VMOBJ_PERM & ~VMOBJ_WRITE;

    /* map the page */
    page_map(curr, PAGE_ADDR(addr),
	     perm, phys_addr);
    /* set up the inverse page table entry */
    inv_map = malloc(sizeof(inv_map_t));
    inv_map->obj = obj;
    inv_map->offset = offset;
    inv_page = inv_page_lookup(phys_addr);
    mutex_lock(&(inv_page->mutex));
    inv_page->num_mappers++;
    inv_page->type = PAGE_USED;
    inv_map_hash_table_insert(inv_page->mappers, inv_map);
    mutex_unlock(&(inv_page->mutex));

    if(check_access(perm, access))
      do_protect_fault(addr, obj, access, offset, page_src);

  }

  /* SMACK!! Access to unmapped virtual memory */
  else
    unmapped_access(addr);

}


void vm_protect_fault(void* addr, unsigned int access) {

  unsigned int offset;
  vmap_t* map;
  vm_obj_t* obj;
  addr_spc_t* curr;
  page_src_t page_src;

  /* XXX might want to disable the timer here for optimization */
  curr = current_addr_spc();
  map = get_mapping_obj(curr, addr);
  obj = map->obj;
  offset = (PAGE_ADDR(addr) -
	    (char*)(map->addr)) >> PAGE_POW;
  page_src = get_page_src(curr, obj, offset);
  do_protect_fault(addr, obj, access, offset, page_src);

}


/* The kernel's heap area will be demand-paged.
 * Thus, we have to be able to deal with page faults
 * in kernel mode, if they are caused by the heap.
 * needless to say, this function can't call malloc.
 */

void kern_page_fault(void* addr, unsigned int access) {

  page_src_t src;
  phys_addr_t phys_addr;
  inv_page_t* inv_page;

  /* page fault in the kernel heap */
  if(kern_heap_start > addr && kern_heap_end < addr) {

    src.type = PSRC_KERN;
    /* XXX fix this */
    src.anon_page =
      anon_page_create(src, kern_addr_spc.color_table,
		       kern_vm_obj.color_table, 0);
    phys_addr = phys_page_fetch(src, kern_addr_spc.color_table,
				kern_vm_obj.color_table, 0);
    kern_page_map(PAGE_ADDR(addr),
		  (VMOBJ_READ | VMOBJ_WRITE | VMOBJ_EXEC),
		  phys_addr);
    inv_page = inv_page_lookup(phys_addr);
    mutex_lock(&(inv_page->mutex));
    inv_page->type = PAGE_KERNEL;
    mutex_unlock(&(inv_page->mutex));

  }

  /* we could be going after user space and cause a
   * page fault
   */
  else if(kern_addr_start > addr)
    vm_page_fault(addr, access);

  else {

    /* XXX kernel panic here */

  }

}


/* kernel accesses to user space might cause a
 * copy-on-write protection fault
 */

void kern_protect_fault(void* addr, unsigned int access) {

  if(kern_addr_start > addr)
    vm_protect_fault(addr, access);

  else {

    /* XXX kernel panic here */

  }

}

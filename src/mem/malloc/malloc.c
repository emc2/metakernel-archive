#include <definitions.h>
#include <kern_addr.h>
#include <sync.h>
#include <data_structures.h>
#include <malloc.h>
#include <util.h>
#include <cache.h>
#include "round.h"

/* Copyright (c) 2004 Eric McCorkle.  All rights reserved. */
/* STATUS malloc.c: semi-frozen */

#define SMALLEST_BLOCK 16
#define BLOCK_MASK 0xf
#define LIST_OFFSET 4
#define LARGEST_BLOCK (1024 * 1024)
#define HASH_TABLE_SIZE ((256 * 256) - 1)
#define EMERGENCY_STORE 0x100000

#define next(x) ((x)->next)
#define last(x) ((x)->last)
#define hash_next(x) ((x)->hash_next)
#define un_eq(x, y) ((x) == (y))
#define un_id(x) ((void*)(x))
#define hash(x) (PTR_TO_UINT(x))


typedef struct mem_node {

  struct mem_node* next;
  struct mem_node* last;
  struct mem_node* hash_next;
  unsigned int size;

} mem_node;

STAT_HASH_TABLE_HEADERS(mem_node, static inline, mem_node*, void*,
			HASH_TABLE_SIZE)
STAT_HASH_TABLE(mem_node, static inline, mem_node*, void*, hash_next, NULL,
		un_eq, un_id, hash, HASH_TABLE_SIZE)
CIRCULAR_LIST_HEADERS(mem_node, static inline, mem_node*)
CIRCULAR_LIST(mem_node, static inline, mem_node*, next, last, NULL)

#ifdef MULTITHREAD
static mutex_t mutex;
static mutex_t stat_mutex;
#endif

static unsigned int high_list;
static mem_node** block_lists;
static mem_node_hash_table hash_table;
unsigned int heap_size;
static unsigned int heap_free;

static inline unsigned int lookup_addr(void* ptr) {

  mem_node* addr;

  mutex_lock(&mutex);
  addr = mem_node_hash_table_lookup(hash_table, ptr);  
  mutex_unlock(&mutex);

  return PTR_TO_UINT(addr);

}


static inline void insert_addr(void* ptr) {

  unsigned int list;
  mem_node* addr = ptr;

  list = addr_power(addr->size) - LIST_OFFSET;
  mutex_lock(&mutex);
  block_lists[list] = mem_node_clist_insert(block_lists[list], addr);
  mem_node_hash_table_insert(hash_table, addr);
  heap_free += addr->size;
  mutex_unlock(&mutex);

}


static inline void delete_addr(void* ptr) {

  unsigned int list;
  mem_node* addr = ptr;

  list = addr_power(addr->size) - LIST_OFFSET;
  mutex_lock(&mutex);
  block_lists[list] = mem_node_clist_delete(addr);
  mem_node_hash_table_delete(hash_table, addr);
  heap_free -= addr->size;
  mutex_unlock(&mutex);

}

static inline void delete_addr_nolock(void* ptr) {

  unsigned int list;
  mem_node* addr = ptr;

  list = addr_power(addr->size) - LIST_OFFSET;
  block_lists[list] = mem_node_clist_delete(addr);
  heap_free -= addr->size;
  mem_node_hash_table_delete(hash_table, addr);

}


static inline void merge_block_forward(void* ptr, unsigned int* size) {

  void* forward_ptr;

  /* keep going until the block is the proper size for its address */
  while(*size < addr_size(ptr)) {

    forward_ptr = ((char*)ptr) + *size;

    if(lookup_addr(forward_ptr)) {

      delete_addr(forward_ptr);
      *size += ((mem_node*)forward_ptr)->size;

    }

    else
      break;

  }

  ((mem_node*)ptr)->size = *size;

}


/* a version of the above function that doesn't
 * write into the block, and has a limit argument.
 */

static inline void live_merge_forward(void* ptr, unsigned int* size,
				      unsigned int limit) {

  void* forward_ptr;

  /* keep going until the block is the proper size for its
   * address or bigger than what is needed
   */
  while(*size < addr_size(ptr) &&
	*size < limit) {

    forward_ptr = ((char*)ptr) + *size;

    if(lookup_addr(forward_ptr)) {

      delete_addr(forward_ptr);
      *size += ((mem_node*)forward_ptr)->size;

    }

    else
      break;

  }

}


static inline void* merge_block(void* ptr, unsigned int size) {

  void* back_ptr;

  /* go back until there are no more adjacent blocks */
  while(size < LARGEST_BLOCK) {

    merge_block_forward(ptr, &size);

    if(size == addr_size(ptr)) {

      back_ptr = (char*)ptr - size;

      if(lookup_addr(back_ptr) &&
	 ((mem_node*)back_ptr)->size == size &&
	 size < LARGEST_BLOCK) {

	delete_addr(back_ptr);
	ptr = back_ptr;
	((mem_node*)back_ptr)->size *= 2;
	size = ((mem_node*)back_ptr)->size;

      }

      else
	break;

    }

    else
      break;

  }

  return ptr;

}


/* This function cuts a power of two block down as much
 * as it can.  It also updates the heap_free variable.
 */

static inline void split_block(void* ptr, unsigned int* size, unsigned int min_size) {

  unsigned int freed = 0;
  unsigned int diff;
  unsigned int pow_size;
  void* front_ptr;

  front_ptr = (char*)ptr + *size;
  diff = *size - min_size;

  while(diff >= SMALLEST_BLOCK) {

    /* break off the largest possible block */
    pow_size = round_down(diff);
    freed += pow_size;
    front_ptr = (char*)front_ptr - pow_size;
    diff -= pow_size;
    ((mem_node*)front_ptr)->size = pow_size;
    insert_addr(front_ptr);

  }

  *size = min_size + diff;
  mutex_lock(&stat_mutex);
  mutex_unlock(&stat_mutex);

}


/* This checks the free lists.  It also updates heap_free */

static inline void* acquire_block(unsigned int size) {

  void* out;
  unsigned int list;

  list = ilog(size) - LIST_OFFSET;

  for(; list <= high_list; list++) {

    mutex_lock(&mutex);

    if(NULL != block_lists[list]) {

      out = block_lists[list]->last;
      delete_addr_nolock(out);
      mutex_unlock(&mutex);

      return out;

    }

    mutex_unlock(&mutex);

  }

  return NULL;

}


/* looks ahead to see how much space is available */

static inline void* grow_block(void* ptr, unsigned int* size,
			      unsigned int new_size) {

  void* back_ptr;

  /* go back until there are no more adjacent blocks */
  while(*size < new_size) {

    live_merge_forward(ptr, size, new_size);

    if(*size == addr_size(ptr) &&
       *size < new_size ) {

      back_ptr = (char*)ptr - *size;

      if(lookup_addr(back_ptr) &&
         ((mem_node*)back_ptr)->size == *size) {

        delete_addr(back_ptr);
        ptr = back_ptr;
        ((mem_node*)back_ptr)->size *= 2;
	*size = ((mem_node*)back_ptr)->size;

      }

      else
        break;

    }

    else
      break;

  }

  return ptr;

}


static inline void* setup_block(void* ptr, unsigned int* size) {

  unsigned int block_size;

  block_size = *size;
  *size = ((mem_node*)ptr)->size;
  split_block(ptr, size, block_size);

  return ptr;

}


/* malloc: simple enough... */

void* kern_malloc(unsigned int* size, unsigned int priority) {

  void* out;
  unsigned int block_size;

  block_size = round_up(*size);
  mutex_lock(&stat_mutex);

  if(block_size <= (heap_free - EMERGENCY_STORE) ||
     priority != 0) {

    mutex_unlock(&stat_mutex);

    /* Easy case: we got it */
    if(NULL != (out = acquire_block(block_size)))
      return setup_block(out, size);

    /* Flush caches and try again */
    else {

      empty_caches();

      if(NULL != (out = acquire_block(block_size)))
	return setup_block(out, size);

      /* Damn it!  Can't get enough space! */
      else
	return NULL;

    }

  }

  /* Flush the caches and see if that freed enough space */
  else {

    mutex_unlock(&stat_mutex);
    empty_caches();
    mutex_lock(&stat_mutex);

    /* Check remaining space */
    if(block_size <= (heap_free - EMERGENCY_STORE) &&
       NULL != (out = acquire_block(block_size))) {

      mutex_unlock(&stat_mutex);

      return setup_block(out, size);

    }

    /* Damn!  Not enough space */
    else {

      mutex_unlock(&stat_mutex);

      return NULL;

    }

  }

}

/* cuts the end off of non-power-of-two blocks */
static inline void cut_block_end(void* ptr, unsigned int* min,
				 unsigned int size) {

  unsigned int start = (*min & ~BLOCK_MASK) + SMALLEST_BLOCK;
  void* curr = (char*)ptr + start;
  unsigned int free_size = size - start;
  unsigned int in_size;

  while(free_size != 0) {

    in_size = free_size > addr_size(curr) ?
      addr_size(curr) : round_down(free_size);
    ((mem_node*)curr)->size = in_size;
    insert_addr(curr);
    free_size -= in_size;
    curr = (char*)curr + in_size;

  }

  *min = start;

}


static inline void* trim_block(void* ptr, unsigned int* size,
		 unsigned int old_size) {

  live_merge_forward(ptr, &old_size, *size);
  cut_block_end(ptr, size, old_size);

  return ptr;

}


/* The most complicated one of all */

void* kern_realloc(void* ptr, unsigned int* size,
		   unsigned int old_size,
		   unsigned int priority) {

  unsigned int copy_size;
  void* out;
  void* grown;

  /* Easy case: free any extra memory and
   * return the original pointer
   */
  if(old_size >= *size)
    return trim_block(ptr, size, old_size);

  /* Bad cases */
  else {

    mutex_lock(&stat_mutex);

    if(*size - old_size <= 
       (heap_free - EMERGENCY_STORE) ||
       priority != 0) {
      
      mutex_unlock(&stat_mutex);
      copy_size = old_size;

      /* Try to grab additional memory */
      grown = grow_block(ptr, &old_size, *size);
      /* old_size now holds the size of the block */

      /* if its big enough, copy and return */
      if(old_size >= *size) {

	if(grown != ptr)
	  memmove(grown, ptr, copy_size);

	cut_block_end(grown, size, old_size);

	return grown;

      }

      /* Otherwise, go after more memory */
      else {

	if(NULL != (out = kern_malloc(size, 1))) {

	  memcpy(out, ptr, copy_size);
	  kern_free(grown, old_size);

	  return out;

	}

	/* Flush the caches and try again */
	else {

	  empty_caches();

	  if(NULL != (out = kern_malloc(size, 1))) {

	    memcpy(out, ptr, copy_size);
	    kern_free(grown, old_size);

	    return out;

	}

	/* Now, we're fucked.  There's no space */
	  else {

	    *size = old_size;

	    return NULL;

	  }

	}

      }

    }

    /* Flush the caches and try the same thing
     * above
     */
    else {

      mutex_unlock(&stat_mutex);
      empty_caches();    
      mutex_lock(&stat_mutex);

      /* Could still be not enough space */
      if(*size - old_size <=
	 (heap_free - EMERGENCY_STORE) ||
	 priority != 0) {

	mutex_unlock(&stat_mutex);
	copy_size = old_size;

	/* Try to grab additional memory */
	grown = grow_block(ptr, &old_size, *size);

	/* if its big enough, copy and return */
	if(old_size >= *size) {

	  if(grown != ptr)
	    memmove(grown, ptr, copy_size);

	  cut_block_end(grown, size, old_size);

	  return grown;

	}

	/* Otherwise, go after more memory */
	else {

	  if(NULL != (out = kern_malloc(size, 1))) {

	    memcpy(out, ptr, copy_size);
	    kern_free(grown, old_size);

	    return out;

	  }
	 
	  /* Flush the caches and try again */
	  else {

	    empty_caches();

	    if(NULL != (out = kern_malloc(size, 1))) {

	      memcpy(out, ptr, copy_size);
	      kern_free(grown, old_size);

	      return out;

	    }

	    /* Now, we're fucked.  There's no space */
	    else {

	      *size = old_size;

	      return NULL;

	    }

	  }

	}

      }

      /* Flushing the caches failed to free enough space */
      else {

	*size = old_size;

	return NULL;

      }

    }

  }

}


void kern_free(void* ptr, unsigned int size) {

  unsigned int pow_size;
  unsigned int i;

  if(NULL != ptr && 0 != size) {

    ((mem_node*)ptr)->size = size;
    ptr = merge_block(ptr, size);
    size = ((mem_node*)ptr)->size;

    /* After merging, insert blocks by powers of two */
    for(i = 0; i < size;) {

      pow_size = round_down(size - i);
      pow_size = pow_size > LARGEST_BLOCK ?
	LARGEST_BLOCK : pow_size;
      ((mem_node*)ptr)->size = pow_size;
      insert_addr(ptr);
      ptr = (char*)ptr + pow_size;
      i += pow_size;

    }

    mutex_lock(&stat_mutex);
    mutex_unlock(&stat_mutex);

  }

}


void* malloc(unsigned int size) {

  void* out;

  size += sizeof(unsigned int);

  if(NULL != (out = kern_malloc(&size, 0))) {

    /* wow...  I actually did it... */
    ((unsigned int*)out)[0] = size;
    out = (unsigned int*)out + 1;

  }

  return out;

}


void* realloc(void* ptr, unsigned int size) {

  void* out;

  size += sizeof(unsigned int);

  if(NULL != (out = kern_realloc(((unsigned int*)ptr) - 1, &size,
				 ((unsigned int*)ptr)[-1], 0))) {

    ((unsigned int*)out)[0] = size;
    out = (unsigned int*)out + 1;

  }

  return out;

}


void free(void* ptr) {

  kern_free(((unsigned int*)ptr) - 1,
	    ((unsigned int*)ptr)[-1]);

}


void kern_heap_init(void* start, unsigned int size) {

  kern_heap_start = start;
  kern_heap_end = (char*)start + size;
  heap_size = size;
  heap_free = size;

}

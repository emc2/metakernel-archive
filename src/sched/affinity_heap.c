#include <malloc.h>
#include "affinity_heap.h"
#include "priority_heap.h"
#include "scheduler.h"

/* Copyright (c) 2004 Eric McCorkle.  All rights reserved. */
/* STATUS affinity_heap.c: tested */

/* this is kind of wierd, to avoid the possibility of
 * calling realloc inside the timer interrupt
 */

#ifdef MULTITHREAD

affinity_heap* affinity_heap_init(unsigned int size) {

  affinity_heap* out;
  
  /* XXX WILL NOT USE MALLOC HERE EVENTUALLY */
  out = malloc(sizeof(affinity_heap));
  out->size = 0;
  out->max = size * sizeof(thread_t*);
  out->data_name = kern_malloc(&(out->max), 0);

  return out;

}


void affinity_heap_grow(affinity_heap* heap, unsigned int size) {

  unsigned int new_max;

  if(size >= (heap->max / sizeof(thread_t*))) {

    new_max = heap->max * 2;
    heap->data_name = kern_realloc(heap->data_name, &new_max, heap->max, 0);
    heap->max = new_max;

  }

}


/* size MUST be checked first */

void affinity_heap_insert(affinity_heap* heap, thread_t* in) {

  int i;
  int cont = 1;

  i = ++(heap->size);

  /* bubble up */
  while(i != 1 && in->affinity <
	heap->data_name[(i / 2) - 1]->affinity) {

    heap->data_name[(i / 2) - 1]->affinity_heap_pos = i;
    heap->data_name[i - 1] = heap->data_name[(i / 2) - 1];
    i /= 2;

  }

  /* fall down */
  while(cont) {

    if(((2 * i) + 1) <= heap->size &&
       heap->data_name[2 * i]->affinity <
       in->affinity) {

      if((2 * i) <= heap->size &&
	 heap->data_name[(2 * i) - 1]->affinity <
	 heap->data_name[2 * i]->affinity) {

        heap->data_name[(i * 2) - 1]->affinity_heap_pos = i;
	heap->data_name[i - 1] = heap->data_name[(i * 2) - 1];
	i *= 2;

      }

      else {

	heap->data_name[i * 2]->affinity_heap_pos = i;
	heap->data_name[i - 1] = heap->data_name[2 * i];
	i *= 2;
	i++;

      }

    }

    else if((2 * i) <= heap->size &&
            heap->data_name[(2 * i) - 1]->affinity <
	    in->affinity) {

      if(((2 * i) + 1) <= heap->size &&
	 heap->data_name[2 * i]->affinity <
	 heap->data_name[(2 * i) - 1]->affinity) {

	heap->data_name[i * 2]->affinity_heap_pos = i;
	heap->data_name[i - 1] = heap->data_name[2 * i];
	i *= 2;
	i++;

      }

      else {

        heap->data_name[(i * 2) - 1]->affinity_heap_pos = i;
	heap->data_name[i - 1] = heap->data_name[(i * 2) - 1];
	i *= 2;

      }

    }

    else
      cont = 0;

  }

  in->affinity_heap_pos = i;
  heap->data_name[i - 1] = in;

}


thread_t* affinity_heap_head(affinity_heap* heap) {

  if(0 != heap->size)
    return heap->data_name[0];

  else
    return NULL;

}


thread_t* affinity_heap_delete(affinity_heap* heap, unsigned int num) {

  int i = num;
  thread_t* out;
  thread_t* tmp;
  int cont = 1;

  if(0 == heap->size || 0 == num || num > heap->size)
    return NULL;

  /* pull up the bottom one */
  out = heap->data_name[num - 1];
  heap->size--;

  if(num - 1 == heap->size)
    return out;

  if(0 != heap->size) {

    tmp = heap->data_name[heap->size];

    while(i != 1 && tmp->affinity <
	  heap->data_name[(i / 2) - 1]->affinity) {

      heap->data_name[(i / 2) - 1]->affinity_heap_pos = i;
      heap->data_name[i - 1] = heap->data_name[(i / 2) - 1];
      i /= 2;

    }

    while(cont) {

      if(((2 * i) + 1) <= heap->size &&
	 heap->data_name[2 * i]->affinity <
	 tmp->affinity) {

	if((2 * i) <= heap->size &&
	   heap->data_name[(2 * i) - 1]->affinity <
	   heap->data_name[2 * i]->affinity) {

	  heap->data_name[(i * 2) - 1]->affinity_heap_pos = i;
	  heap->data_name[i - 1] = heap->data_name[(i * 2) - 1];
	  i *= 2;

	}

        else {

	  heap->data_name[2 * i]->affinity_heap_pos = i;
	  heap->data_name[i - 1] = heap->data_name[2 * i];
	  i *= 2;
	  i++;

	}

      }

      else if((2 * i) <= heap->size &&
	      heap->data_name[(2 * i) - 1]->affinity <
	      tmp->affinity) {

	if(((2 * i) + 1) <= heap->size &&
	   heap->data_name[2 * i]->affinity <
	   heap->data_name[(2 * i) - 1]->affinity) {

          heap->data_name[2 * i]->affinity_heap_pos = i;
	  heap->data_name[i - 1] = heap->data_name[2 * i];
	  i *= 2;
	  i++;

	}

	else {

          heap->data_name[(i * 2) - 1]->affinity_heap_pos = i;
	  heap->data_name[i - 1] = heap->data_name[(i * 2) - 1];
	  i *= 2;

	}

      }

      else
	cont = 0;

    }

    tmp->affinity_heap_pos = i;
    heap->data_name[i - 1] = tmp;

  }

  return out;

}


void affinity_heap_set_key(affinity_heap* heap,
			   unsigned int index,
			   unsigned int val) {

  int i = index;
  thread_t* tmp;
  int cont = 1;
  int direction;

  if(0 == i || i > heap->size ||
     heap->data_name[i - 1]->affinity == val)
    return;

  tmp = heap->data_name[i - 1];
  direction = tmp->affinity > val;
  tmp->affinity = val;

  /* only if we increase the key do we need to go up */
  if(direction) {

    if(1 == i)
      return;

    while(i != 1 && tmp->affinity <
	  heap->data_name[(i / 2) - 1]->affinity) {

      heap->data_name[(i / 2) - 1]->affinity_heap_pos = i;
      heap->data_name[i - 1] = heap->data_name[(i / 2) - 1];
      i /= 2;

    }

  }

  /* now decend */
  while(cont) {

    if(((2 * i) + 1) <= heap->size &&

       heap->data_name[2 * i]->affinity <
       tmp->affinity) {

      if((2 * i) <= heap->size &&
         heap->data_name[(2 * i) - 1]->affinity <
         heap->data_name[2 * i]->affinity) {

        heap->data_name[(i * 2) - 1]->affinity_heap_pos = i;
        heap->data_name[i - 1] = heap->data_name[(i * 2) - 1];
	i *= 2;

      }

      else {

        heap->data_name[i * 2]->affinity_heap_pos = i;
        heap->data_name[i - 1] = heap->data_name[2 * i];
        i *= 2;
        i++;

      }

    }

    else if((2 * i) <= heap->size &&
            heap->data_name[(2 * i) - 1]->affinity <
            tmp->affinity) {

      if(((2 * i) + 1) <= heap->size &&
         heap->data_name[2 * i]->affinity <
         heap->data_name[(2 * i) - 1]->affinity) {

        heap->data_name[i * 2]->affinity_heap_pos = i;
        heap->data_name[i - 1] = heap->data_name[2 * i];
        i *= 2;
        i++;

      }

      else {

        heap->data_name[(i * 2) - 1]->affinity_heap_pos = i;
        heap->data_name[i - 1] = heap->data_name[(i * 2) - 1];
        i *= 2;

      }

    }

    else
      cont = 0;

  }

  tmp->affinity_heap_pos = i;
  heap->data_name[i - 1] = tmp;

}

#endif

#include <definitions.h>
#include <data_structures.h>
#include <phys_mem.h>
#include <inv_page_table.h>
#include <static_alloc.h>
#include <sync.h>
#include "page_alloc.h"

/* Copyright (c) 2004 Eric McCorkle.  All rights reserved. */
/* STATUS page_alloc.c: compiles */

/* page color table format:
 *
 * first num_page_colors ints hold index of each color in ordered list
 * second num_page_colors ints hold count of each color
 * third num_page_colors ints hold ordered list of color
 */
#define color_index_tab(x) (x)
#define color_count_tab(x) ((x) + num_page_colors)
#define ordered_color_tab(x) ((x) + (2 * num_page_colors))
#define next(x) ((x)->lnext)
#define last(x) ((x)->llast)

CIRCULAR_LIST_HEADERS(page, static inline, inv_page_t*)
CIRCULAR_LIST(page, static inline, inv_page_t*, next, last, NULL)

/* number of page colors */
unsigned int num_page_colors;

/* array of lists of pages */
static inv_page_t** free_pages;
static unsigned int* page_count;
static mutex_t mutex;

void change_color_count(unsigned int* color_tab,
			unsigned int color, int num) {

  int i;
  unsigned int count;
  unsigned int* count_ptr;
  unsigned int* list_ptr;

  if(0 != num) {

    count_ptr = color_tab + num_page_colors;
    list_ptr = count_ptr + num_page_colors;
    i = color_tab[color] + 1;
    count = count_ptr[color] + num;
    count_ptr[color] = count;

    if(0 > num)
      for(; i != 1 && count < count_ptr[list_ptr[(i / 2) - 1]];) {

	list_ptr[i - 1] = list_ptr[(i / 2) - 1];
	color_tab[list_ptr[i - 1]] = i - 1;
	i /= 2;

      }

    for(;;) {

      if((2 * i) + 1 <= num_page_colors &&
	 count_ptr[list_ptr[2 * i]] < count) {

	if(count_ptr[list_ptr[2 * i]] <
	   count_ptr[list_ptr[(2 * i) - 1]]) {

	  list_ptr[i - 1] = list_ptr[2 * i];
	  color_tab[list_ptr[i - 1]] = i - 1;
	  i *= 2;
	  i++;

	}

	else {

	  list_ptr[i - 1] = list_ptr[(2 * i) - 1];
	  color_tab[list_ptr[i - 1]] = i - 1;
	  i *= 2;

	}

      }

      else if(2 * i <= num_page_colors &&
	      count_ptr[list_ptr[(2 * i) - 1]] < count) {

        if((2 * i) + 1 <= num_page_colors &&
	   count_ptr[list_ptr[2 * i]] <
           count_ptr[list_ptr[(2 * i) - 1]]) {

          list_ptr[i - 1] = list_ptr[2 * i];
          color_tab[list_ptr[i - 1]] = i - 1;
          i *= 2;
          i++;

	}

	else {

          list_ptr[i - 1] = list_ptr[(2 * i) - 1];
          color_tab[list_ptr[i - 1]] = i - 1;
          i *= 2;

        }

      }

      else
	break;

    }

    list_ptr[i - 1] = color;
    color_tab[color] = i - 1;

  }

}


phys_addr_t phys_page_alloc(unsigned int* color_tab) {

  int i;
  unsigned int* list_ptr = ordered_color_tab(color_tab);
  phys_addr_t out;

  /* keep going until we have it! */
  for(i = 0; i < num_page_colors; i++) {

    mutex_lock(&mutex);

    if(0 != page_count[list_ptr[i]]) {

      page_count[list_ptr[i]]--;
      out = inv_page_addr(free_pages[list_ptr[i]]);
      free_pages[list_ptr[i]] = page_clist_delete(free_pages[list_ptr[i]]);
      mutex_unlock(&mutex);

      return out;

    }

    mutex_unlock(&mutex);

  }

  return 0/*reclaim_pages(color_tab)*/;

}


phys_addr_t kern_page_alloc(unsigned int* color_tab) {

  unsigned int* list_ptr = ordered_color_tab(color_tab);
  phys_addr_t out;

  mutex_lock(&mutex);

  if(0 != page_count[list_ptr[0]]) {

    page_count[list_ptr[0]]--;
    out = inv_page_addr(free_pages[list_ptr[0]]);
    free_pages[list_ptr[0]] = page_clist_delete(free_pages[list_ptr[0]]);
    mutex_unlock(&mutex);
    change_color_count(color_tab, list_ptr[0], 1);

    return out;

  }

  else {

    mutex_unlock(&mutex);

    return 0/*steal_pages(color_tab)*/;

  }

}


/* Ahhh...  Easier functions */
static inline void phys_page_free_one(unsigned int* color_tab,
				      phys_addr_t addr) {

  unsigned int color = page_color(addr);
  inv_page_t* inv_page = inv_page_lookup(addr);

  change_color_count(color_tab, color, -1);
  mutex_lock(&mutex);
  free_pages[color] = page_clist_insert(free_pages[color], inv_page);
  page_count[page_color(addr)]++;
  mutex_unlock(&mutex);

}


void phys_page_insert(unsigned int num, const phys_addr_t* addrs) {

  int i;

  for(i = 0; i < num; i++) {

    mutex_lock(&mutex);
    free_pages[page_color(addrs[i])] =
      page_clist_insert(free_pages[page_color(addrs[i])],
			inv_page_lookup(addrs[i]));
    page_count[page_color(addrs[i])]++;
    mutex_unlock(&mutex);

  }

}


void phys_page_free(unsigned int num, unsigned int* color_tab,
		    const phys_addr_t* addrs) {

  int i;

  for(i = 0; i < num; i++)
    phys_page_free_one(color_tab, addrs[i]);

}


void page_alloc_static_init(unsigned int cache_size) {

  num_page_colors = cache_size / PAGE_SIZE;
  free_pages = static_alloc(num_page_colors * sizeof(inv_page_t*),
			    CACHE_LINE_SIZE);
  page_count = static_alloc(num_page_colors * sizeof(unsigned int),
			    CACHE_LINE_SIZE);

}

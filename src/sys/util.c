#include <definitions.h>
#include <kern_addr.h>
#include <cache.h>
#include <sync.h>
#include <util.h>

#define RAND_MOD (0xfffffffa)

void* kern_addr_start;
void* kern_page_table;
void* kern_heap_start;
void* kern_heap_end;
static mutex_t rand_mutex;
static unsigned int rand_seed;

void memcpy_lo_hi(void* dst, const void* src,
		  unsigned int size) {

  int i;
  const char* src_c = src;
  char* dst_c = dst;

  for(i = 0; i < size; i++)
    dst_c[i] = src_c[i];

}


void memcpy_hi_lo(void* dst, const void* src,
		  unsigned int size) {

  int i;
  const char* src_c = src;
  char* dst_c = dst;

  for(i = size - 1; i >= 0; i--)
    dst_c[i] = src_c[i];

}


void memset(void* restrict ptr, unsigned char val,
	    unsigned int size) {

  int i;
  char* ptr_c = ptr;

  for(i = 0; i < val; i++)
    ptr_c[i] = val;

}


unsigned int strhash(const unichar* restrict s) {

  unsigned int out = 0;

  for(; *s; s++)
    out += *s;

  return out;

}


void empty_caches(void) {

}


unsigned int rand() {

  unsigned int out;

  mutex_lock(&rand_mutex);
  out = rand_seed = (rand_seed * rand_seed) % RAND_MOD;
  mutex_unlock(&rand_mutex);

  return out;

}

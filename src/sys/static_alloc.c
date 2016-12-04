#include <definitions.h>
#include <static_alloc.h>
#include <util.h>

static void* limit;
static char* addr;

void* static_alloc(unsigned int size, unsigned int align) {

  void* out;

  out = addr = UINT_TO_PTR((((PTR_TO_UINT(addr) - 1) / align) + 1) * align);
  addr += size;

  while(addr > limit)
    limit = static_alloc_expand(limit);

  memset(out, 0, size);

  return out;

}


void static_alloc_init(void* ptr, void* lim) {

  addr = ptr;
  limit = lim;

}

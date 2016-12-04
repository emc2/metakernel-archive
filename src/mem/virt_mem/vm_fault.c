#include <definitions.h>
#include <panic.h>
#include "vm_fault.h"

void illegal_access(void* ptr) {

  kernel_panic(NULL);

}


void unmapped_access(void* ptr, unsigned int perm,
		     unsigned int access) {

  kernel_panic(NULL);

}

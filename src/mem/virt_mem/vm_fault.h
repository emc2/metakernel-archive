#ifndef VM_FAULT_H
#define VM_FAULT_H

void unmapped_access(void* addr);
void illegal_access(void* addr, unsigned int perm,
		    unsigned int access);

#endif

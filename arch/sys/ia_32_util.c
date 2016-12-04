#include <definitions.h>
#include <arch_vmem.h>
#include <util.h>
#include <timer.h>
#include <cpu.h>

void phys_memcpy(phys_addr_t dst, phys_addr_t src,
		 unsigned int size) {

  cpu_t* cpu = cpus + current_cpu();
  char* src_page = cpu->phys_memcpy_pages;
  char* dst_page = src_page + PAGE_SIZE;
  int i;

  timer_disable();

  for(i = 0; i < size;) {

    /* XXX replace permissions */
    kern_page_map(src_page, 0x7, src + (i * PAGE_SIZE));
    kern_page_map(dst_page, 0x7, dst + (i * PAGE_SIZE));
    memcpy(dst_page, src_page, PAGE_SIZE > size ? size : PAGE_SIZE);
    i += PAGE_SIZE;

  }

  timer_restore();

}


void phys_memset(phys_addr_t dst, unsigned char val,
                 unsigned int size) {

  cpu_t* cpu = cpus + current_cpu();
  char* dst_page = cpu->phys_memcpy_pages;
  int i;

  timer_disable();

  for(i = 0; i < size;) {

    /* XXX replace permissions */
    kern_page_map(dst_page, 0x7, dst + (i * PAGE_SIZE));
    memset(dst_page, val, PAGE_SIZE > size ? size : PAGE_SIZE);
    i += PAGE_SIZE;

  }

  timer_restore();

}

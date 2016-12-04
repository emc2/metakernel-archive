#include <definitions.h>
#include <static_alloc.h>
#include <virt_mem.h>
#include <phys_mem.h>
#include <kern_addr.h>
#include <inv_page_table.h>
#include <ia_32_cputabs.h>

#ifdef PAGE_COLOR

void mem_static_init(void* bios_memmap, unsigned int bios_map_size) {

  int i;
  unsigned int cache_size;
  void** virt_page_tab_map;

  if(!(cache_tlb_data.flags & CACHE_L3)) {

    /* Just in case the developers make a split top-level cache, AND
     * the instruction and data caches are different sizes.  If this
     * ever happens, somebody go to the developers and take away their
     * crackpipes.
     */
    cache_size = !(cache_tlb_data.flags & CACHE_L2_SPLIT) ?
      cache_tlb_data.l2_dcache :
      (cache_tlb_data.l2_dcache < cache_tlb_data.l2_icache) ?
      cache_tlb_data.l2_dcache : cache_tlb_data.l2_icache;
    page_alloc_static_init(cache_size);

  }

  /* allocate kernel address space, vm object, and page tables */
  kern_vm_obj = static_alloc(sizeof(kern_vm_obj) +
                             (sizeof(unsigned int) * kern_pages),
                             CACHE_LINE_SIZE);
  kern_addr_spc = static_alloc(3 * PAGE_SIZE, PAGE_SIZE);
  kern_page_tables = static_alloc(sizeof(unsigned int) * kern_pages,
				  PAGE_SIZE);
  /* the next one after the page tables must be page-aligned */
  memset(kern_vm_obj, 0, sizeof(kern_vm_obj) +
	 (sizeof(unsigned int) * kern_pages));
  memset(kern_addr_spc, 0, 3 * PAGE_SIZE);
  memset(kern_page_tables, 0, sizeof(unsigned int) * kern_pages);
  virt_page_tab_map = (void**)((char*)kern_addr_spc + (2 * PAGE_SIZE));

  for(i = (kern_addr_start >> 22); i < 1024; i++)
    virt_page_tab_map[i] = (char*)kern_page_tables + (PAGE_SIZE * i);

  /* XXX read the bios memory map */

}


unsigned int bios_map_high_addr(void* bios_map, unsigned int size) {

  unsigned int high_addr = 0;
  int i;

  for(i = 0; i < size;) {


    i += 20;

  }

}

#endif

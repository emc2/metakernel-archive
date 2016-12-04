#include <definitions.h>
#include <ia_32_cputabs.h>
#include <interrupt.h>
#include <panic.h>
#include <cpu.h>

u_64 idt[256];
u_64* gdt;
void* tss_array;
unichar cpu_vendor_str[13];
unsigned int cpu_model;
unsigned int cpu_brand;
unsigned int cpu_family;
unsigned int cpu_stepping;
unsigned int cpu_features;
unsigned int cpu_logical;
cache_tlb_t cache_tlb_data;

void idt_init(void) {

  int i;

  /* die for any fault */
  for(i = 0; i < 32; i++)
    int_set_handler(i, int_fatal);

  /* ignore all interrupts by default */
  for(; i < 256; i++)
    int_set_handler(i, int_ignore);

  /* setup ipi's */
  int_set_handler(IPI_STOP, ipi_stop);
  /*  int_set_handler(IPI_RESCHED, ipi_resched); */
  int_set_handler(IPI_RESTART, ipi_restart);
  int_set_handler(IPI_RESTART_TLBINV, ipi_restart_tlbinv);
  int_set_handler(IPI_RESTART_KERN_TLBINV, ipi_restart_kern_tlbinv);
  int_set_handler(IPI_HALT, ipi_halt);

}


static inline void cache_desc_parse(unsigned char desc) {

  switch(desc) {

  case 0x00:
  case 0x40:

    break;

  case 0x01:

    cache_tlb_data.flags |= TLB_INSTR_SPLIT | TLB_INSTR;
    cache_tlb_data.itlb_norm = 32;
    cache_tlb_data.itlb_norm_assoc = 4;
    break;

  case 0x02:

    cache_tlb_data.flags |= TLB_INSTR_SPLIT | TLB_INSTR;
    cache_tlb_data.itlb_super = 2;
    cache_tlb_data.itlb_super_assoc = 4;
    break;

  case 0x03:

    cache_tlb_data.flags |= TLB_DATA_SPLIT | TLB_DATA;
    cache_tlb_data.dtlb_norm = 64;
    cache_tlb_data.dtlb_norm_assoc = 4;
    break;

  case 0x04:

    cache_tlb_data.flags |= TLB_DATA_SPLIT | TLB_DATA;
    cache_tlb_data.dtlb_super = 8;
    cache_tlb_data.dtlb_super_assoc = 4;
    break;

  case 0x06:

    cache_tlb_data.flags |= CACHE_L1 | CACHE_L1_SPLIT;
    cache_tlb_data.l1_icache = 8192;
    cache_tlb_data.l1_icache_assoc = 4;
    cache_tlb_data.l1_icache_line = 32;
    break;

  case 0x08:

    cache_tlb_data.flags |= CACHE_L1 | CACHE_L1_SPLIT;
    cache_tlb_data.l1_icache = 16384;
    cache_tlb_data.l1_icache_assoc = 4;
    cache_tlb_data.l1_icache_line = 32;
    break;

  case 0x0a:

    cache_tlb_data.flags |= CACHE_L1 | CACHE_L1_SPLIT;
    cache_tlb_data.l1_dcache = 8192;
    cache_tlb_data.l1_dcache_assoc = 2;
    cache_tlb_data.l1_dcache_line = 32;
    break;

  case 0x0c:

    cache_tlb_data.flags |= CACHE_L1 | CACHE_L1_SPLIT;
    cache_tlb_data.l1_dcache = 16384;
    cache_tlb_data.l1_dcache_assoc = 4;
    cache_tlb_data.l1_dcache_line = 32;
    break;

  case 0x22:

    cache_tlb_data.flags |= CACHE_L3;
    cache_tlb_data.l3_dcache = 524288;
    cache_tlb_data.l3_dcache_assoc = 4;
    cache_tlb_data.l3_dcache_line = 128;
    cache_tlb_data.l3_icache = 524288;
    cache_tlb_data.l3_icache_assoc = 4;
    cache_tlb_data.l3_icache_line = 128;
    break;

  case 0x23:

    cache_tlb_data.flags |= CACHE_L3;
    cache_tlb_data.l3_dcache = 1048576;
    cache_tlb_data.l3_dcache_assoc = 8;
    cache_tlb_data.l3_dcache_line = 128;
    cache_tlb_data.l3_icache = 1048576;
    cache_tlb_data.l3_icache_assoc = 8;
    cache_tlb_data.l3_icache_line = 128;
    break;

  case 0x25:

    cache_tlb_data.flags |= CACHE_L3;
    cache_tlb_data.l3_dcache = 2097152;
    cache_tlb_data.l3_dcache_assoc = 8;
    cache_tlb_data.l3_dcache_line = 128;
    cache_tlb_data.l3_icache = 2097152;
    cache_tlb_data.l3_icache_assoc = 8;
    cache_tlb_data.l3_icache_line = 128;
    break;

  case 0x2c:

    cache_tlb_data.flags |= CACHE_L1 | CACHE_L1_SPLIT;
    cache_tlb_data.l1_icache = 32768;
    cache_tlb_data.l1_icache_assoc = 8;
    cache_tlb_data.l1_icache_line = 64;
    break;

  case 0x30:

    cache_tlb_data.flags |= CACHE_L1 | CACHE_L1_SPLIT;
    cache_tlb_data.l1_dcache = 32768;
    cache_tlb_data.l1_dcache_assoc = 8;
    cache_tlb_data.l1_dcache_line = 64;
    break;

  case 0x41:

    cache_tlb_data.flags |= CACHE_L2;
    cache_tlb_data.l1_dcache = 131072;
    cache_tlb_data.l1_dcache_assoc = 4;
    cache_tlb_data.l1_dcache_line = 32;
    cache_tlb_data.l1_icache = 131072;
    cache_tlb_data.l1_icache_assoc = 4;
    cache_tlb_data.l1_icache_line = 32;
    break;

  case 0x42:

    cache_tlb_data.flags |= CACHE_L2;
    cache_tlb_data.l1_dcache = 262144;
    cache_tlb_data.l1_dcache_assoc = 4;
    cache_tlb_data.l1_dcache_line = 32;
    cache_tlb_data.l1_icache = 262144;
    cache_tlb_data.l1_icache_assoc = 4;
    cache_tlb_data.l1_icache_line = 32;
    break;

  case 0x43:

    cache_tlb_data.flags |= CACHE_L2;
    cache_tlb_data.l1_dcache = 524288;
    cache_tlb_data.l1_dcache_assoc = 4;
    cache_tlb_data.l1_dcache_line = 32;
    cache_tlb_data.l1_icache = 524288;
    cache_tlb_data.l1_icache_assoc = 4;
    cache_tlb_data.l1_icache_line = 32;
    break;

  case 0x44:

    cache_tlb_data.flags |= CACHE_L2;
    cache_tlb_data.l1_dcache = 1048576;
    cache_tlb_data.l1_dcache_assoc = 4;
    cache_tlb_data.l1_dcache_line = 32;
    cache_tlb_data.l1_icache = 1048576;
    cache_tlb_data.l1_icache_assoc = 4;
    cache_tlb_data.l1_icache_line = 32;
    break;

  case 0x45:

    cache_tlb_data.flags |= CACHE_L2;
    cache_tlb_data.l1_dcache = 2097152;
    cache_tlb_data.l1_dcache_assoc = 4;
    cache_tlb_data.l1_dcache_line = 32;
    cache_tlb_data.l1_icache = 2097152;
    cache_tlb_data.l1_icache_assoc = 4;
    cache_tlb_data.l1_icache_line = 32;
    break;

  case 0x50:

    cache_tlb_data.flags |= TLB_INSTR;
    cache_tlb_data.itlb_norm = 64;
    cache_tlb_data.itlb_norm_assoc = 64;
    cache_tlb_data.itlb_super = 64;
    cache_tlb_data.itlb_super_assoc = 64;
    break;

  case 0x51:

    cache_tlb_data.flags |= TLB_INSTR;
    cache_tlb_data.itlb_norm = 128;
    cache_tlb_data.itlb_norm_assoc =128;
    cache_tlb_data.itlb_super = 128;
    cache_tlb_data.itlb_super_assoc = 128;
    break;

  case 0x52:

    cache_tlb_data.flags |= TLB_INSTR;
    cache_tlb_data.itlb_norm = 256;
    cache_tlb_data.itlb_norm_assoc = 256;
    cache_tlb_data.itlb_super = 256;
    cache_tlb_data.itlb_super_assoc = 256;
    break;

  case 0x5b:

    cache_tlb_data.flags |= TLB_DATA;
    cache_tlb_data.dtlb_norm = 64;
    cache_tlb_data.dtlb_norm_assoc = 64;
    cache_tlb_data.dtlb_super = 64;
    cache_tlb_data.dtlb_super_assoc = 64;
    break;

  case 0x5c:

    cache_tlb_data.flags |= TLB_DATA;
    cache_tlb_data.dtlb_norm = 128;
    cache_tlb_data.dtlb_norm_assoc = 128;
    cache_tlb_data.dtlb_super = 128;
    cache_tlb_data.dtlb_super_assoc = 128;
    break;

  case 0x5d:

    cache_tlb_data.flags |= TLB_DATA;
    cache_tlb_data.dtlb_norm = 256;
    cache_tlb_data.dtlb_norm_assoc = 256;
    cache_tlb_data.dtlb_super = 256;
    cache_tlb_data.dtlb_super_assoc = 256;
    break;

  case 0x66:

    cache_tlb_data.flags |= CACHE_L1 | CACHE_L1_SPLIT;
    cache_tlb_data.l1_dcache = 8192;
    cache_tlb_data.l1_dcache_assoc = 4;
    cache_tlb_data.l1_dcache_line = 64;
    break;

  case 0x67:

    cache_tlb_data.flags |= CACHE_L1 | CACHE_L1_SPLIT;
    cache_tlb_data.l1_dcache = 16384;
    cache_tlb_data.l1_dcache_assoc = 4;
    cache_tlb_data.l1_dcache_line = 64;
    break;

  case 0x68:

    cache_tlb_data.flags |= CACHE_L1 | CACHE_L1_SPLIT;
    cache_tlb_data.l1_dcache = 32768;
    cache_tlb_data.l1_dcache_assoc = 4;
    cache_tlb_data.l1_dcache_line = 64;
    break;

  case 0x70:

    cache_tlb_data.trace_cache = 12288;
    break;

  case 0x71:

    cache_tlb_data.trace_cache = 16384;
    break;

  case 0x72:

    cache_tlb_data.trace_cache = 32768;
    break;

  case 0x78:

    cache_tlb_data.flags |= CACHE_L2;
    cache_tlb_data.l1_dcache = 1048576;
    cache_tlb_data.l1_dcache_assoc = 8;
    cache_tlb_data.l1_dcache_line = 64;
    cache_tlb_data.l1_icache = 1048576;
    cache_tlb_data.l1_icache_assoc = 8;
    cache_tlb_data.l1_icache_line = 64;
    break;

  case 0x79:

    cache_tlb_data.flags |= CACHE_L2;
    cache_tlb_data.l1_dcache = 131072;
    cache_tlb_data.l1_dcache_assoc = 8;
    cache_tlb_data.l1_dcache_line = 128;
    cache_tlb_data.l1_icache = 131072;
    cache_tlb_data.l1_icache_assoc = 8;
    cache_tlb_data.l1_icache_line = 128;
    break;

  case 0x7a:

    cache_tlb_data.flags |= CACHE_L2;
    cache_tlb_data.l1_dcache = 262144;
    cache_tlb_data.l1_dcache_assoc = 8;
    cache_tlb_data.l1_dcache_line = 128;
    cache_tlb_data.l1_icache = 262144;
    cache_tlb_data.l1_icache_assoc = 8;
    cache_tlb_data.l1_icache_line = 128;
    break;

  case 0x7b:

    cache_tlb_data.flags |= CACHE_L2;
    cache_tlb_data.l1_dcache = 524288;
    cache_tlb_data.l1_dcache_assoc = 8;
    cache_tlb_data.l1_dcache_line = 128;
    cache_tlb_data.l1_icache = 524288;
    cache_tlb_data.l1_icache_assoc = 8;
    cache_tlb_data.l1_icache_line = 128;
    break;

  case 0x7c:

    cache_tlb_data.flags |= CACHE_L2;
    cache_tlb_data.l1_dcache = 1048576;
    cache_tlb_data.l1_dcache_assoc = 8;
    cache_tlb_data.l1_dcache_line = 128;
    cache_tlb_data.l1_icache = 1048576;
    cache_tlb_data.l1_icache_assoc = 8;
    cache_tlb_data.l1_icache_line = 128;
    break;

  case 0x7d:

    cache_tlb_data.flags |= CACHE_L2;
    cache_tlb_data.l1_dcache = 2097152;
    cache_tlb_data.l1_dcache_assoc = 8;
    cache_tlb_data.l1_dcache_line = 64;
    cache_tlb_data.l1_icache = 2097152;
    cache_tlb_data.l1_icache_assoc = 8;
    cache_tlb_data.l1_icache_line = 64;
    break;

  case 0x82:

    cache_tlb_data.flags |= CACHE_L2;
    cache_tlb_data.l1_dcache = 262144;
    cache_tlb_data.l1_dcache_assoc = 8;
    cache_tlb_data.l1_dcache_line = 32;
    cache_tlb_data.l1_icache = 262144;
    cache_tlb_data.l1_icache_assoc = 8;
    cache_tlb_data.l1_icache_line = 32;
    break;

  case 0x83:

    cache_tlb_data.flags |= CACHE_L2;
    cache_tlb_data.l1_dcache = 524288;
    cache_tlb_data.l1_dcache_assoc = 8;
    cache_tlb_data.l1_dcache_line = 32;
    cache_tlb_data.l1_icache = 524288;
    cache_tlb_data.l1_icache_assoc = 8;
    cache_tlb_data.l1_icache_line = 32;
    break;

  case 0x84:

    cache_tlb_data.flags |= CACHE_L2;
    cache_tlb_data.l1_dcache = 1048576;
    cache_tlb_data.l1_dcache_assoc = 8;
    cache_tlb_data.l1_dcache_line = 32;
    cache_tlb_data.l1_icache = 1048576;
    cache_tlb_data.l1_icache_assoc = 8;
    cache_tlb_data.l1_icache_line = 32;
    break;

  case 0x85:

    cache_tlb_data.flags |= CACHE_L2;
    cache_tlb_data.l1_dcache = 2097152;
    cache_tlb_data.l1_dcache_assoc = 8;
    cache_tlb_data.l1_dcache_line = 32;
    cache_tlb_data.l1_icache = 2097152;
    cache_tlb_data.l1_icache_assoc = 8;
    cache_tlb_data.l1_icache_line = 32;
    break;

  case 0x86:

    cache_tlb_data.flags |= CACHE_L2;
    cache_tlb_data.l1_dcache = 524288;
    cache_tlb_data.l1_dcache_assoc = 8;
    cache_tlb_data.l1_dcache_line = 64;
    cache_tlb_data.l1_icache = 524288;
    cache_tlb_data.l1_icache_assoc = 8;
    cache_tlb_data.l1_icache_line = 64;
    break;

  case 0x87:

    cache_tlb_data.flags |= CACHE_L2;
    cache_tlb_data.l1_dcache = 1048576;
    cache_tlb_data.l1_dcache_assoc = 8;
    cache_tlb_data.l1_dcache_line = 64;
    cache_tlb_data.l1_icache = 1048576;
    cache_tlb_data.l1_icache_assoc = 8;
    cache_tlb_data.l1_icache_line = 64;
    break;

  case 0xb0:

    cache_tlb_data.flags |= TLB_INSTR_SPLIT;
    cache_tlb_data.itlb_super = 128;
    cache_tlb_data.itlb_super_assoc = 4;
    break;

  case 0xb3:

    cache_tlb_data.flags |= TLB_DATA_SPLIT;
    cache_tlb_data.dtlb_super = 128;
    cache_tlb_data.dtlb_super_assoc = 4;
    break;

  default:

    break;

  }

}


void cpuid_parse(void) {

  unsigned int i;
  unsigned int j;
  unsigned int max;
  unsigned int cache_max;
  unsigned int buf[4];

  cpuid_store(0, buf);

  if(1 > (max = buf[0]))
    kernel_panic(NULL);

  cpu_vendor_str[0] = buf[1] & 0xff;
  cpu_vendor_str[1] = (buf[1] >> 8) & 0xff;
  cpu_vendor_str[2] = (buf[1] >> 16) & 0xff;
  cpu_vendor_str[3] = (buf[1] >> 24) & 0xff;
  cpu_vendor_str[4] = buf[3] & 0xff;
  cpu_vendor_str[5] = (buf[3] >> 8) & 0xff;
  cpu_vendor_str[6] = (buf[3] >> 16) & 0xff;
  cpu_vendor_str[7] = (buf[3] >> 24) & 0xff;
  cpu_vendor_str[8] = buf[2] & 0xff;
  cpu_vendor_str[9] = (buf[2] >> 8) & 0xff;
  cpu_vendor_str[10] = (buf[2] >> 16) & 0xff;
  cpu_vendor_str[11] = (buf[2] >> 24) & 0xff;
  cpu_vendor_str[12] = 0;

  cpuid_store(1, buf);
  cpu_features = buf[3];

  if((cpu_features & CPU_REQUIRED) != CPU_REQUIRED)
    kernel_panic(NULL);

  cpu_brand = buf[1] & 0x000000ff;
  cpu_logical = (buf[1] >> 16) & 0x000000ff;
  cpu_stepping = buf[0] & 0x000000f;
  cpu_model = ((buf[0] >> 4) & 0x0000000f) |
    ((buf[0] >> 12) & 0x000000f0);
  cpu_family = ((buf[0] >> 8) & 0x0000000f) |
    ((buf[0] >> 16) & 0x00000ff0);

  /* default cache statistics */
  cache_tlb_data.flags = CACHE_L1 | CACHE_L1_SPLIT |
    CACHE_L2 | TLB_INSTR | TLB_DATA;
  cache_tlb_data.trace_cache = 0;
  cache_tlb_data.l1_dcache = 16384;
  cache_tlb_data.l1_icache = 16384;
  cache_tlb_data.l2_dcache = 262144;
  cache_tlb_data.l2_icache = 262144;
  cache_tlb_data.l3_dcache = 0;
  cache_tlb_data.l3_icache = 0;
  cache_tlb_data.l1_dcache_assoc = 4;
  cache_tlb_data.l1_icache_assoc = 4;
  cache_tlb_data.l2_dcache_assoc = 4;
  cache_tlb_data.l2_icache_assoc = 4;
  cache_tlb_data.l3_dcache_assoc = 0;
  cache_tlb_data.l3_icache_assoc = 0;
  cache_tlb_data.l1_dcache_line = 4;
  cache_tlb_data.l1_icache_line = 4;
  cache_tlb_data.l2_dcache_line = 4;
  cache_tlb_data.l2_icache_line = 4;
  cache_tlb_data.l3_dcache_line = 0;
  cache_tlb_data.l3_icache_line = 0;
  cache_tlb_data.itlb_norm = 128;
  cache_tlb_data.itlb_super = 128;
  cache_tlb_data.dtlb_norm = 128;
  cache_tlb_data.dtlb_super = 128;
  cache_tlb_data.itlb_norm_assoc = 4;
  cache_tlb_data.itlb_super_assoc = 4;
  cache_tlb_data.dtlb_norm_assoc = 4;
  cache_tlb_data.dtlb_super_assoc = 4;

  if(2 < max) {

    cpuid_store(2, buf);
    cache_max = buf[0] & 0x000000ff;

    for(i = 0; i < cache_max; i++) {

      for(j = 1; j < 16;)
	if(j & 0xfffffffc)
	  cache_desc_parse(((char*)buf)[j++]);

	else
	  j += 4;

      cpuid_store(2, buf);

    }

  }

}

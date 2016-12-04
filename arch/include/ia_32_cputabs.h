#ifndef IA_32_CPUTABS_H
#define IA_32_CPUTABS_H

#define GDT_NULL 0
#define GDT_KERN_CODE 1
#define GDT_KERN_DATA 2
#define GDT_USER_CODE 3
#define GDT_USER_DATA 4
#define GDT_TSS_START 5
#define CACHE_L1 0x1
#define CACHE_L2 0x4
#define CACHE_L3 0x10
#define CACHE_L1_SPLIT 0x2
#define CACHE_L2_SPLIT 0x8
#define CACHE_L3_SPLIT 0x20
#define TLB_INSTR 0x100
#define TLB_DATA 0x400
#define TLB_INSTR_SPLIT 0x200
#define TLB_DATA_SPLIT 0x800
#define IA_32_X87 0x1
#define IA_32_VME 0x2
#define IA_32_DEBUG 0x4
#define IA_32_PSE 0x8
#define IA_32_TSC 0x10
#define IA_32_MSR 0x20
#define IA_32_PAE 0x40
#define IA_32_MCE 0x80
#define IA_32_CX8 0x100
#define IA_32_APIC 0x200
#define IA_32_SEP 0x800
#define IA_32_MTRR 0x1000
#define IA_32_PGE 0x2000
#define IA_32_MCA 0x4000
#define IA_32_CMOV 0x8000
#define IA_32_PAT 0x10000
#define IA_32_PSE_36 0x20000
#define IA_32_PSN 0x40000
#define IA_32_CFLUSH 0x80000
#define IA_32_DS 0x200000
#define IA_32_ACPI 0x400000
#define IA_32_MMX 0x800000
#define IA_32_FXSR 0x1000000
#define IA_32_SSE 0x2000000
#define IA_32_SSE2 0x4000000
#define IA_32_SS 0x8000000
#define IA_32_HTT 0x10000000
#define IA_32_TM 0x20000000
#define IA_32_PBE 0x40000000
#define CPU_REQUIRED (IA_32_X87 | IA_32_PSE | IA_32_APIC)

typedef struct {

  unsigned int flags;
  unsigned int trace_cache;
  unsigned int l1_icache;
  unsigned int l1_dcache;
  unsigned int l2_icache;
  unsigned int l2_dcache;
  unsigned int l3_icache;
  unsigned int l3_dcache;
  unsigned int l1_icache_assoc;
  unsigned int l1_dcache_assoc;
  unsigned int l2_icache_assoc;
  unsigned int l2_dcache_assoc;
  unsigned int l3_icache_assoc;
  unsigned int l3_dcache_assoc;
  unsigned int l1_icache_line;
  unsigned int l1_dcache_line;
  unsigned int l2_icache_line;
  unsigned int l2_dcache_line;
  unsigned int l3_icache_line;
  unsigned int l3_dcache_line;
  unsigned int itlb_norm;
  unsigned int itlb_super;
  unsigned int dtlb_norm;
  unsigned int dtlb_super;
  unsigned int itlb_norm_assoc;
  unsigned int itlb_super_assoc;
  unsigned int dtlb_norm_assoc;
  unsigned int dtlb_super_assoc;

} cache_tlb_t;

extern u_64 idt[256];
extern u_64* gdt;
extern void* tss_array;
extern unichar cpu_vendor_str[13];
extern unsigned int cpu_model;
extern unsigned int cpu_family;
extern unsigned int cpu_stepping;
extern unsigned int cpu_features;
extern unsigned int cpu_logical;
extern cache_tlb_t cache_tlb_data;

void idt_init(void);
void cpuid_parse(void);
void cpuid_store(unsigned int, void*);

#endif

#ifndef CPU_H
#define CPU_H

/* Copyright (c) 2004 Eric McCorkle.  All rights reserved */

#ifdef MULTITHREAD

#include <types.h>

#define IPI_BASE 0x30
#define IPI_STOP (IPI_BASE + 0)
#define IPI_RESCHED (IPI_BASE + 1)
#define IPI_RESTART (IPI_BASE + 2)
#define IPI_RESTART_TLBINV (IPI_BASE + 3)
#define IPI_RESTART_KERN_TLBINV (IPI_BASE + 4)
#define IPI_HALT (IPI_BASE + 5)
#define IPI_ALL 0xffffffff
#define IPI_ALL_OTHER 0xfffffffe

typedef struct {

  unsigned int id;
  unsigned int page_faults;
  addr_spc_t* addr_spc;
  void* phys_memcpy_pages;

} cpu_t;

cpu_t* cpus;

void cpu_halt(void);
unsigned int current_cpu(void);
void ipi_send(unsigned int ivect, unsigned int cpu);

/* ipi interrupt functions */
void ipi_stop(void);
void ipi_resched(void);
void ipi_restart(void);
void ipi_restart_tlbinv(void);
void ipi_restart_kern_tlbinv(void);
void ipi_halt(void);

#endif

#endif

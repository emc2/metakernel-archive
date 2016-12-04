.include "Makefile.def"
.include "Makefile.$(COMPILER)"

ASM=nasm
ASMFLAGS=-f elf
ASMOBJ=$(ASM) $(ASMFLAGS)
MKDEP=mkdep

.c.o:;

	$(CCOBJ) $<

.s.o:;

	$(ASMOBJ) -o $@ $<

kernel: $(OBJS);

	$(CCEXE) -o kernel $(OBJS)
	brandelf -f 255 kernel

depend:;

	cat asm.depend | ./depend.sh $(KERNEL_ARCH) > .depend
	$(MKDEP) -a -nostdinc -I ../src/sched/ -I- -I ../include/ $(DFLAGS) ../src/sched/*.c
	$(MKDEP) -a -nostdinc -I ../src/mem/malloc/ -I- -I ../include/ $(DFLAGS) ../src/mem/malloc/*.c
	$(MKDEP) -a -nostdinc -I ../src/mem/virt_mem/ -I- -I ../include/ $(DFLAGS) ../src/mem/virt_mem/*.c
	$(MKDEP) -a -nostdinc -I ../src/sync/ -I- -I ../include/ $(DFLAGS) ../src/sync/*.c
	$(MKDEP) -a -nostdinc -I ../src/mem/phys_mem/ -I- -I ../include/ $(DFLAGS) ../src/mem/phys_mem/*.c
	$(MKDEP) -a -nostdinc -I ../src/io/ -I- -I ../include $(DFLAGS) ../src/io/*.c
	$(MKDEP) -a -nostdinc -I ../src/sys/ -I- -I ../include $(DFLAGS) ../src/sys/*.c
	$(MKDEP) -a -nostdinc -I ../src/sys -I- -I ../include $(DFLAGS) ../src/sys/util.c
	$(MKDEP) -a -nostdinc -I ../arch/sys/ -I- -I ../include -I ../arch/include $(DFLAGS) ../arch/sys/*.c
	$(MKDEP) -a -nostdinc -I ../arch/mem/virt_mem/ -I- -I ../include -I ../arch/include $(DFLAGS) ../arch/mem/virt_mem/*.c

clean:;

	rm -f *.o *.il report.opt kernel

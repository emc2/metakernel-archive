#include <definitions.h>
#include <panic.h>
#include <cpu.h>

void kernel_panic(unichar* s) {

  cpu_halt();

}

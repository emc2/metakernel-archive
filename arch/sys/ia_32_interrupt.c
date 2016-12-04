#include <definitions.h>
#include <ia_32_cputabs.h>
#include <interrupt.h>
#include <util.h>

void int_set_handler(unsigned int inum, void (*handler)(void)) {

  u_64 h_addr = PTR_TO_UINT(handler) & 0xffffffff;

  h_addr = (h_addr | (h_addr << 32)) & 0xffff00000000ffff;
  idt[inum] = 0x00008e0000080000 | h_addr;

}


int_handler_t int_get_handler(unsigned int inum) {

  unsigned int out;

  out = idt[inum] & 0x0000ffff;
  out |= (idt[inum] >> 32) & 0xffff0000;

  return (int_handler_t)UINT_TO_PTR(out);

}

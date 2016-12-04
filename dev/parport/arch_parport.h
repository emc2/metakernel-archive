#ifndef ARCH_PARPORT_H
#define ARCH_PARPORT_H

#include <definitions.h>

int parport_write_char(void* addr, unichar c);

#endif

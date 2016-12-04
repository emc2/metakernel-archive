#ifndef ROUND_H
#define ROUND_H

/* Copyright (c) 2004 Eric McCorkle.  All rights reserved. */
/* round to power of 2 */
unsigned int round_down(unsigned int);
unsigned int round_up(unsigned int);

/* determine which list to use */
unsigned int addr_power(unsigned int);
unsigned int addr_size(void*);

/* integer log base 2 */
unsigned int ilog(unsigned int);

#endif

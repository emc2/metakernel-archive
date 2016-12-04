#ifndef INTERRUPT_H
#define INTERRUPT_H

/* Copyright (c) 2004 Eric McCorkle.  All rights reserved. */

typedef void (*int_handler_t)(void);

void int_set_handler(unsigned int, void (*)(void));
int_handler_t int_get_handler(unsigned int);
void int_clear(unsigned int);
void int_restore(unsigned int);

/* some handlers */
void int_fatal(void);
void int_ignore(void);

#endif

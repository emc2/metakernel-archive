#ifndef PARPORT_H
#define PARPORT_H

#include <definitions.h>
#include <inode.h>
/*
#define PARPORT_FLUSH 1
#define PARPORT_CLEAR 2
#define PARPORT_GET_MODE 3
#define PARPORT_SET_MODE 4
#define PARPORT_GET_BUFSIZE 5
#define PARPORT_SET_BUFSIZE 6
*/
inode_t* parport_create(void* port);

#endif

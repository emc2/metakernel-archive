#include <definitions.h>
#include <parport.h>
#include <inode.h>
#include <inode_table.h>
#include <sync.h>
#include "arch_parport.h"

typedef struct {

  unsigned int buf_size;
  unsigned int buf_pos;
  void* port_addr;
  unichar* buf;

} parport_data_t;

static int parport_seq_write(inode_t*, unsigned int, void*,
			     soffset_t, unsigned int);
/*
static int parport_spec_op(inode_t*, unsigned int,
			   unsigned int, ...);
*/
static void parport_free(inode_t*);

static const inode_func_set_t parport_funcs = {
  NULL, NULL, NULL, NULL, NULL, parport_seq_write,
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  NULL, NULL, /*parport_spec_op*/ NULL, parport_free
};


inode_t* parport_create(void* port) {

  inode_t* out;
  parport_data_t* data;

  out = malloc(sizeof(inode_t) + sizeof(parport_data_t));
  out->stat.id = inode_id();
  out->stat.flags = 0;
  out->stat.size = 0;
  out->stat.owner = 0;
  out->stat.group = 0;
  out->stat.block_size = 0;
  out->stat.filesys = 0;
  out->stat.pack_count = 0;
  out->stat.hard_ref_count = 0;
  out->stat.ref_count = 0;
  mutex_init(out->mutex);
  cond_init(out->cond);
  out->hash_next = NULL;
  out->funcs = &parport_funcs;
  data = (parport_data_t*)out->metadata;
  data->buf_size = 2 * PAGE_SIZE;
  data->buf = kern_malloc(&(data->buf_size), 0);
  data->port_addr = port;
  data->buf_pos = 0;

  return out;

}


static inline void parport_flush(inode_t* inode) {

  int i;
  parport_data_t* data;

  data = (parport_data_t*)out->metadata;

  for(i = 0; i < data->buf_pos; i++)
    parport_write_char(data->port_addr, data->buf[i]);

  data->buf_pos = 0;

}


int parport_seq_write(inode_t* inode, unsigned int size,
		      void* buf, soffset_t seek, unsigned int mode) {

  unichar* charbuf;
  parport_data_t* data;

  if(0 == seek && (mode & MODE_POS) == MODE_CURR) {

    mutex_lock(&(inode->mutex));
    data = (parport_data_t*)out->metadata;

    if(data->buf_pos + (size / sizeof(unichar)) <= data->buf_size) {

      memcpy(data->buf, buf, size);
      data->buf_pos += size / sizeof(unichar);

    }

    else {

      parport_flush(inode);
      charbuf = buf;

      for(i = 0; i < (size / sizeof(unichar)); i++)
	parport_write_char(data->port_addr, charbuf[i]);

    }

    mutex_unlock(&(inode->mutex));

    return size;

  }

}


void parport_free(inode_t* inode) {

  parport_data_t* data;

  data = (parport_data_t*)out->metadata;
  kern_free(data->buf, data->buf_size);
  free(data);

}

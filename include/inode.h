#ifndef INODE_H
#define INODE_H

/* Copyright (c) 2004 Eric McCorkle.  All rights reserved. */

#include <definitions.h>
#include <types.h>
#include <sync.h>

#define INODE_INDEX_READ 0x1
#define INODE_INDEX_WRITE 0x2
#define INODE_SEQ_READ 0x4
#define INODE_SEQ_WRITE 0x8
#define INODE_PACK_READ 0x10
#define INODE_PACK_WRITE 0x20
#define INODE_DETERMINISTIC 0x40
#define INODE_SEEK_FWD 0x80
#define INODE_SEEK_REV 0x100
#define INODE_SET 0x200
#define INODE_STAT 0x400
#define INODE_SIZE 0x800
#define INODE_TYPE 0x7000
#define INODE_FUND 0x0000
#define INODE_INDEX 0x1000
#define INODE_SEQ 0x2000
#define INODE_PACK 0x3000
#define INODE_SLINK 0x4000
#define INODE_DIR 0x5000
#define INODE_MOUNTED 0x8000

/* Obviously, some of these combinations are
 * bogus.
 */

/* Virtual directory entries (vdirs) are used
 * to mount filesystems, create unbacked files,
 * pipes, etc.
 */

typedef struct {

  unsigned int id;
  unsigned int flags;
  u_64 size;
  u_64 owner;
  u_64 group;
  u_64 block_size;
  unsigned int filesys;
  unsigned int pack_count;
  unsigned int hard_ref_count;
  unsigned int ref_count;
  /* XXX Need to decide on ACL format */

} inode_stat_t;

typedef struct {

  int (*page_read)(inode_t*, offset_t, phys_addr_t);
  int (*page_write)(inode_t*, offset_t, phys_addr_t);
  int (*index_read)(inode_t*, offset_t, offset_t,
		    void*, unsigned int);
  int (*index_write)(inode_t*, offset_t, offset_t,
		     const void*, unsigned int);
  int (*seq_read)(inode_t*, offset_t, void*, unsigned int);
  int (*seq_write)(inode_t*, offset_t,
		   const void*, unsigned int);
  int (*pack_read)(inode_t*, offset_t, void*, unsigned int);
  int (*pack_write)(inode_t*, offset_t,
		    const void*, unsigned int);
  int (*stat)(inode_t*, inode_stat_t*);
  inode_t* (*dir_lookup)(inode_t*, const unichar*);
  int (*dir_link)(inode_t*, const unichar*, inode_t*);
  int (*dir_vlink)(inode_t*, const unichar*, inode_t*);
  int (*dir_unlink)(inode_t*, const unichar*);
  int (*dir_list)(inode_t*, unichar*);
  int (*dir_count)(inode_t*);
  int (*slink_read)(inode_t*, unichar*);
  int (*slink_write)(inode_t*, const unichar*);
  int (*spec_op)(inode_t*, unsigned int,
                 unsigned int, ...);
  void (*free)(inode_t*);

} inode_func_set_t;

/* The unglorious inode struct.  An
 * amalgamation of other structs...
 */

struct inode_t {

  /* OS metadata */
  mutex_t mutex;
  cond_t cond;
  struct inode_t* hash_next;
  inode_stat_t stat;
  inode_func_set_t* funcs;
  char metadata[];

};

/* The physical memory system needs these
 * to be synchronous.  The rest of the world
 * needs asynchronous.
 */
#define MODE_SYNC 0x1
#define MODE_PEEK 0x2
#define MODE_POS 0xc
#define MODE_CURR 0x0
#define MODE_START 0x4
#define MODE_END 0x8

/* I REALLY don't like the direction this is going.
 * Way too much complication, way too much
 * specialization, but I can't seem to see the grand
 * unifying picture of IO semantics...
 */

/* Random thoughts as of 1/1/2004...  There seem to be two things at
 * work here: files and streams.  Files are indexed always,
 * deterministic always, and can be mapped.  Streams don't support
 * reverse seeks, and are either sequential or packet based.
 * Sequential represents continuity, packets represent discreteness.
 * But what about fixed-size packet streams?  Should they get special
 * treatment?
 *
 * Note: Anything that supports reverse seeks, and is deterministic
 * can be indexed...  And should be.
 *
 * Inodes glom files and streams together.  Should I do this?  Is
 * there some underlying universal IO model I'm missing?
 *
 * The particular read operation used must correspond to the inode's
 * type.  Though it is possible to implement some kinds of IO with
 * others, this kind of grimy multiplexing is exiled forthwith to
 * userland libraries.
 */

/* Index reads and writes work on block-granular units these files are
 * stateless.
 *
 * Sequential reads and writes position the stream, then perform an
 * operation (advancing the stream unless MODE_PEEK is set).  Either
 * operation can be omitted.
 *
 * Packet reads and writes pull in the next packet.  They consume the
 * whole packet, even if the whole packet isn't read, unless MODE_PEEK
 * is used.  Writes send a single packet.
 */

inode_t* inode_create(const inode_stat_t* stat);
int inode_close(inode_t* inode);
int inode_get(inode_t* inode, inode_stat_t* stat);
int inode_set(inode_t* inode, const inode_stat_t* stat);
int inode_page_read(inode_t* inode, offset_t offset, phys_addr_t);
int inode_page_write(inode_t* inode, offset_t offset, phys_addr_t);
int inode_index_read(inode_t* inode, offset_t offset, offset_t size,
		     void* ptr, unsigned int mode);
int inode_index_write(inode_t* inode, offset_t offset, offset_t size,
		      const void* ptr, unsigned int mode);
int inode_seq_read(inode_t* inode, offset_t size,
		   void* ptr, unsigned int mode);
int inode_seq_write(inode_t* inode, offset_t size, 
		    const void* ptr, unsigned int mode);
int inode_pack_read(inode_t* inode, offset_t size,
		    void* ptr, unsigned int mode);
int inode_pack_write(inode_t*, offset_t size,
		     const void* ptr, unsigned int mode);
int inode_stat(inode_t*, inode_stat_t* stat);
/* directories get their own inode calls */
inode_t* inode_dir_lookup(inode_t* dir, const unichar* name);
int inode_dir_link(inode_t* dir, const unichar* name,
		   inode_t* inode);
int inode_dir_vlink(inode_t* dir, const unichar* name,
		    inode_t* inode);
int inode_dir_unlink(inode_t* dir, const unichar* name);
int inode_dir_list(inode_t* dir, unichar* buf);
int inode_dir_count(inode_t* dir);
/* slinks only are allowed two calls */
int inode_slink_read(inode_t* inode, unichar* buf);
int inode_slink_write(inode_t* inode, const unichar* buf);
/* wildcard special operation (ioctl on POSIX) */
int inode_spec_op(inode_t* inode, unsigned int op,
		  unsigned int mode, ...);

#endif

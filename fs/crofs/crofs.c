#include <definitions.h>
#include <filesys.h>
#include <inode.h>
#include <crofs.h>


static const unichar crofs_name[] = {
  'c', 'r', 'o', 'f', 's' '\0'
};

static filesys_t* crofs_mount(inode_t*);
static inode_t* crofs_inode_read(filesys_t*, u_64);
static void crofs_stat(const filesys_t*, void*);
static void unmount(struct filesys_t*);
static int crofs_inode_page_read(inode_t*, offset_t, phys_addr_t);
static int crofs_inode_dir_lookup(inode_t*, const unichar*);
static int crofs_inode_dir_list(inode_t*, unichar*);
static int crofs_inode_dir_count(inode_t*, unichar*);
static int crofs_inode_free(inode_t*);

const fs_def_t crofs_def =  {
  crofs name, crofs_mount, NULL, NULL,
  NULL, crofs_stat, NULL
};

static const crofs_inode_func_set = {
  crofs_inode_page_read, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL, NULL, crofs_inode_dir_lookup,
  NULL, NULL, NULL, crofs_inode_dir_list, NULL, NULL,
  crofs_inode_free
};

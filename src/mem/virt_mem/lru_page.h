#ifndef LRU_PAGE_H
#define LRU_PAGE_H

#include <definitions.h>

phys_addr_t lru_page(void);
void lru_age_pages(void);

#endif

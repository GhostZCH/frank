
#ifndef _FRK_SLAB_
#define _FRK_SLAB_

#include <stdint.h>


typedef struct frk_slab_s frk_slab_t;

frk_slab_t* frk_new_slab(void *pool, int64_t size);
void* frk_slab_malloc(frk_slab_t *s, int64_t size, int reset);
void frk_slab_free(frk_slab_t *s, void *ptr);

#endif

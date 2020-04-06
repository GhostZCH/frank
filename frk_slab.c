#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <frk_slab.h>

const int64_t magic = 1111575635; // "SLAB"
const int64_t page_size = 4096;
const int64_t typeconf[][3] = {
    // {itemsize, fullmask, itemcount=page_size/item_size}
    {64, 0xFFFFFFFFFFFFFFFF, 64},
    {128, 0xFFFFFFFF, 32},
    {256, 0xFFFF, 16},
    {512, 0xFF, 8},
    {1024, 0xF, 4},
    {2048, 0x3, 2},
    {4096, 0x1, 1},
};
const int64_t types = sizeof(typeconf) / sizeof(int64_t[3]);


typedef struct
{
    int64_t type;
    int64_t used;
    void *items;
}frk_slab_block_t;


struct frk_slab_s
{
    int64_t magic;
    int64_t blockcount;
    frk_slab_block_t* empty;
    frk_slab_block_t* free[sizeof(typeconf) / sizeof(int64_t[3])];
    frk_slab_block_t* end;
    void *pages;
    frk_slab_block_t blocks[];
};


frk_slab_t*
frk_new_slab(void *pool, int64_t size) {
    int64_t i;
    frk_slab_t* s = pool;

    s->empty = &s->blocks[0];
    s->blockcount = (size - sizeof(frk_slab_t)) / (page_size + sizeof(frk_slab_block_t));
    s->pages = pool + sizeof(frk_slab_t) + sizeof(frk_slab_block_t) * s->blockcount;
    s->end = s->blocks + s->blockcount;

    for (i = 0; i < s->blockcount; i++) {
        s->blocks[i].type = -1;
        s->blocks[i].items = s->pages + i * page_size;
    }

    for (i = 0; i < types; i++) {
        s->free[i] = s->end;
    }

    s->magic = magic;
    return s;
}


frk_slab_block_t*
frk_slab_add_block(frk_slab_t *s, int64_t type) {
    if (s->empty == s->end) {
        return NULL;
    }

    frk_slab_block_t* b = s->empty;
    b->type = type;
    memset(b->items, 0, page_size);

    s->free[type] = b;

    for (s->empty += 1; s->empty != s->end && s->empty->type != -1; s->empty++);

    return b;
}


void
frk_update_free(frk_slab_t* s, int64_t type) {
    int64_t mask = typeconf[type][1];
    frk_slab_block_t *i = s->free[type] + 1;
    for (; i != s->end; i++) {
        if (i->type == type && i->used != mask) {
            break;
        }
    }
    s->free[type] = i;
}


void
frk_slab_del_block(frk_slab_t *s, frk_slab_block_t *b) {
    if (b == s->free[b->type]) {
        frk_update_free(s, b->type);
    }

    b->type = -1;
    if (b < s->empty) {
        s->empty = b;
    }
}


void*
frk_slab_malloc(frk_slab_t *s, int64_t size, int reset) {
    if (s == NULL || s->magic != magic || size > typeconf[types-1][0]) {
        return NULL;
    }

    int64_t type = 0;
    for (;size > typeconf[type][0]; type++);

    frk_slab_block_t *b = s->free[type];

    if (b == s->end){
        b = frk_slab_add_block(s, type);
        if (b == NULL) {
            return NULL;
        }
    }

    int64_t i, n = typeconf[type][2];
    for (i = 0; i < n && b->used & (0x1 << i); i++);

    b->used |= (0x1 << i);
    if (b->used == typeconf[type][1]) {
        frk_update_free(s, type);
    }

    void *ptr = b->items + i * typeconf[type][0];
    if (reset) {
        memset(ptr, 0, typeconf[type][0]);
    }
    return ptr;
}


void
frk_slab_free(frk_slab_t *s, void *ptr) {
    if (s == NULL || s->magic != magic || ptr == NULL) {
        return;
    }

    int64_t page = (ptr - s->pages) / page_size;
    if (page < 0 || page >= s->blockcount) {
        return;
    }

    frk_slab_block_t* b = &s->blocks[page];

    int64_t idx = (ptr - s->pages) % page_size / typeconf[b->type][0];
    b->used &= ~(0x1 << idx);

    if (b->used == 0) {
        return frk_slab_del_block(s, b);
    }

    if (b < s->free[b->type]) {
        s->free[b->type] = b;
    }
}

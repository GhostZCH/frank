
#ifndef _FRK_DICT_
#define _FRK_DICT_

#include <stdint.h>


typedef void* (*frk_malloc_fp)(int64_t size, void* data);
typedef void (*frk_free_fp)(void* ptr, void *data);
typedef struct frk_dict_s frk_dict_t;
typedef struct frk_str_s frk_str_t;
typedef struct frk_iter_s frk_iter_t;
typedef struct frk_node_s frk_node_t;


typedef enum {
    FRK_NIL = 0,
    FRK_INT = 1,
    FRK_STR = 2,
    FRK_DICT = 3
} frk_type_e;


struct frk_str_s
{
    int64_t len;
    char data[];
};


struct frk_dict_s
{
    int64_t count;
    int64_t bucket_count;
    frk_malloc_fp malloc;
    frk_free_fp free;
    void* data;
    frk_node_t** buckets;
};


struct frk_node_s
{
    frk_node_t **pre;
    frk_node_t *next;
    frk_str_t *key;
    int64_t hash;
    frk_type_e type;
    union
    {
        int64_t i;
        frk_str_t* s;
        frk_dict_t* d;
    };
};


struct frk_iter_s
{
    int64_t bucket;
    frk_node_t* node;
};


frk_dict_t* frk_new_dict(frk_malloc_fp m, frk_free_fp f, void* data);

int64_t* frk_insert_int(frk_dict_t *d, char *key, int64_t key_len, int64_t val);
frk_str_t* frk_insert_string(frk_dict_t *d, char *key, int64_t key_len, char *val, int64_t val_len);
frk_dict_t* frk_insert_dict(frk_dict_t *d, char *key, int64_t key_len);

frk_node_t* frk_get_node(frk_dict_t *d, char *key, int64_t key_len);
frk_dict_t* frk_get_dict(frk_dict_t *d, char *key, int64_t key_len);
int64_t* frk_get_int(frk_dict_t *d, char *key, int64_t key_len);
frk_str_t* frk_get_str(frk_dict_t *d, char *key, int64_t key_len);

void frk_remove(frk_dict_t *d, char *key, int64_t key_len);
void frk_remove_node(frk_dict_t *d, frk_node_t* node);

frk_iter_t* frk_iterator(frk_dict_t *d, frk_iter_t *last, frk_iter_t *next);
int64_t frk_dict_dump(frk_dict_t *d, char *buffer, int64_t len);

#endif

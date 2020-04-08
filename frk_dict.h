
#ifndef _FRK_DICT_
#define _FRK_DICT_

#include <stdint.h>


typedef void* (*frk_malloc_fp)(int64_t size, void* data);
typedef void (*frk_free_fp)(void* ptr, void *data);
typedef char* (*frk_to_string_fp)(void *data, char* buf, int64_t len);

typedef struct frk_dict_s frk_dict_t;
typedef struct frk_str_s frk_str_t;
typedef struct frk_dict_iter_s frk_dict_iter_t;
typedef struct frk_node_s frk_node_t;
typedef struct frk_obj_s frk_obj_t;

typedef enum {
    FRK_NIL = 0,
    FRK_NUM,
    FRK_STR,
    FRK_DICT,
    FRK_OBJ,
} frk_type_e;


struct frk_str_s
{
    int64_t len;
    char data[];
};


struct frk_obj_s
{
    frk_to_string_fp string;
    frk_malloc_fp malloc;
    frk_free_fp free;
    void* malloc_data;
    void* data;
};


struct frk_dict_s
{
    int64_t count;
    int64_t bucket_count;
    frk_malloc_fp malloc;
    frk_free_fp free;
    frk_obj_t* data;
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
        double n;
        frk_str_t* s;
        frk_dict_t* d;
        void* o;
    };
};


struct frk_dict_iter_s
{
    int64_t bucket;
    frk_node_t* node;
};


frk_dict_t* frk_new_dict(frk_malloc_fp m, frk_free_fp f, void* data);

double* frk_insert_num(frk_dict_t *d, char *key, int64_t key_len, double val);
frk_str_t* frk_insert_string(frk_dict_t *d, char *key, int64_t key_len, char *val, int64_t val_len);
frk_dict_t* frk_insert_dict(frk_dict_t *d, char *key, int64_t key_len);

frk_node_t* frk_get_node(frk_dict_t *d, char *key, int64_t key_len);
frk_dict_t* frk_get_dict(frk_dict_t *d, char *key, int64_t key_len);
double* frk_get_num(frk_dict_t *d, char *key, int64_t key_len);
frk_str_t* frk_get_str(frk_dict_t *d, char *key, int64_t key_len);

void frk_remove(frk_dict_t *d, char *key, int64_t key_len);
void frk_remove_node(frk_dict_t *d, frk_node_t* node);

void frk_dict_clear(frk_dict_t *d);

frk_dict_iter_t* frk_dict_iter(frk_dict_t *d, frk_dict_iter_t *last, frk_dict_iter_t *next);

int64_t frk_dict_dump(frk_dict_t *d, char *buffer, int64_t len);
frk_dict_t* frk_dict_load(frk_dict_t* d, char* json, char **end);

#endif

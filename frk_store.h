
#ifndef _FRK_STORE_
#define _FRK_STORE_

#include <stdint.h>

typedef enum {
    FRK_NIL = 0,
    FRK_NUM,
    FRK_STR,
    FRK_DICT,
    FRK_LIST,
} frk_type_e;


typedef struct frk_store_s frk_store_t;
typedef struct frk_dict_s frk_dict_t;
typedef struct frk_list_s frk_list_t;
typedef struct frk_str_s frk_str_t;
typedef struct frk_dict_iter_s frk_dict_iter_t;
typedef struct frk_dict_node_s frk_dict_node_t;
typedef struct frk_item_s frk_item_t;
typedef frk_item_t* frk_list_iter_t;
typedef void* (*frk_calloc_cb)(int64_t size, void* data);
typedef void (*frk_free_cb)(void* ptr, void *data);


struct frk_store_s
{
    frk_calloc_cb calloc;
    frk_free_cb free;
    void* data;
};


struct frk_str_s
{
    int64_t len;
    char data[];
};


struct frk_dict_s
{
    int64_t count;
    int64_t bucket_count;
    frk_dict_node_t** buckets;
    frk_store_t *store;
};


struct frk_dict_node_s
{
    int64_t hash;
    frk_dict_node_t **pre;
    frk_dict_node_t *next;
    frk_item_t* item;
    int64_t klen;
    char key[];
};


struct frk_list_s
{
    int64_t count;
    int64_t capacity;
    frk_item_t** items;
    frk_store_t *store;
};


struct frk_item_s
{
    frk_type_e type;
    union
    {
        double n;
        frk_str_t* s;
        frk_dict_t* d;
        frk_list_t* l;
        void* user;
    };
};


struct frk_dict_iter_s
{
    int64_t bucket;
    frk_dict_node_t* node;
    frk_item_t *item;
};


// new
frk_store_t* frk_new_store(frk_calloc_cb m, frk_free_cb f, void* data);
frk_item_t* frk_root(frk_store_t *s);

// dict
frk_item_t* frk_dict_get(frk_dict_t *d, char *key, int64_t klen);
double* frk_dict_get_num(frk_dict_t *d, char *key, int64_t klen);
frk_str_t* frk_dict_get_str(frk_dict_t *d, char *key, int64_t klen);
frk_list_t* frk_dict_get_list(frk_dict_t *d, char *key, int64_t klen);
frk_dict_t* frk_dict_get_dict(frk_dict_t *d, char *key, int64_t klen);

double* frk_dict_set_num(frk_dict_t *d, char *key, int64_t klen, double num);
frk_str_t* frk_dict_set_str(frk_dict_t *d, char *key, int64_t klen, char *val, int64_t vlen);
frk_list_t* frk_dict_set_list(frk_dict_t *d, char *key, int64_t klen);
frk_dict_t* frk_dict_set_dict(frk_dict_t *d, char *key, int64_t klen);

frk_dict_iter_t* frk_dict_iter(frk_dict_t *d, frk_dict_iter_t *last, frk_dict_iter_t *next);
void frk_dict_del(frk_dict_t *d, char*key, int64_t klen);
void frk_dict_clear(frk_dict_t *d);

// list
frk_item_t* frk_list_get(frk_list_t *l, int64_t index);
double* frk_list_get_num(frk_list_t *l, int64_t index);
frk_str_t* frk_list_get_str(frk_list_t *l,int64_t index);
frk_list_t* frk_list_get_list(frk_list_t *l, int64_t index);
frk_dict_t* frk_list_get_dict(frk_list_t *l, int64_t index);

double* frk_list_set_num(frk_list_t *l, int64_t index, double num);
frk_str_t* frk_list_set_str(frk_list_t *l, int64_t index, char* val, int64_t vlen);
frk_list_t* frk_list_set_list(frk_list_t *l, int64_t index);
frk_dict_t* frk_list_set_dict(frk_list_t *l, int64_t index);

double* frk_list_append_num(frk_list_t *l, double num);
frk_str_t* frk_list_append_str(frk_list_t *l, char *val, int64_t vlen);
frk_list_t* frk_list_append_list(frk_list_t *l);
frk_dict_t* frk_list_append_dict(frk_list_t *l);

frk_list_iter_t*frk_list_iter(frk_list_t *l, frk_list_iter_t *last);
void frk_list_del(frk_list_t *l, int64_t index);
void frk_list_clear(frk_list_t *l);

int64_t frk_dump_item(frk_item_t *item, char* buf, int64_t len, char end);
frk_item_t* frk_load_item(frk_store_t *store, char* json, char **end);

#endif

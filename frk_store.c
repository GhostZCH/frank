#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>
#include <frk_store.h>

const int64_t frk_dict_init_size = 8;
const int64_t frk_dict_load_rate = 4;
const int64_t frk_dict_incr_rate = 4;
const int64_t frk_list_init_size = 8;
const int64_t frk_list_incr_rate = 2;


int64_t
frk_hash(char *str, int64_t len)
{
    int64_t i, hash = 0;
    for (i = 0; i < len; i++) {
        hash = hash * 31 + str[i];
    }
    return hash;
}


frk_store_t*
frk_new_store(frk_calloc_cb m, frk_free_cb f, void* data)
{
    frk_store_t* s = m(sizeof(frk_store_t), data);
    if (s == NULL) {
        return NULL;
    }
    s->calloc = m;
    s->free = f;
    s->data = data;
    return s;
}


frk_str_t*
frk_new_str(frk_store_t *s, char *src, int64_t len)
{
    frk_str_t* str = s->calloc(sizeof(frk_str_t)+len, s->data);
    if (str == NULL) {
        return NULL;
    }
    str->len = len;
    memcpy(str->data, src, len);
    return str;
}


frk_dict_t*
frk_new_dict(frk_store_t *s)
{
    frk_dict_t *d = s->calloc(sizeof(frk_dict_t), s->data);
    frk_dict_node_t** buckets = s->calloc(sizeof(frk_dict_t *) * frk_dict_init_size, s->data);
 
    if (d == NULL || buckets == NULL) {
        s->free(d, s->data);
        s->free(buckets, s->data);
        return NULL;
    }

    d->store = s;
    d->count = 0;
    d->bucket_count = frk_dict_init_size;
    d->buckets = buckets;

    return d;
}


frk_list_t*
frk_new_list(frk_store_t *s)
{
    frk_list_t *l = s->calloc(sizeof(frk_list_t), s->data);
    frk_item_t **items = s->calloc(sizeof(frk_item_t *) * frk_dict_init_size, s->data);

    if (l == NULL || items == NULL) {
        s->free(l, s->data);
        s->free(items, s->data);
        return NULL;
    }

    l->store = s;
    l->count = 0;
    l->items = items;
    l->capacity = frk_list_init_size;

    return l;
}


frk_item_t*
frk_new_item(frk_store_t *s, frk_type_e t, void *val, int64_t len)
{
    frk_item_t* item = s->calloc(sizeof(frk_item_t), s->data);
 
    if (item == NULL) {
        return NULL;
    }

    switch (t)
    {
    case FRK_NUM:
        item->n = *(double *)val;
        break;

    case FRK_STR:
        item->s = frk_new_str(s, val, len);
        break;

    case FRK_LIST:
        item->l = frk_new_list(s); 
        break;

    case FRK_DICT:
        item->d = frk_new_dict(s); 
        break;

    default:
        break;
    }

    if (t != FRK_NUM && item->user == NULL){
        s->free(item, s->data);
        return NULL;
    }

    item->type = t;
    return item;
}


frk_item_t*
frk_root(frk_store_t *s)
{
    return frk_new_item(s, FRK_DICT, NULL, 0);
}


void*
frk_item_data(frk_item_t *item, frk_type_e t)
{
    if (item == NULL || item->type != t) {
        return NULL;
    }
    return item->type == FRK_NUM ? &item->d : item->user;
}


void
frk_item_free(frk_store_t *s, frk_item_t *item)
{
    if (s == NULL || item == NULL) {
        return;
    }

    switch (item->type)
    {
    case FRK_STR:
        s->free(item->s, s->data);
        break;

    case FRK_DICT:
        frk_dict_clear(item->d);
        s->free(item->d->buckets, s->data);
        s->free(item->d, s->data);
        break;

    case FRK_LIST:
        frk_list_clear(item->l);
        s->free(item->l->items, s->data);
        s->free(item->l, s->data);
        break;
    }

    s->free(item, s->data);
}


frk_dict_node_t*
frk_dict_get_node(frk_dict_t *d, char *key, int64_t klen)
{
    int64_t h = frk_hash(key, klen);
    int64_t b = h % d->bucket_count;
 
    frk_dict_node_t* i = d->buckets[b];
    for (;i != NULL; i = i->next) {
        if (i->hash == h) {
            return i;
        }
    }

    return NULL;
}


void
frk_dict_bucket_add_node(frk_dict_node_t* node, frk_dict_node_t** buckets, int64_t count)
{
    int64_t b = node->hash % count;
    node->next = buckets[b];
    node->pre = &(buckets[b]);
    if (buckets[b] != NULL) {
        buckets[b]->pre = &node->next;
    }
    buckets[b] = node;
}


frk_dict_t*
frk_dict_resize(frk_dict_t *d)
{
    int64_t count =  d->bucket_count * frk_dict_incr_rate;
    frk_store_t *store = d->store;
    frk_dict_node_t **buckets = store->calloc(sizeof(frk_dict_node_t *) * count, store->data);
    if (buckets == NULL) {
        return NULL;
    }

    int64_t n, b = 0;
    frk_dict_node_t *i, *next;
    for (; b < d->bucket_count; b++) {
        for (i = d->buckets[b]; i != NULL; i = next) {
            next = i->next;
            frk_dict_bucket_add_node(i, buckets, count);
        }
    }

    d->bucket_count = count;
    d->buckets = buckets;

    return d;
}


frk_item_t*
frk_dict_get(frk_dict_t *d, char *key, int64_t klen)
{
    frk_dict_node_t *node = frk_dict_get_node(d, key, klen);
    return node != NULL ? node->item: NULL;
}


double*
frk_dict_get_num(frk_dict_t *d, char *key, int64_t klen)
{
    return frk_item_data(frk_dict_get(d, key, klen), FRK_NUM);
}


frk_str_t*
frk_dict_get_str(frk_dict_t *d, char *key, int64_t klen)
{
    return frk_item_data(frk_dict_get(d, key, klen), FRK_STR);
}


frk_list_t*
frk_dict_get_list(frk_dict_t *d, char *key, int64_t klen)
{
    return frk_item_data(frk_dict_get(d, key, klen), FRK_LIST);
}


frk_dict_t*
frk_dict_get_dict(frk_dict_t *d, char *key, int64_t klen)
{
    return frk_item_data(frk_dict_get(d, key, klen), FRK_DICT);
}


frk_item_t*
frk_dict_set(frk_dict_t *d, char *key, int64_t klen, frk_type_e t, void* val, int64_t vlen)
{
    frk_store_t* s = d->store;
    frk_item_t * i = frk_new_item(s, t, val, vlen);
    frk_dict_node_t* n = s->calloc(sizeof(frk_dict_node_t) + klen, s->data);

    if (n == NULL || i == NULL) {
        s->free(n, s->data);
        s->free(i, s->data);
        return NULL;
    }

    n->item = i;
    n->klen = klen;
    n->hash = frk_hash(key, klen);
    memcpy(n->key, key, klen);

    frk_dict_bucket_add_node(n, d->buckets, d->bucket_count);
    d->count++;

    if (d->count > frk_dict_load_rate * d->bucket_count) {
        frk_dict_resize(d);
    }

    return n->item;
}


double*
frk_dict_set_num(frk_dict_t *d, char *key, int64_t klen, double num)
{
    return frk_item_data(frk_dict_set(d, key, klen, FRK_NUM, &num, sizeof(double)), FRK_NUM);
}


frk_str_t*
frk_dict_set_str(frk_dict_t *d, char *key, int64_t klen, char *val, int64_t vlen)
{
    return frk_item_data(frk_dict_set(d, key, klen, FRK_STR, val, vlen), FRK_STR);
}


frk_list_t*
frk_dict_set_list(frk_dict_t *d, char *key, int64_t klen)
{
    return frk_item_data(frk_dict_set(d, key, klen, FRK_LIST, NULL, 0), FRK_LIST);  
}


frk_dict_t*
frk_dict_set_dict(frk_dict_t *d, char *key, int64_t klen)
{
    return frk_item_data(frk_dict_set(d, key, klen, FRK_DICT, NULL, 0), FRK_DICT);  
}


void
frk_dict_clear(frk_dict_t* d)
{
    frk_store_t *s = d->store;

    int64_t b = 0;
    for (b = 0; b < d->bucket_count; b++) {
        frk_dict_node_t *next, *iter = d->buckets[b];
        for (; next != NULL; iter = next) {
            next = iter->next;
            frk_item_free(s, iter->item);
            s->free(iter, s->data);
        }
    }

    frk_dict_node_t **buckets = s->calloc(sizeof(frk_dict_node_t *) * frk_dict_init_size, s->data);
    if (buckets == NULL) {
        return;
    }

    s->free(d->buckets, s->data);
    d->buckets = buckets;
    d->bucket_count = frk_dict_init_size;
}


void
frk_dict_del(frk_dict_t *d, char*key, int64_t klen)
{
    frk_dict_node_t *node = frk_dict_get_node(d, key, klen);
 
    *node->pre = node->next;
    if (node->next != NULL) {
        node->next->pre = node->pre;
    }
    d->count--;

    frk_item_free(d->store, node->item);    
    d->store->free(node, d->store->data);
}


frk_dict_iter_t*
frk_dict_iter(frk_dict_t *d, frk_dict_iter_t *last, frk_dict_iter_t *next)
{
    if (last == NULL) {
        next->bucket = -1;
        next->node = NULL;
    } else {
        next->bucket = last->bucket;
        next->node = last->node->next;
    }

    if (next->node == NULL) { 
        int64_t b = next->bucket + 1; // search next bucket
        for (; b < d->bucket_count;b++) {
            if (d->buckets[b] != NULL) {
                next->bucket = b;
                next->node = d->buckets[b];
                next->item = next->node->item;
                return next;
            }
        }
    }

    return NULL;
}


frk_item_t*
frk_list_get(frk_list_t *l, int64_t index)
{
    return index < l->count ? l->items[index]: NULL;
}


double*
frk_list_get_num(frk_list_t *l, int64_t index)
{
    return frk_item_data(frk_list_get(l, index), FRK_NUM);
}


frk_str_t*
frk_list_get_str(frk_list_t *l,int64_t index)
{
    return frk_item_data(frk_list_get(l, index), FRK_STR);
}


frk_list_t*
frk_list_get_list(frk_list_t *l, int64_t index)
{
    return frk_item_data(frk_list_get(l, index), FRK_LIST);
}


frk_dict_t*
frk_list_get_dict(frk_list_t *l, int64_t index)
{
    return frk_item_data(frk_list_get(l, index), FRK_DICT);
}


frk_list_t*
frk_list_resize(frk_list_t *l)
{
    frk_store_t *s = l->store;

    int64_t cap = l->capacity * frk_list_incr_rate;
    frk_item_t **items = s->calloc(sizeof(frk_item_t *) * cap, s->data);

    if (items == NULL) {
        return NULL;
    }

    memcpy(items, l->items, l->capacity);
    s->free(l->items, s->data);

    l->items = items;
    l->capacity = cap;
    return l;
}


frk_item_t*
frk_list_set(frk_list_t *l, int64_t index, frk_type_e t, void* val, int64_t len)
{
    if (index > l->count) {
        return NULL;
    }

    if (index == l->capacity && frk_list_resize(l) == NULL) {
        return NULL;
    }

    frk_item_free(l->store, l->items[index]);
    l->items[index] = frk_new_item(l->store, t, val, len);
    l->count += (index == l->count) ? 1 : 0;

    return l->items[index];
}


double*
frk_list_set_num(frk_list_t *l, int64_t index, double num)
{
    return frk_item_data(frk_list_set(l, index, FRK_NUM, &num, sizeof(double)), FRK_NUM);
}


frk_str_t*
frk_list_set_str(frk_list_t *l, int64_t index, char * val, int64_t vlen)
{
    return frk_item_data(frk_list_set(l, index, FRK_STR, val, vlen), FRK_STR);
}


frk_list_t*
frk_list_set_list(frk_list_t *l, int64_t index)
{
    return frk_item_data(frk_list_set(l, index, FRK_LIST, NULL, 0), FRK_LIST);
}


frk_dict_t*
frk_list_set_dict(frk_list_t *l, int64_t index)
{
    return frk_item_data(frk_list_set(l, index, FRK_DICT, NULL, 0), FRK_DICT);
}


double*
frk_list_append_num(frk_list_t *l, double num)
{
    return frk_list_set_num(l, l->count, num);
}


frk_str_t*
frk_list_append_str(frk_list_t *l, char *val, int64_t vlen)
{
    return frk_list_set_str(l, l->count, val, vlen);
}


frk_list_t*
frk_list_append_list(frk_list_t *l)
{
    return frk_list_set_list(l, l->count);
}


frk_dict_t*
frk_list_append_dict(frk_list_t *l)
{
    return frk_list_set_dict(l, l->count);
}


frk_list_iter_t*
frk_list_iter(frk_list_t *l, frk_list_iter_t *last)
{
    if ((last == NULL && l->count == 0)
        || (last != NULL && last + 1 >= l->items + l->count)) {
        return NULL;
    }
    return last == NULL ? l->items : last + 1;
}


void
frk_list_del(frk_list_t *l, int64_t index)
{
    if (index >= l->count) {
        return;
    }

    frk_item_t **start = l->items + index;
    memmove(start, start + 1, sizeof(frk_item_t *) * (l->count - index - 1));
    l->count--;
}


void
frk_list_clear(frk_list_t *l)
{
    frk_store_t *s = l->store;

    frk_item_t **i, **end = l->items + l->count;
    for (i = l->items; i < end; i++) {
        frk_item_free(s, *i);
        *i = NULL;
    }
    l->count = 0;

    frk_item_t **items = s->calloc(sizeof(frk_item_t*) * frk_list_init_size, s->data);
    if (items == NULL) {
        return;
    }

    s->free(l->items, s->data);
    l->capacity = frk_list_init_size;
    l->items = items;
}

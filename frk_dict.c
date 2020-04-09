// flatten dict
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>
#include <frk_dict.h>

const int64_t dict_init_size = 8;
const int64_t dict_load_rate = 4;
const int64_t dict_resize_rate = 4;


int64_t
frk_hash(char *str, int64_t len)
{
    int64_t i, hash = 0;
    for (i = 0; i < len; i++) {
        hash = hash * 31 + str[i];
    }
    return hash;
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

    // search next bucket
    if (next->node == NULL) {
        int64_t b = next->bucket + 1;
        for (; b < d->bucket_count;b++) {
            if (d->buckets[b] != NULL) {
                next->bucket = b;
                next->node = d->buckets[b];
                return next;
            }
        }
    }

    return NULL;
}


frk_node_t*
frk_get_node(frk_dict_t *d, char *key, int64_t key_len) {
    int64_t h = frk_hash(key, key_len);
    int64_t b = h % d->bucket_count;
 
    frk_node_t* i = d->buckets[b];
    for (;i != NULL; i = i->next) {
        if (i->hash == h) {
            return i;
        }
    }

    return NULL;
}


frk_dict_t*
frk_get_dict(frk_dict_t *d, char *key, int64_t key_len)
{
    frk_node_t* n = frk_get_node(d, key, key_len);
    return (n != NULL && n->type == FRK_DICT) ? n->d : NULL;
}


double*
frk_get_num(frk_dict_t *d, char *key, int64_t key_len)
{
    frk_node_t* n = frk_get_node(d, key, key_len);
    return (n != NULL && n->type == FRK_NUM) ? &n->n : NULL;
}


frk_str_t*
frk_get_str(frk_dict_t *d, char *key, int64_t key_len)
{
    frk_node_t* n = frk_get_node(d, key, key_len);
    return (n != NULL && n->type == FRK_STR) ? n->s : NULL;
}


frk_str_t*
frk_new_str(char *src, int64_t len, frk_malloc_fp m, void* data)
{
    frk_str_t* tar = m(sizeof(frk_str_t) + len, data);
    if (tar == NULL) {
        return NULL;
    }
    tar->len = len;
    memcpy(tar->data, src, len);
    return tar;
}


void
frk_dict_clear(frk_dict_t *d)
{
    int64_t b = 0;
    frk_node_t *iter;
    for (b = 0; b < d->bucket_count; b++) {
        for (iter = d->buckets[b]; iter != NULL; iter = iter->next) {
            frk_remove_node(d, iter);
        }
    }
}


void
frk_remove_node(frk_dict_t *d, frk_node_t* node)
{
    *node->pre = node->next;
    if (node->next != NULL) {
        node->next->pre = node->pre;
    }
    d->count--;

    if (node->type == FRK_STR) {
        d->free(node->s, d->data);
    } else if (node->type == FRK_DICT) {
        int64_t b;
        frk_node_t *iter;
        frk_dict_t *sub = node->d;

        for (b = 0; b < sub->bucket_count; b++) {
            for (iter = sub->buckets[b]; iter != NULL; iter = iter->next) {
                frk_remove_node(sub, iter);
            }
        }

        d->free(sub->buckets, d->data);
        d->free(sub, d->data);
    }

    d->free(node, d->data);
}


void
frk_remove(frk_dict_t *d, char *key, int64_t key_len)
{
    frk_node_t* n = frk_get_node(d, key, key_len);
    if (n != NULL) {
        frk_remove_node(d, n);
    }
}


void
frk_insert_node_to_bucket(frk_node_t* node, frk_node_t** buckets, int64_t count)
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
    int64_t count =  d->bucket_count * dict_resize_rate;

    frk_node_t **buckets = d->malloc(sizeof(frk_node_t *) * count, d->data);
    if (buckets == NULL) {
        return NULL;
    }

    int64_t n, b = 0;
    frk_node_t *i, *next;
    for (; b < d->bucket_count; b++) {
        for (i = d->buckets[b]; i != NULL; i = next) {
            next = i->next;
            frk_insert_node_to_bucket(i, buckets, count);
        }
    }

    d->bucket_count = count;
    d->buckets = buckets;

    return d;
}


frk_node_t*
frk_insert_node(frk_dict_t *d, char *key, int64_t key_len)
{
    frk_node_t* node = d->malloc(sizeof(frk_node_t), d->data);
    if (node == NULL) {
        return NULL;
    }

    frk_remove(d, key, key_len);

    node->key = frk_new_str(key, key_len, d->malloc, d->data);
    if (node->key == NULL) {
        d->free(node, d->data);
        return NULL;
    }

    node->hash = frk_hash(key, key_len);
    frk_insert_node_to_bucket(node, d->buckets, d->bucket_count);
    d->count++;

    if (d->count > dict_load_rate * d->bucket_count) {
        frk_dict_resize(d);
    }

    return node;
}


double*
frk_insert_num(frk_dict_t *d, char *key, int64_t key_len, double val)
{
    if (d == NULL || key == NULL || key_len == 0) {
        return NULL;
    }

    frk_node_t *node = frk_insert_node(d, key, key_len);
    if (node == NULL) {
        d->free(node, d->data);
        return NULL;
    }

    node->type = FRK_NUM;
    node->n = val;
    return &node->n;
}


frk_str_t*
frk_insert_string(frk_dict_t *d, char *key, int64_t key_len, char *val, int64_t val_len)
{
    if (d == NULL || key == NULL || key_len == 0 || val == NULL) {
        return NULL;
    }

    frk_node_t *node = frk_insert_node(d, key, key_len);
    frk_str_t* new = frk_new_str(val, val_len, d->malloc, d->data);
    if (node == NULL || new == NULL) {
        d->free(node, d->data);
        d->free(new, d->data);
        return NULL;
    }

    node->type = FRK_STR;
    node->s = new;
    return new;
}


frk_dict_t*
frk_insert_dict(frk_dict_t *d, char *key, int64_t key_len)
{
    if (d == NULL || key == NULL || key_len == 0) {
        return NULL;
    }

    frk_node_t *node = frk_insert_node(d, key, key_len);
    frk_dict_t *new = frk_new_dict(d->malloc, d->free, d->data);

    if (node == NULL || new == NULL) {
        d->free(node, d->data);
        d->free(new, d->data);
        return NULL;
    }

    node->type = FRK_DICT;
    node->d = new;
    return node->d;
}


frk_dict_t*
frk_new_dict(frk_malloc_fp m, frk_free_fp f, void* data)
{
    frk_dict_t *d = m(sizeof(frk_dict_t), data);
    d->bucket_count = dict_init_size;
    d->buckets = m(sizeof(frk_dict_t *) * dict_init_size, data);
    d->count = 0;
    d->data = data;
    d->free = f;
    d->malloc = m;
    return d;
}


int64_t
frk_str_dump(frk_str_t* str, char *buffer, int64_t len)
{
    if (len < str->len + 2) {
        return -1;
    }

    char *p = buffer;
    *p++ = '"';
    memcpy(p, str->data, str->len);
    p += str->len;
    *p++ = '"';
    return str->len + 2;
}


int64_t
frk_dict_dump(frk_dict_t *d, char *buffer, int64_t len)
{
    if (d == NULL || buffer == NULL || len <= 2) {
        return -1;
    }

    int64_t n;
    char *p = buffer, *end = buffer + len;

    *p++ = '{';
    frk_dict_iter_t tmp, *i = frk_dict_iter(d, NULL, &tmp);
    for (; i != NULL; i = frk_dict_iter(d, i, &tmp)) {
        frk_node_t *node = i->node;

        n = frk_str_dump(node->key, p, end - p);
        if (n < 0 || p + n + 1 >= end) {
            return -1;
        }
        p += n;
        *p++ = ':';

        if (node->type == FRK_STR) {
            n = frk_str_dump(node->s, p, end - p);
        } else if (node->type == FRK_NUM) {
            n = end - p < 20 ? -1: sprintf(p, "%lf", i->node->n);
        } else {
            n = frk_dict_dump(node->d, p, end - p);
        }
        
        if (n < 0 || p + n + 1 >= end) {
            return -1;
        }
        p += n;
        *p++ = ',';
    }

    if (*(p - 1) == ',') {
        p--;
    }
    *p++ = '}';

    return p - buffer;
}


char*
frk_next_char(char* p)
{
    for(;*p == ' ' && *p; p++);
    return p;
}


frk_dict_t*
frk_dict_load(frk_dict_t* d, char* json, char **end)
{
    char *p = json, *tmp, *key;
    if (*(p = frk_next_char(p))!= '{') {
        return NULL;
    }
    p++;

    // todo rewrite this
    while (*(p = frk_next_char(p))) {
        char* key;
        int64_t key_len;

        if (*p != '"' || (tmp = strchr(p+1, '"')) == NULL) {
            return NULL;
        }
        key = p + 1;
        key_len = tmp - p - 1;

        p = frk_next_char(tmp + 1);
        if (*p++ != ':') {
            return NULL;
        }
        p = frk_next_char(p);

        if (*p == '"') {
            if ((tmp = strchr(p+1, '"')) == NULL
                || frk_insert_string(d, key, key_len, p + 1, tmp - p - 1) == NULL) {
                return NULL;
            }
            p = tmp + 1;
        } else if (*p == '-' || (*p <= '9' && *p >= '0')) {
            double n = strtod(p, &p);
            if (frk_insert_num(d, key, key_len, n) == NULL) {
                return NULL;
            }
        } else if (*p == '{') {
            frk_dict_t* val = frk_insert_dict(d, key, key_len);
            if (frk_dict_load(val, p, &p) == NULL) {
                return NULL;
            }
        } else {
            return NULL;
        }

        if (*p++ == '}') {
            break;
        }

        if (*p == ',') {
            p++;
        }
    }

    if (end != NULL) {
        *end = p;
    }
    return d;
}
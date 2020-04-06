// flatten dict
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
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


frk_iter_t*
frk_iterator(frk_dict_t *d, frk_iter_t *last, frk_iter_t *next)
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


int64_t*
frk_get_int(frk_dict_t *d, char *key, int64_t key_len)
{
    frk_node_t* n = frk_get_node(d, key, key_len);
    return (n != NULL && n->type == FRK_INT) ? &n->i : NULL;
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


int64_t*
frk_insert_int(frk_dict_t *d, char *key, int64_t key_len, int64_t val)
{
    if (d == NULL || key == NULL || key_len == 0) {
        return NULL;
    }

    frk_node_t *node = frk_insert_node(d, key, key_len);
    if (node == NULL) {
        d->free(node, d->data);
        return NULL;
    }

    node->type = FRK_INT;
    node->i = val;
    return &node->i;
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
    char *p = buffer;
    *p++ = '"';
    memcpy(p, str->data, str->len);
    p += str->len;
    *p++ = '"';
    return p - buffer;
}


int64_t
frk_dict_dump(frk_dict_t *d, char *buffer, int64_t len)
{
    if (d == NULL || buffer == NULL || len <= 2) {
        return -1;
    }

    int64_t n;
    char* p = buffer;

    *p++ = '{';
    frk_iter_t tmp, *i = frk_iterator(d, NULL, &tmp);
    for (; i != NULL; i = frk_iterator(d, i, &tmp)) {
        frk_node_t *node = i->node;
        
        if ((n = frk_str_dump(node->key, p, len - (p - buffer))) < 0) {
            return -1;
        }
        p += n;
        *p++ = ':';

        if (i->node->type == FRK_STR) {
            n = frk_str_dump(node->s, p, len - (p - buffer));
        } else if (i->node->type == FRK_INT) {
            n = len - (p - buffer) < 20 ? -1: sprintf(p, "%ld", i->node->i);
        } else {
            n = frk_dict_dump(i->node->d, p, len - (p - buffer));
        }
        
        if (n < 0) {
            return n;
        }
        p += n;
        *p++ = ',';
    }

    *(p - 1) = '}';

    return p - buffer;
}

// int frk_dump_json(frk_dict_t *d, frk_str_t* buf);
// int frk_load_json(frk_dict_t *d, frk_str_t* buf);
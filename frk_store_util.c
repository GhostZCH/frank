#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <frk_store_util.h>



int64_t
frk_dump_num(frk_item_t *item, char* buf, int64_t len)
{
    int n = snprintf(buf, len, "%lf", item->n);
    if (n == len) {
        return -1;
    }
    return n;
}


int64_t
frk_dump_orgin_str(char *str, int64_t slen, char* buf, int64_t blen)
{
    int64_t i;
    int64_t quotes = 0;

    for (i = 0; i < slen; i++) {
        if (str[i] == '"') {
            quotes++;
        }
    }

    if (quotes + slen + 2 > blen) {
        return -1;
    }

    *buf++ = '"';
    for (i = 0; i < slen; i++) {
        if (str[i] == '"') {
            *buf++ = '\\';
        }
        *buf++ = str[i];
    }
    *buf++ = '"';

    return quotes + slen + 2;
}


int64_t
frk_dump_str(frk_item_t *item, char* buf, int64_t len)
{
    return frk_dump_orgin_str(item->s->data, item->s->len, buf, len);
}


int64_t
frk_dump_key(frk_dict_node_t *node, char* buf, int64_t len)
{
    int64_t n = frk_dump_orgin_str(node->key, node->klen, buf, len);

    if (n < 0) {
        return n;
    }

    if (n + 1 >= len) {
        return -1;
    }

    buf[n] = ':';
    return n + 1;
}


int64_t
frk_dump_dict(frk_item_t *item, char* buf, int64_t len)
{
    if (len < 2) {
        return -1;
    }

    *buf++ = '{';
    len--;

    int64_t n, total = 1;
    frk_dict_iter_t tmp, *i;

    i = frk_dict_iter(item->d, NULL, &tmp);
    for (; i != NULL; i = frk_dict_iter(item->d, i, &tmp)) {
        // dump key
        n = frk_dump_key(i->node, buf, len);
        if (n < 0) {
            return n;
        }
        total += n;
        buf += n;
        len -= n;

        // dump value
        n = frk_dump_item(i->item, buf, len, ',');
        if (n < 0) {
            return n;
        }
        buf += n;
        len -= n ;
        total += n;
    }

    if (*(buf - 1) == ',') {
        *(buf - 1) = '}';
    }

    return total;
}



int64_t
frk_dump_list(frk_item_t *item, char* buf, int64_t len)
{
    if (len < 2) {
        return -1;
    }

    *buf++ = '[';
    len--;

    int64_t n, total = 1;
    frk_list_iter_t *i = frk_list_iter(item->l, NULL);
    for (; i != NULL; i = frk_list_iter(item->l, i)) {
        n = frk_dump_item(*i, buf, len, ',');
        if (n < 0) {
            return n;
        }
        buf += n;
        len -= n;
        total += n ;
    }

    if (*(buf - 1) == ',') {
        *(buf - 1) = ']';
    }

    return total;
}


int64_t
frk_dump_item(frk_item_t *item, char* buf, int64_t len, char end)
{
    int64_t n;

    switch (item->type)
    {
    case FRK_NUM:
        n = frk_dump_num(item, buf, len);
        break;

    case FRK_STR:
        n = frk_dump_str(item, buf, len);
        break;

    case FRK_DICT:
        n = frk_dump_dict(item, buf, len);
        break;

    case FRK_LIST:
        n = frk_dump_list(item, buf, len);
        break;

    default:
        n = -2;
    }

    if (n < 0) {
        return n;
    }

    if (end > 0) {
        if (n + 1 >= len) {
            return -1;
        }
        buf[n] = end;
    }

    return n + 1;
}


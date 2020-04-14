#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <frk_store_util.h>


// int64_t
// frk_str_dump(frk_str_t* str, char *buffer, int64_t len)
// {
//     if (len < str->len + 2) {
//         return -1;
//     }

//     char *p = buffer;
//     *p++ = '"';
//     memcpy(p, str->data, str->len);
//     p += str->len;
//     *p++ = '"';
//     return str->len + 2;
// }


// int64_t
// frk_dump(frk_item_t *item, char* buf, int64_t len, char** end)
// {
//     if (item == NULL || buf == NULL || end == buf) {
//         return -1;
//     }

//     int64_t n;
//     char *p = buf;

//     *p++ = '{';
//     frk_dict_iter_t tmp, *i = frk_dict_iter(d, NULL, &tmp);
//     for (; i != NULL; i = frk_dict_iter(d, i, &tmp)) {
//         frk_dict_node_t *node = i->node;

//         n = frk_str_dump(node->key, p, end - p);
//         if (n < 0 || p + n + 1 >= end) {
//             return -1;
//         }
//         p += n;
//         *p++ = ':';

//         if (node->type == FRK_STR) {
//             n = frk_str_dump(node->s, p, end - p);
//         } else if (node->type == FRK_NUM) {
//             n = end - p < 20 ? -1: sprintf(p, "%lf", i->node->n);
//         } else {
//             n = frk_dict_dump(node->d, p, end - p);
//         }
        
//         if (n < 0 || p + n + 1 >= end) {
//             return -1;
//         }
//         p += n;
//         *p++ = ',';
//     }

//     if (*(p - 1) == ',') {
//         p--;
//     }
//     *p++ = '}';

//     return p - buffer;
// }


// char*
// frk_next_char(char* p)
// {
//     for(;*p == ' ' && *p; p++);
//     return p;
// }


// frk_item_t*
// frk_load(frk_store_t *s, char* json, char **end)
// {
//     char *p = json, *tmp, *key;
//     if (*(p = frk_next_char(p))!= '{') {
//         return NULL;
//     }
//     p++;

//     // todo rewrite this
//     while (*(p = frk_next_char(p))) {
//         char* key;
//         int64_t key_len;

//         if (*p != '"' || (tmp = strchr(p+1, '"')) == NULL) {
//             return NULL;
//         }
//         key = p + 1;
//         key_len = tmp - p - 1;

//         p = frk_next_char(tmp + 1);
//         if (*p++ != ':') {
//             return NULL;
//         }
//         p = frk_next_char(p);

//         if (*p == '"') {
//             if ((tmp = strchr(p+1, '"')) == NULL
//                 || frk_dict_set_str(d, key, key_len, p + 1, tmp - p - 1) == NULL) {
//                 return NULL;
//             }
//             p = tmp + 1;
//         } else if (*p == '-' || (*p <= '9' && *p >= '0')) {
//             double n = strtod(p, &p);
//             if (frk_dict_set_num(d, key, key_len, n) == NULL) {
//                 return NULL;
//             }
//         } else if (*p == '{') {
//             frk_dict_t* val = frk_dict_set_dict(d, key, key_len);
//             if (frk_dict_load(val, p, &p) == NULL) {
//                 return NULL;
//             }
//         } else {
//             return NULL;
//         }

//         if (*p++ == '}') {
//             break;
//         }

//         if (*p == ',') {
//             p++;
//         }
//     }

//     if (end != NULL) {
//         *end = p;
//     }
//     return d;
// }
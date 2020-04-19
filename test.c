#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <frk_store.h>
#include <frk_store_util.h>
#include <frk_slab.h>


const size_t size = 1024 * 10240;


void*
slab_malloc(int64_t size, void* data)
{
    frk_slab_t *s = data;
    return frk_slab_malloc(s, size, 1);
}


void
slab_free(void* ptr, void* data)
{
    frk_slab_t *s = data;
    return frk_slab_free(s, ptr);
}


void
slab_test()
{
    int i;
    void* pool = malloc(size);
    frk_slab_t *s = frk_new_slab(pool, size);

    // basic use
    int64_t *p = frk_slab_malloc(s, 100, 1);
    p[0] = 1;
    frk_slab_free(s, p);

    // test malloc free
    for (int i = 0; i < 5000; i += 1) {
        void *p = frk_slab_malloc(s, i, 1);
        frk_slab_free(s, p);
        if (p == NULL) {
            printf("malloc %d %p\n", i, p);
            break;
        }
    }

    // test full
    for (int i = 0; i < 10241; i++) {
        void *p = frk_slab_malloc(s, 1024, 0);
        if (p == NULL) {
            printf("malloc %d %p\n", i, p);
            break;
        }
    }

    free(s);
}


// just for test
#define _STR_(str) (str), (strlen(str))

void store_test()
{
    frk_dict_iter_t tmp, *i;

    void* pool = malloc(size);
    frk_slab_t *slab = frk_new_slab(pool, size);
    frk_store_t *store = frk_new_store(slab_malloc, slab_free, slab);

    frk_item_t* root = frk_root(store);
    frk_dict_t* sites = root->d;

    frk_dict_t* a = frk_dict_set_dict(sites, _STR_("a.com"));

    frk_dict_t* wild = frk_dict_set_dict(a, _STR_(".a.com"));
    frk_dict_set_num(wild, _STR_("timeout"), 10);
    frk_dict_set_num(wild, _STR_("buffer"), 4096);
    frk_dict_set_str(wild, _STR_("upstream"), _STR_("127.0.0.1:5000"));

    frk_dict_t* www = frk_dict_set_dict(a, _STR_("www.a.com"));
    frk_dict_set_num(www, _STR_("timeout"), 5);
    frk_dict_set_num(www, _STR_("buffer"), 1024);
    frk_dict_set_str(www, _STR_("upstream"), _STR_("192.168.0.3:5000"));

    frk_dict_t* news = frk_dict_set_dict(a, "news.a.com", strlen("news.a.com"));
    frk_dict_set_num(news, _STR_("timeout"), 5);
    frk_dict_set_str(news, _STR_("upstream"), _STR_("192.168.0.100:5000"));

    frk_dict_t* black = frk_dict_set_dict(news, "black_ip", strlen("black_ip"));
    frk_dict_set_num(black, _STR_("222.222.222.1"), 1);
    frk_dict_set_num(black, _STR_("222.222.222.2"), 1);
    frk_dict_set_num(black, _STR_("222.222.222.3"), 1);
    frk_dict_set_num(black, _STR_("222.222.222.4"), 1);

    frk_list_t* rules = frk_dict_set_list(news, "rules", strlen("rules"));
    frk_list_append_num(rules, 10010);
    frk_list_append_str(rules, _STR_("_XSS_"));
    frk_list_set_str(rules, 1, _STR_("XSS"));
    frk_list_append_dict(rules);
    frk_dict_set_num(frk_list_get_dict(rules, 2), _STR_("A"), 100);
    frk_dict_set_num(frk_list_get_dict(rules, 2), _STR_("B"), 200);
    frk_dict_set_num(frk_list_get_dict(rules, 2), _STR_("C"), 300);

    printf("rules 0 %lf\n", *frk_list_get_num(rules, 0));
    printf("rules 1 %s\n", frk_list_get_str(rules, 1)->data);

    // for each
    i = frk_dict_iter(frk_list_get_dict(rules, 2), NULL, &tmp);
    for (; i != NULL; i = frk_dict_iter(frk_list_get_dict(rules, 2), i, &tmp)) {
        printf("%s: %lf\n", i->node->key, i->item->n);
    }

    frk_dict_t* b = frk_dict_set_dict(sites, _STR_("b.com"));
    frk_dict_t* wild_b = frk_dict_set_dict(b, _STR_(".b.com"));
    frk_dict_set_num(wild_b, _STR_("timeout"), 100);
    frk_dict_set_num(wild_b, _STR_("buffer"), 40960);

    frk_dict_t* c = frk_dict_set_dict(sites, _STR_("c.com"));
    frk_dict_t* www_c = frk_dict_set_dict(c, _STR_("www.c.com"));
    frk_dict_set_num(www_c, _STR_("timeout"), 200);
    frk_dict_set_num(www_c, _STR_("buffer"), 10240);

    // for each 
    i = frk_dict_iter(black, NULL, &tmp);
    for (; i != NULL; i = frk_dict_iter(black, i, &tmp)) {
        printf("%s: %lf\n", i->node->key, i->item->n);
    }

    char *buffer = calloc(1024, 100);
    frk_dump_item(root, buffer, 102400, '\n');
    printf("sites:\n %s\n", buffer);
    free(buffer);
}


void 
load_test()
{
    char *json = 
        "{\"a.com\":"
            "{\"news.a.com\":{\"timeout\":5.000000,\"upstream\":\"192.168.0.100:5000\","
                "\"black_ip\":{\"222.222.222.4\":1.000000,\"222.222.222.1\":1.000000,\"222.222.222.2\":1.000000,\"222.222.222.3\":1.000000}},"
            "\"www.a.com\":{\"buffer\":1024.000000,\"timeout\":5.000000,\"upstream\":\"192.168.0.3:5000\"},"
            "\".a.com\":{\"buffer\":4096.000000,\"timeout\":10.000000,\"upstream\":\"127.0.0.1:5000\"}},"
        "\"b.com\":"
            "{\".b.co\":{\"buffer\":40960.000000,\"timeout\":100.000000}},"
        "\"c.com\":"
            "{\"www.c.com\":{\"buffer\":10240.000000,\"timeout\":200.000000}}}";

    void* pool = malloc(size);
    frk_slab_t *slab = frk_new_slab(pool, size);
    frk_store_t *store = frk_new_store(slab_malloc, slab_free, slab);

    frk_item_t* root = frk_root(store);
    frk_dict_t* sites = root->d;

    frk_dict_t *a_com = frk_dict_get_dict(sites, "a.com", strlen("a.com"));
    frk_dict_t *news_a_com = frk_dict_get_dict(a_com, "news.a.com", strlen("news.a.com"));
    frk_dict_t *black = frk_dict_get_dict(news_a_com, "black_ip", strlen("black_ip"));

    frk_dict_iter_t tmp, *i = frk_dict_iter(black, NULL, &tmp);
    for (; i != NULL; i = frk_dict_iter(black, i, &tmp)) {
        // i->node->key->data may not ends with '\0' don't do that in product
        printf("%s: %lf\n", i->node->key, i->node->item->n);
    }

//     char *buffer = calloc(1024, 100);
//     frk_dict_dump(sites, buffer, 102400);
//     printf("load:\n %s\n", buffer);

//     frk_dict_clear(sites);
//     memset(buffer, 0, 102400);
//     frk_dict_dump(sites, buffer, 102400);
//     printf("load:\n %s\n", buffer);
}

int
main() {
    slab_test();
    store_test();
    // load_test();

    return 0;
}

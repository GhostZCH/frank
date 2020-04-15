#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <frk_store.h>
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
    for (int i = 0; i < 5000; i += 23) {
        void *p = frk_slab_malloc(s, i, 1);
        // printf("malloc %d %p\n", i, p);
        frk_slab_free(s, p);
    }

    // test full
    for (int i = 0; i < 10241; i++) {
        void *p = frk_slab_malloc(s, 1024, 0);
        printf("malloc %p\n", p);
    }

    free(s);
}


void store_test()
{
    void* pool = malloc(size);
    frk_slab_t *slab = frk_new_slab(pool, size);
    frk_store_t *store = frk_new_store(slab_malloc, slab_free, slab);

    frk_dict_t* sites = frk_new_dict(store);

    frk_dict_t* a = frk_dict_set_dict(sites, "a.com", strlen("a.com"));

    frk_dict_t* wild = frk_dict_set_dict(a, ".a.com", strlen(".a.com"));
    frk_dict_set_num(wild, "timeout", strlen("timeout"), 10);
    frk_dict_set_num(wild, "buffer", strlen("buffer"), 4096);
    frk_dict_set_str(wild, "upstream", strlen("upstream"), "127.0.0.1:5000", strlen("127.0.0.1:5000"));

    frk_dict_t* www = frk_dict_set_dict(a, "www.a.com", strlen("www.a.com"));
    frk_dict_set_num(www, "timeout", strlen("timeout"), 5);
    frk_dict_set_num(www, "buffer", strlen("buffer"), 1024);
    frk_dict_set_str(www, "upstream", strlen("upstream"), "192.168.0.3:5000", strlen("192.168.0.3:5000"));

    frk_dict_t* news = frk_dict_set_dict(a, "news.a.com", strlen("news.a.com"));
    frk_dict_set_num(news, "timeout", strlen("timeout"), 5);
    frk_dict_set_str(news, "upstream", strlen("upstream"), "192.168.0.100:5000", strlen("192.168.0.100:5000"));
    frk_dict_t* black = frk_dict_set_dict(news, "black_ip", strlen("black_ip"));
    frk_dict_set_num(black, "222.222.222.1", strlen("222.222.222.1"), 1);
    frk_dict_set_num(black, "222.222.222.2", strlen("222.222.222.2"), 1);
    frk_dict_set_num(black, "222.222.222.3", strlen("222.222.222.3"), 1);
    frk_dict_set_num(black, "222.222.222.4", strlen("222.222.222.4"), 1);
    frk_list_t* rules = frk_dict_set_list(news, "rules", strlen("rules"));
    frk_list_append_num(rules, 10010);
    frk_list_append_str(rules, "_XSS_", 5);
    frk_list_set_str(rules, 1, "XSS", 3);
    frk_list_append_dict(rules);
    frk_dict_set_num(frk_list_get_dict(rules, 2), "A", 1, 100);
    frk_dict_set_num(frk_list_get_dict(rules, 2), "B", 1, 200);
    frk_dict_set_num(frk_list_get_dict(rules, 2), "C", 1, 300);

    frk_dict_t* b = frk_dict_set_dict(sites, "b.com", strlen("b.com"));
    frk_dict_t* wild_b = frk_dict_set_dict(b, ".b.com", strlen(".bcom"));
    frk_dict_set_num(wild_b, "timeout", strlen("timeout"), 100);
    frk_dict_set_num(wild_b, "buffer", strlen("buffer"), 40960);

    frk_dict_t* c = frk_dict_set_dict(sites, "c.com", strlen("c.com"));
    frk_dict_t* www_c = frk_dict_set_dict(c, "www.c.com", strlen("www.c.com"));
    frk_dict_set_num(www_c, "timeout", strlen("timeout"), 200);
    frk_dict_set_num(www_c, "buffer", strlen("buffer"), 10240);

    char *buffer = calloc(1024, 100);

    // for each 
    frk_dict_iter_t tmp, *i = frk_dict_iter(black, NULL, &tmp);
    for (; i != NULL; i = frk_dict_iter(black, i, &tmp)) {
        // i->node->key->data may not ends with '\0' don't do that in product
        printf("%s: %lf\n", i->node->key, i->node->item->n);
    }

    // frk_dict_dump(sites, buffer, 102400);
    // printf("sites:\n %s\n", buffer);
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

    frk_dict_t* sites = frk_new_dict(store);

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

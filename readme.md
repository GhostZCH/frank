# frank

一个Ｃ语言编写的包含几个简单功能的库，代码尽可能追求简单易懂，便于维护。可以通过直接复制源文件到项目里进行修改和使用。使用详细见test.c。

## frk_slab

一个在连续内存上使用的slab内存分配工具，特别适用于在共享内存和持久化


### 使用

    // get same memery, use shm or mmap for share memery
    void* pool = malloc(size);
    
    // init
    frk_slab_t *s = frk_new_slab(pool, size);

    // malloc
    int64_t *p = frk_slab_malloc(s, sizeof(int64_t) * 5, 1);

    // use
    p[0] = 1;

    // free
    frk_slab_free(s, p);


### 备注

+ 支持如下size的slab:　64, 128, 256, 512, 1024, 2048, 4096

## frk_store

一个可以自定义内存分配函数的k-v存储结构，可以实现类似json或者python中dict的多层存储, 与frk_slab结合使用可以解决在多进程间共享复杂配置的问题，支持num,str,list,dict等数据结构。支持导入和导出jso。建议与frk_slab配合使用


### 使用


api 详见　frk_store.h


    void
    store_test()
    {
        frk_dict_iter_t tmp, *i;

        // init a store
        void* pool = malloc(size);
        frk_slab_t *slab = frk_new_slab(pool, size);
        frk_store_t *store = frk_new_store(slab_malloc, slab_free, slab);

        // get a root, root is a dict item
        frk_item_t* root = frk_root(store);
        frk_dict_t* sites = root->d;

        // insert
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

        // dump
        char *buffer = calloc(1024, 100);
        frk_dump_item(root, buffer, 102400, '\n');
        printf("sites:\n %s\n", buffer);

        // load
        frk_item_t *load = frk_load_item(store, buffer, NULL);

        // dump again
        frk_dump_item(load, buffer, 102400, '\n');
        printf("load:\n %s\n", buffer);

        free(buffer);
    }


### 备注

+ 主要针对多读少写的情况
+ 仅支持加载正常格式的json，不做安全和格式校验，建议导入其他库导出的标准格式的json
+ 不同于protocal buffer, 不需要预先定义格式
+ 不同于cjson，主要方便快速查询读取，代码更加简单
+ 虽然不建议，但是支持多个root


## TODO

+ review code & fix bugs
+ support user object
+ refactor json operation
+ add filter (bloom filter and wildcard filter)
+ slab suport bigger and smaller sizes

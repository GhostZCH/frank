# frank

## frk_slab

一个在连续内存上使用的slab内存分配工具，特别适用于在共享内存和持久化

## frk_dict

一个可以自定义内存分配函数的k-v存储结构，可以实现类似json或者python中dict的多层存储, 与frk_slab结合使用可以解决在多进程间共享复杂配置的问题

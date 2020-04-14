#ifndef _FRK_STORE_UTIL_
#define _FRK_STORE_UTIL_

#include <stdint.h>
#include <frk_store.h>

int64_t frk_dump(frk_item_t *item, char* buf, int64_t len, char** end);
frk_item_t* frk_load(frk_store_t *store, char* json, int64_t len, char** end);

#endif
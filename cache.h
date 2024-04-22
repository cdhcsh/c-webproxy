#ifndef __CACHE_H__
#define __CACHE_H__

#include "csapp.h"

#define MAX_CACHE_SIZE 1049000

typedef char *data_t;

typedef struct node_t {
    data_t key;
    data_t value;
    size_t value_len;
    struct node_t *next;
    struct node_t *prev;
} node_t;
typedef struct cache_t {
    size_t size;
    node_t *front;
    node_t *rear;
} cache_t;

cache_t *new_cache();

node_t *cache_get(cache_t *cache,data_t key);

int cache_add(cache_t *cache,data_t key, data_t value,size_t value_len);

#endif

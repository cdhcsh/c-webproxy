#include "cache.h"

/* function prototypes */

/* 노드를 연결리스트에서 제거*/
static void _cache_pop(cache_t *cache,node_t *node);
/* 노드를 front에 삽입*/
static void _cache_node_insert(cache_t *cache,node_t *node);
/* 노드를 front로 이동*/
static void _cache_move_front(cache_t *cache,node_t *node);
/* 노드 메모리 반환*/
static void _node_delete(node_t *node);

/* main functions*/

cache_t* new_cache() {
    return (cache_t*)Calloc(1, sizeof(cache_t));

}

node_t *cache_get(cache_t *cache,data_t key) {
    node_t *p = cache->front;
    while (p != NULL) {
        if (memcmp(p->key, key, MAXLINE) == 0) {
            _cache_move_front(cache,p);
            return p;
        }
        p = p->next;
    }
    return NULL;
}

int cache_add(cache_t *cache,data_t key, data_t value,size_t value_len) {

    if (cache == NULL) return -1;
    if (value_len > MAX_CACHE_SIZE) return 0;
    while (cache->size + value_len > MAX_CACHE_SIZE) { // 캐시 최대 사이즈 초과시 기존 캐쉬 삭제
        node_t *tmp = cache->rear;
        _cache_pop(cache,tmp);
        _node_delete(tmp);
    }
    node_t *node = (node_t*)calloc(1, sizeof(node_t));
    node->key = key;
    node->value = value;
    node->value_len = value_len;
    _cache_node_insert(cache,node);

    cache->size += value_len;
    return 0;

}

/* sub functions */

static void _cache_pop(cache_t *cache,node_t *node) {
    if (node == NULL) {
        return;
    }
    if (node == cache->front) {
        cache->front = node->next;
    }
    if (node == cache->rear) {
        cache->rear = node->prev;
    }
    if (node->prev != NULL) {
        node->prev->next = node->next;
    }
    if (node->next != NULL) {
        node->next->prev = node->prev;
    }
    node->next = NULL;
    node->prev = NULL;
    return;
}

static void _cache_node_insert(cache_t *cache,node_t *node) {
    if (node == NULL) return;
    node_t *front = cache->front;
    if (front == NULL) {
        cache->front = node;
        cache->rear = node;
    } else {
        node->next = front;
        front->prev = node;
        cache->front = node;
    }
}

static void _cache_move_front(cache_t *cache,node_t *node) {
    if (node == NULL || node == cache->front) return;
    _cache_pop(cache,node);
    _cache_node_insert(cache,node);
}

static void _node_delete(node_t *node) {
    free(node->key);
    free(node->value);
    free(node);
    node = NULL;
}

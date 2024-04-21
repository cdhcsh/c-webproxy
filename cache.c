#include "cache.h"


static void _cache_pop(node_t *node);

static void _cache_move_front(node_t *node);

static void _node_delete(node_t *node);

static void _cache_node_insert(node_t *node);

cache_t *cache; // 캐쉬 구조체 선언

void cache_init() {
    cache = Calloc(1, sizeof(cache_t));
}

node_t *cache_get(data_t key) {
    node_t *p = cache->front;
    while (p != NULL) {
        if (memcmp(p->key, key, MAXLINE) == 0) {
            _cache_move_front(p);
            return p;
        }
        p = p->next;
    }
    return NULL;
}

int cache_add(data_t key, data_t value,size_t value_len) {
    if (cache == NULL) return -1;
    size_t value_size = strlen(value);
    if (value_size > MAX_CACHE_SIZE) return 0;
    while (cache->size + value_size > MAX_CACHE_SIZE) { // 캐시 최대 사이즈 초과시 기존 캐쉬 삭제
        node_t *tmp = cache->rear;
        _cache_pop(cache->rear);
        _node_delete(tmp);
    }
    node_t *node = (node_t*)calloc(1, sizeof(node_t));
    node->key = key;
    node->value = value;
    node->value_len = value_len;
    _cache_node_insert(node);

    cache->size += value_size;
    return 0;
}

static void _cache_node_insert(node_t *node) {
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

static void _node_delete(node_t *node) {
    free(node->key);
    free(node->value);
    free(node);
    node = NULL;
}

static void _cache_move_front(node_t *node) {
    if (node == NULL || node == cache->front) return;
    _cache_pop(node);
    _cache_node_insert(node);
}

static void _cache_pop(node_t *node) {
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


#ifdef DEBUG

int main(void){
    cache_init();
    cache_add("key1","response1");
    cache_add("key2","response2");
    cache_add("key3","response3");
    cache_add("key4","response4");

    printf("%s\n",cache_get("key3"));
    printf("%s\n",cache->front->value);
        printf("%s\n",cache->rear->value);
    return 0;
}
#endif


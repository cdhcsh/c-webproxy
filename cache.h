#ifndef __CACHE_H__
#define __CACHE_H__

#include "csapp.h"

/* ===================================================================================
 * ===================================== DEFINES  ====================================
 * =================================================================================== */

#define _CACHE_
#define MAX_CACHE_SIZE 1049000

typedef char *data_t;

typedef struct node_t {
    data_t key;
    data_t value;
    ssize_t value_len;
    struct node_t *next;
    struct node_t *prev;
} node_t;
typedef struct cache_t {
    size_t size;
    node_t *front;
    node_t *rear;
} cache_t;

/* ===================================================================================
 * ============================ MAIN FUNCTION PROTOTYPES  ============================
 * =================================================================================== */

/* 캐쉬 메모리 생성 */
cache_t *new_cache();

/* 캐쉬에서 데이터 검색, 검색한 데이터는 front로 이동 */
node_t *cache_get(cache_t *cache,data_t key);

/* 캐쉬에 데이터 추가*/
int cache_add(cache_t *cache,data_t key, data_t value,ssize_t value_len);

#endif

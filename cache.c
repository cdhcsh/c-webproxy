#include "csapp.h"
#include "cache.h"


static void _cache_remove();

static void _cache_erase(string_t key);

static void _cache_move_front(string_t key);

cache_t* cache; // 캐쉬 구조체 선언

void cache_init(){
    cache = Calloc(1,sizeof(cache_t));
}

void cache_delete(){
    node_t *tmp = cache->front;
    while(tmp != NULL){
        t
    }
}

string_t cache_get(string_t key){

}

int cache_add(string_t key,string_t value){
    if(cache == NULL) return -1;
    size_t value_size = strlen(value);
    while(cache->size + value_size > MAX_CACHE_SIZE){ // 캐시 최대 사이즈 초과시 기존 캐쉬 삭제
        _cache_remove();
    }

}


static void _cache_remove(){

}


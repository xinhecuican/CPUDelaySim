#include "cache/replace/lru.h"

void LRUReplace::insert(int set, int id) {
    std::list<int>::iterator& lru_iter = lru_map[set][id];
    lru_list[set].splice(lru_list[set].end(), lru_list[set], lru_iter);
}

void LRUReplace::clear(int set, int id) {
    std::list<int>::iterator& lru_iter = lru_map[set][id];
    lru_list[set].splice(lru_list[set].begin(), lru_list[set], lru_iter);
}

int LRUReplace::get(int set) {
    int id = lru_list[set].front();
    lru_list[set].splice(lru_list[set].end(), lru_list[set], lru_list[set].begin());
    return id;
}

void LRUReplace::setParams(int arg_num, ...) {
    va_list args;
    va_start(args, arg_num);
    set = va_arg(args, int);
    way = va_arg(args, int);
    va_end(args);
    
    lru_list.resize(set);
    lru_map.resize(set);
    for (int i = 0; i < set; i++) {
        for (int j = 0; j < way; j++) {
            lru_map[i][j] = lru_list[i].insert(lru_list[i].end(), j);
        }
    }
}
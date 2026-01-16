#include "cache/dcache.h"

void DCache::afterLoad() {
    Cache::afterLoad();
}

bool DCache::lookup(int callback_id, CacheReq* req) {

    return true;
}

void DCache::tick() {

}

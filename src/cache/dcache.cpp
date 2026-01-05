#include "cache/dcache.h"

DCache::~DCache() {
    delete[] mshr_entrys;
}

void DCache::afterLoad() {
    mshr_entrys = new LookupEntry[mshr_size];
    Cache::afterLoad();
}



bool DCache::lookup(int id, snoop_req_t req, uint64_t addr, uint32_t size) {
    uint64_t tag;
    uint32_t set, offset;
    splitAddr(addr, tag, set, offset);
    CacheTagv* tagv = match(tag, set);
    if (tagv) {

    }
    return true;
}

void DCache::setParentCallback() {
}

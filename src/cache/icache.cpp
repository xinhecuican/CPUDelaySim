#include "cache/icache.h"

bool ICache::lookup(int id, snoop_req_t req, uint64_t addr, uint32_t size) {
// TODO add delay
    uint64_t tag;
    uint32_t set, offset;
    splitAddr(addr, tag, set, offset);
    if (lookup_valid && _match) {
        lookup_valid = false;
        callbacks[0](lookup_id, nullptr);
    } else if (lookup_valid) {
        replace_way = replace->get(set);
        parent->lookup(lookup_id | this->id, lookup_req, lookup_addr, lookup_size);
    }
    if (lookup_valid && !_match || !req) {
        return false;
    }

    CacheTagv* tagv = match(tag, set);

    lookup_id = id;
    if (tagv) {
        _match = true;
    } else if (likely(parent)) {
        _match = false;
        lookup_valid = true;
        lookup_offset = offset;
        lookup_size = size;
        lookup_addr = addr;
        lookup_set = set;
        lookup_req = req;
    }
    return true;
}

void ICache::setParentCallback() {
    parent->setCallback(std::bind(&ICache::callbackFunc, this, std::placeholders::_1, std::placeholders::_2));
}

void ICache::callbackFunc(int id, CacheTagv* tagv_i) {
    lookup_valid = false;
    CacheTagv* tagv = tagvs[lookup_set][replace_way];
    tagv->tag = lookup_addr >> tag_offset;
    tagv->valid = true;
    callbacks[0](id & ~this->id, nullptr);
}

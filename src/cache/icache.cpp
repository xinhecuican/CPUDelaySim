#include "cache/icache.h"

void ICache::afterLoad() {
    callback_id = parent->setCallback(std::bind(&ICache::callbackFunc, this, std::placeholders::_1, std::placeholders::_2));
    lookup_req = new CacheReq;
    lookup_req->callback_id = callback_id;
    lookup_req->id[1] = 0;
    lookup_req->size = line_size;
    Cache::afterLoad();
}

ICache::~ICache() {
    delete lookup_req;
}

bool ICache::lookup(int callback_id, CacheReq* req) {
    if (!flush_valid && (state == IDLE || state == LOOKUP && _match)) {
        idle_req = req;
        idle_req_valid = true;
        return true;
    }
    return false;
}

void ICache::tick() {
    if (unlikely(flush_valid)) {
        flush_num--;
        for (int i = 0; i < way; i++) {
            tagvs[flush_set][i]->valid = false;
        }
        flush_set = (flush_set + 1) % set_size;
        if (flush_num == 0) {
            flush_valid = false;
        }
    } else {
        switch(state) {
            case IDLE: {
                if (idle_req_valid) {
                    lookup_req->addr = idle_req->addr;
                    lookup_req->id[0] = idle_req->id[0];
                    state = LOOKUP;
                    idle_req_valid = false;
                }
                break;
            }
            case LOOKUP: {
                uint64_t tag;
                uint32_t set, offset;
                splitAddr(lookup_req->addr, tag, set, offset);
                CacheTagv* tagv = match(tag, set);
                _match = tagv != nullptr;
                if (!_match) {
                    bool success = parent->lookup(callback_id, lookup_req);
                    if (success) {
                        lookup_set = set;
                        lookup_tag = lookup_req->addr >> tag_offset;
                        replace_way = replace->get(set);
                        state = MISS;
                    }
                } else if (idle_req_valid) {
                    lookup_req->addr = idle_req->addr;
                    lookup_req->id[0] = idle_req->id[0];
                    idle_req_valid = false;
                } else {
                    state = IDLE;
                }
                break;
            }
            case MISS: {
                break;
            }
            case REFILL: {
                state = IDLE;
                idle_req_valid = false;
                break;
            }
        }
    }
}

void ICache::callbackFunc(uint16_t* ids, CacheTagv* tagv_i) {
    assert(state == MISS);
    CacheTagv* tagv = tagvs[lookup_set][replace_way];
    tagv->tag = lookup_tag;
    tagv->valid = true; 
    callbacks[0](lookup_req->id, nullptr);
    state = REFILL;
}


void ICache::flush(uint64_t addr, uint32_t asid) {
    flush_set = 0;
    flush_num = set_size;
    flush_valid = true;
}
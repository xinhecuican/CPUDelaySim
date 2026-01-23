#include "cache/icache.h"

void ICache::afterLoad() {
    callback_id = parent->setCallback([this](uint16_t* ids, CacheTagv* tagv_i) {
        CacheTagv* tagv = this->tagvs[this->lookup_set][this->replace_way];
        tagv->tag = this->lookup_tag;
        tagv->valid = true; 
        if (this->req_clear_wait) {
            this->req_clear_wait = false;
        } else {
            this->callbacks[0](this->lookup_req->id, nullptr);
            this->state = REFILL;
        }
    });
    lookup_req = new CacheReq;
    lookup_req->id[1] = 0;
    lookup_req->size = line_size;
    lookup_req->req = READ_SHARED;
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
                    handleIdleReq();
                    state = LOOKUP;
                }
                break;
            }
            case LOOKUP: {
                if (!_match) {
                    lookup_req->id[1] = current_id;
                    if (!req_clear_wait) {
                        bool success = parent->lookup(callback_id, lookup_req);
                        if (success) {
                            replace_way = replace->get(lookup_set);
                            state = MISS;
                            current_id++;
                        }
                    }
                } else if (idle_req_valid) {
                    handleIdleReq();
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

void ICache::flush(uint64_t addr, uint32_t asid) {
    flush_set = 0;
    flush_num = set_size;
    flush_valid = true;
}

void ICache::redirect() {
    idle_req_valid = false;
    if (state == MISS || state == REFILL) {
        req_clear_wait = true;
    }
    state = IDLE;
}

void ICache::handleIdleReq() {
    lookup_req->addr = idle_req->addr;
    lookup_req->id[0] = idle_req->id[0];
    idle_req_valid = false;
    uint32_t offset;
    splitAddr(lookup_req->addr, lookup_tag, lookup_set, offset);
    CacheTagv* tagv = match(lookup_tag, lookup_set);
    _match = tagv != nullptr;
    if (_match) {
        callbacks[0](lookup_req->id, nullptr);
    }
}
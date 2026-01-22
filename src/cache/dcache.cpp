#include "cache/dcache.h"

void DCache::afterLoad() {
    callback_id = parent->setCallback(std::bind(&DCache::callbackFunc, this, std::placeholders::_1, std::placeholders::_2));
    lookup_req = new CacheReq;
    lookup_req->id[1] = 0;
    lookup_req->size = line_size;
    Cache::afterLoad();
}

bool DCache::lookup(int callback_id, CacheReq* req) {
    if (!flush_valid && (state == IDLE || (state == LOOKUP && _match && lookup_req->req != WRITE_BACK))) {
        idle_req = req;
        idle_req_valid = true;
        return true;
    }
    return false;
}

void DCache::tick() {
    if (flush_valid) {
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
                    state = LOOKUP;
                    handleIdleReq();
                }
                break;
            }
            case LOOKUP: {
                if (_match) {
                    callbacks[0](lookup_req->id, nullptr);
                }
                if (!_match) {
                    if (!req_clear_wait) {
                        bool success = parent->lookup(callback_id, lookup_req);
                        if (success) {
                            replace_way = replace->get(lookup_set);
                            state = MISS;
                        }
                    }
                } else if (lookup_req->req == WRITE_BACK) {
                    state = WRITE;
                    lookup_tagv->dirty = true;
                } else if (idle_req_valid) {
                    handleIdleReq();
                } else {
                    state = IDLE;
                }
                break;
            }
            case WRITE: {
                state = IDLE;
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

void DCache::flush(uint64_t addr, uint32_t asid) {
    flush_set = 0;
    flush_num = set_size;
    flush_valid = true;
}

void DCache::redirect() {
    idle_req_valid = false;
    if (state == MISS || state == REFILL) {
        req_clear_wait = true;
    }
    state = IDLE;
}

void DCache::callbackFunc(uint16_t* ids, CacheTagv* tagv_i) {
    CacheTagv* tagv = tagvs[lookup_set][replace_way];
    tagv->tag = lookup_tag;
    tagv->valid = true;
    tagv->dirty = lookup_req->req == WRITE_BACK;
    if (req_clear_wait) {
        req_clear_wait = false;
    } else {
        callbacks[0](lookup_req->id, nullptr);
        state = REFILL;
    }
}

void DCache::handleIdleReq() {
    lookup_req->addr = idle_req->addr;
    lookup_req->id[0] = idle_req->id[0];
    lookup_req->req = idle_req->req;
    idle_req_valid = false;
    uint32_t offset;
    splitAddr(lookup_req->addr, lookup_tag, lookup_set, offset);
    lookup_tagv = match(lookup_tag, lookup_set);
    _match = lookup_tagv != nullptr;
}
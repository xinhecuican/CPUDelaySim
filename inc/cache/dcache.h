#ifndef CACHE_DCACHE_H
#define CACHE_DCACHE_H
#include "cache/cache.h"

class DCache : public Cache {
public:
    bool lookup(int callback_id, CacheReq* req) override;
    void afterLoad() override;
    void tick() override;
    void load() override;
    void flush(uint64_t addr, uint32_t asid) override;
    void redirect() override;

private:
    void callbackFunc(uint16_t* ids, CacheTagv* tagv_i);
    void handleIdleReq();

    enum state_t {
        IDLE,
        LOOKUP,
        WRITE,
        MISS,
        REFILL
    };

    state_t state = IDLE;
    CacheReq* idle_req;
    CacheReq* lookup_req;
    
    bool idle_req_valid = false;
    bool _match = false;
    bool flush_valid = false;
    int flush_num;
    int flush_set;
    bool req_clear_wait = false;
    uint32_t lookup_set;
    uint64_t lookup_tag;
    uint32_t replace_way;
    CacheTagv* lookup_tagv;
};

#endif
#ifndef ICACHE_H
#define ICACHE_H
#include "cache.h"

class ICache : public Cache {
public:
    ~ICache();
    bool lookup(int callback_id, CacheReq* req) override;
    void afterLoad() override;
    void tick() override;
    void load() override;
    void flush(uint64_t addr, uint32_t asid) override;
    void redirect() override;

private:
    void handleIdleReq();

private:
    typedef enum {
        IDLE,
        LOOKUP,
        MISS,
        REFILL
    } state_t;

    bool lookup_valid = false;
    bool _match = false;
    uint32_t lookup_set;
    uint64_t lookup_tag;
    int replace_way;
    CacheReq* idle_req;
    CacheReq* lookup_req;
    bool idle_req_valid;
    state_t state = IDLE;
    bool req_clear_wait = false; 
    uint16_t current_id = 0;

    CacheReq req_parent;

    bool flush_valid = false;
    uint16_t flush_num;
    uint32_t flush_set;
    bool in_callback = false;
};

REGISTER_CLASS(ICache)

#endif
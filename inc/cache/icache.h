#ifndef ICACHE_H
#define ICACHE_H
#include "cache.h"

class ICache : public Cache {
public:
    bool lookup(int id, snoop_req_t req, uint64_t addr, uint32_t size) override;
    void setParentCallback() override;
    void redirect(FetchStream* stream);
    void load() override;

private:
    void callbackFunc(int id, CacheTagv* tagv_i);

private:
    bool lookup_valid = false;
    bool _match = false;
    int lookup_id;
    uint32_t lookup_offset;
    uint32_t lookup_size;
    uint64_t lookup_addr;
    uint32_t lookup_set;
    int replace_way;
    snoop_req_t lookup_req;
};

REGISTER_CLASS(ICache)

#endif
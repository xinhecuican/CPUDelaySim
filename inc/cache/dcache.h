#ifndef CACHE_DCACHE_H
#define CACHE_DCACHE_H
#include "cache/cache.h"

class DCache : public Cache {
public:
    bool lookup(int callback_id, CacheReq* req) override;
    void afterLoad() override;
    void tick() override;
    void load() override;

private:

};

REGISTER_CLASS(DCache)

#endif
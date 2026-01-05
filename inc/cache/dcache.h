#ifndef DCACHE_H
#define DCACHE_H
#include "cache.h"

class DCache : public Cache {
public:
    ~DCache();
    bool lookup(int id, snoop_req_t req, uint64_t addr, uint32_t size) override;
    void setParentCallback() override;
    void load() override;
    void afterLoad() override;

private:
    /**
     * @ingroup config
     * @brief MSHR size
     */
    int mshr_size = 8;

    struct LookupEntry {
        int id;
        CacheTagv* tagv;
        snoop_req_t req;
        uint64_t addr;
        uint32_t size;
        void* replace_cacheline;
    };
    LookupEntry lookup_entry;
    LookupEntry* mshr_entrys;
};

REGISTER_CLASS(DCache)


#endif

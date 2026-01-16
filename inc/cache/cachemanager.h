#ifndef CACHEMANAGER_H
#define CACHEMANAGER_H
#include "cache/cache.h"
#include "cache/memory.h"
#include "device/irqhandler.h"

class CacheManager : public Base {
public:
    static CacheManager& getInstance() {
        static CacheManager instance;  // C++11保证线程安全
        return instance;
    }
    CacheManager(const CacheManager&) = delete;
    CacheManager& operator=(const CacheManager&) = delete;
    void load() override;
    void afterLoad() override;
    IrqHandler* getIrqHandler() { return irq_handler; }
    Cache* getCache(int id);
    Cache* getICache();
    Cache* getDCache();
    Memory* memory;
    std::vector<Device*> devices;    
    std::map<int, Cache*> cache_map;
private:
    int icache_id;
    int dcache_id;

    CacheManager() = default;
    ~CacheManager() = default;
    IrqHandler* irq_handler = nullptr;
};


#endif

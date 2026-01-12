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
    std::vector<Cache*> l1_caches;
    std::vector<Cache*> l2_caches;
    std::vector<Cache*> l3_caches;
    Memory* memory;
    std::vector<Device*> devices;
private:
    CacheManager() = default;
    ~CacheManager() = default;
    IrqHandler* irq_handler = nullptr;
};


#endif

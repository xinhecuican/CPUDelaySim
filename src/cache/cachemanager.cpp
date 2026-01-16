#include "cache/cachemanager.h"
#include "device/irqhandler.h"
#include "common/log.h"

void CacheManager::afterLoad() {
    for (auto cache : cache_map) {
        cache.second->afterLoad();
    }
    for (auto device : devices) {
        if (dynamic_cast<IrqHandler*>(device) != nullptr) {
            irq_handler = dynamic_cast<IrqHandler*>(device);
            break;
        }
    }
    if (irq_handler != nullptr) {
        for (auto device : devices) {
            device->setIrqHandler(irq_handler);
        }
    } else {
        Log::error("No irq handler found, some devices may not work properly");
    }

    memory->setDevices(devices);
    memory->afterLoad();
}

Cache* CacheManager::getICache() {
    return cache_map[icache_id];
}

Cache* CacheManager::getDCache() {
    return cache_map[dcache_id];
}

Cache* CacheManager::getCache(int id) {
    return cache_map[id];
}
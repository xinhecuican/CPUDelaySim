#include "cache/cachemanager.h"
#include "device/irqhandler.h"
#include "common/log.h"

void CacheManager::afterLoad() {
    if (l3_caches.size() > 0) {
        for (const auto& cache : l2_caches) {
            cache->setParent(l3_caches[0]);
            cache->setParentCallback();
        }
        l3_caches[0]->setParent(memory);
    } else if (l2_caches.size() > 0) {
        for (const auto& cache : l1_caches) {
            cache->setParent(l2_caches[0]);
            cache->setParentCallback();
        }
        l2_caches[0]->setParent(memory);
    } else if (l1_caches.size() > 0) {
        for (const auto& cache : l1_caches) {
            cache->setParent(memory);
            cache->setParentCallback();
        }
    }
    for (auto cache : l1_caches) {
        cache->afterLoad();
    }
    for (auto cache : l2_caches) {
        cache->afterLoad();
    }
    for (auto cache : l3_caches) {
        cache->afterLoad();
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
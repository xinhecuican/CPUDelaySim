#include "cache/cachemanager.h"

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
    memory->setDevices(devices);
    memory->afterLoad();
}
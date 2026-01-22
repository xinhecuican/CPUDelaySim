#include "pred/history/history.h"

void HistoryManager::afterLoad() {
    for (auto history : histories) {
        history->afterLoad();
        meta_size += history->getMetaSize();
    }
}

int HistoryManager::getMetaSize() {
    return meta_size;
}

void HistoryManager::getMeta(void* meta) {
    int currentMetaSize = 0;
    for (auto history : histories) {
        history->getMeta((uint8_t*)meta + currentMetaSize);
        currentMetaSize += history->getMetaSize();
    }
}

void HistoryManager::update(bool speculative, bool real_taken, uint64_t real_pc, InstType type, void* meta) {
    int currentMetaSize = 0;
    for (int i = 0; i < histories.size(); i++) {
        histories[i]->update(speculative, real_taken, real_pc, type, (uint8_t*)meta + currentMetaSize);
        currentMetaSize += histories[i]->getMetaSize();
    }
}
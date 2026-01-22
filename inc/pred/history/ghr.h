#ifndef PRED_HISTORY_GHR_H
#define PRED_HISTORY_GHR_H
#include "pred/history/history.h"

class GHR : public History {
public:
    void update(bool speculative, bool real_taken, uint64_t real_pc, InstType type, void* meta) override;
    void load() override;
    void afterLoad() override;
    bool getLatest();
    bool get(uint32_t idx);
    uint64_t getLatestBlock();
    int getMetaSize() override;
    void getMeta(void* meta) override;

private:
    struct GHRMeta {
        uint32_t idx;
    };
    uint32_t ghr_size = 1024;

    boost::dynamic_bitset<> ghr;
    uint32_t ghr_idx = 0;
};

#endif
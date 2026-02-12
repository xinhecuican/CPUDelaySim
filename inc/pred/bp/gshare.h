#ifndef PRED_GSHARE_GSHARE_H
#define PRED_GSHARE_GSHARE_H
#include "pred/bp/bp.h"
#include "pred/history/ghr.h"

class GShareBP : public BP {
public:
    ~GShareBP();
    void load() override;
    void afterLoad() override;
    void predict(BranchStream* stream, void* meta) override;
    void update(bool real_taken, uint64_t pc, int size, uint64_t target, InstType type, void* meta_info) override;
    int getMetaSize() override;

private:
    struct GShareMeta {
        uint32_t idx;
        uint32_t ghr;
    };
    int table_size = 64;
    int bit_size = 2;
    int offset = 2;
    int hist_bits = 6;

    int8_t* table;
    uint64_t table_pc_mask;
    int min_size;
    int max_size;
    uint32_t hist_mask;
    uint32_t counter_mask;
    GHR* ghr;
};

#endif
#ifndef PRED_BP_RAS_H
#define PRED_BP_RAS_H
#include "pred/bp/bp.h"

class RAS : public BP {
public:
    ~RAS();
    void load() override;
    void afterLoad() override;
    void predict(BranchStream* stream, void* meta) override;
    void redirect(bool real_taken, uint64_t pc, int size, uint64_t target, InstType type, void* meta_info) override;
    int getMetaSize() override;

private:
    struct RASMeta {
        uint32_t top;
    };
    int size;

    uint64_t* ras;
    uint32_t top;
};

#endif
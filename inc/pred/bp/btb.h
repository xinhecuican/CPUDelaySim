#ifndef PRED_BP_BTB_H
#define PRED_BP_BTB_H

#include "pred/bp/bp.h"

struct BTBEntry {
    bool valid;
    InstType type;
    uint64_t tag;
    uint64_t target;
};

class BTB : public BP {
public:
    ~BTB();
    void load() override;
    void afterLoad() override;
    void predict(BranchStream* stream, void* meta) override;
    void update(bool real_taken, uint64_t pc, uint64_t target, InstType type, void* meta) override;
    int getMetaSize() override;
    
private:

    void getIndexTag(uint64_t pc, uint64_t& tag, int& index);

    struct BTBMeta {
        uint64_t tag;
        int index;
        bool hit;
    };

    int table_size;
    int tag_size;
    int offset;

    BTBEntry** table;
    int index_size;
    uint64_t tag_mask;
    uint64_t index_mask;
    int tag_shift;
};


#endif
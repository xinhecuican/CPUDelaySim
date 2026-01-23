#ifndef PRED_BP_BP_H
#define PRED_BP_BP_H
#include "common/base.h"
#include "pred/history/history.h"

struct BranchStream {
    InstType type;
    bool taken;
    bool indv;
    bool rasv;
    int size;
    uint64_t pc;
    uint64_t target;
    uint64_t ind_target;
    uint64_t ras_target;
};

class BP : public Base {
public:
    virtual ~BP() = default;
    virtual int getDelay() { return delay; }
    /**
     * @brief Predict the next instruction.
     * 
     * @param stream The branch stream.
     * @param meta The meta info, used in update.
     */
    virtual void predict(BranchStream* stream, void* meta) = 0;
    virtual void update(bool real_taken, uint64_t pc, uint64_t target, InstType type, void* meta) = 0;
    virtual int getMetaSize() = 0;
    virtual void setHistoryManager(HistoryManager* history_manager) { this->history_manager = history_manager; }

protected:
    int delay = 0;

    HistoryManager* history_manager;
};

#endif
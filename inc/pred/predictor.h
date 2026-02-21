#ifndef PRED_PREDICTOR_H
#define PRED_PREDICTOR_H

#include "common/base.h"
#include "pred/history/history.h"
#include "pred/bp/bp.h"

class Predictor : public Base {
public:
    virtual ~Predictor();
    virtual void load() override;
    virtual void afterLoad() override;
    
    /**
     * @brief Predict the next instruction.
     * 
     * @param pc The current program counter.
     * @param info The predecode info.
     * @param next_pc The predicted next program counter.
     * @param size The size of the predicted instruction.
     * @param taken Whether the branch is taken.
     * @param stall Whether the pipeline is stalled.
     * @return int The meta index
    
    */
    virtual int predict(uint64_t pc, DecodeInfo* info, uint64_t& next_pc, uint8_t& size, bool& taken, bool stall);
    virtual void redirect(bool real_taken, uint64_t pc, int size, uint64_t target, InstType type, int meta_idx);
    virtual void update(bool real_taken, uint64_t pc, int size, uint64_t target, InstType type, int meta_idx);

protected:
    int retire_size;
    HistoryManager* history_manager;
    std::vector<BP*> bps;

    struct MetaInfo {
        void** meta;
        void* history_meta;
        BranchStream* stream;
        uint64_t pred_addr;
        bool taken;
    };
    MetaInfo** metas;
    int meta_idx = 0;
    int bubble = 0;
    bool pred_valid = false;
    int max_delay = 0;
    BP*** bp_layers;
    int* bp_layers_size;
    int bps_size;

    uint64_t predTimes = 0;
    uint64_t condPredTimes = 0;
    uint64_t indirectPredTimes = 0;
    uint64_t callPredTimes = 0;

    uint64_t condErrorTimes = 0;
    uint64_t indirectErrorTimes = 0;
    uint64_t callErrorTimes = 0;
};

#endif
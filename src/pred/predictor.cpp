#include "pred/predictor.h"
#include "common/log.h"
#include "config.h"

Predictor::~Predictor() {
    for (int i = 0; i < retire_size; i++) {
        for (int j = 0; j < bps.size(); j++) {
            free(metas[i]->meta[j]);
        }
        free(metas[i]->stream);
        free(metas[i]->history_meta);
        free(metas[i]->meta);
        free(metas[i]);
    }
    for (int i = 0; i <= max_delay; i++) {
        free(bp_layers[i]);
    }
    free(bp_layers);
    free(bp_layers_size);
    free(metas);
}

void Predictor::afterLoad() {
    history_manager->afterLoad();
    for (auto& bp : bps) {
        bp->setHistoryManager(history_manager);
        bp->afterLoad();
    }

    metas = (MetaInfo**)malloc(retire_size * sizeof(MetaInfo*));
    for (int i = 0; i < retire_size; i++) {
        metas[i] = (MetaInfo*)malloc(sizeof(MetaInfo));
        metas[i]->meta = (void**)malloc(bps.size() * sizeof(void*));
        metas[i]->history_meta = malloc(history_manager->getMetaSize());
        for (int j = 0; j < bps.size(); j++) {
            metas[i]->meta[j] = malloc(bps[j]->getMetaSize());
        }
        metas[i]->stream = (BranchStream*)malloc(sizeof(BranchStream));
    }

    for (auto& bp : bps) {
        max_delay = std::max(max_delay, bp->getDelay());
    }
    bp_layers = (BP***)malloc((max_delay + 1) * sizeof(BP**));
    bp_layers_size = (int*)malloc((max_delay + 1) * sizeof(int));
    memset(bp_layers_size, 0, (max_delay + 1) * sizeof(int));
    for (int i = 0; i <= max_delay; i++) {
        bp_layers[i] = (BP**)malloc(bps.size() * sizeof(BP*));
    }
    int pre_delay = 0;
    for (int i = 0; i < bps.size(); i++) {
        bp_layers[bps[i]->getDelay()][bp_layers_size[bps[i]->getDelay()]++] = bps[i];
        assert(bps[i]->getDelay() == pre_delay || bps[i]->getDelay() == pre_delay + 1);
        pre_delay = bps[i]->getDelay();
    }
    bps_size = bps.size();

#ifdef LOG_PRED
    Log::init("pred", Config::getLogFilePath("pred.log"));
#endif
    Stats::registerStat(&predTimes, "predTimes", "Prediction times");
    Stats::registerStat(&condPredTimes, "condPredTimes", "Conditional branch prediction times");
    Stats::registerStat(&indirectPredTimes, "indirectPredTimes", "Indirect branch prediction times");
    Stats::registerStat(&callPredTimes, "callPredTimes", "Call/ret branch prediction times");

    Stats::registerStat(&condErrorTimes, "condErrorTimes", "Conditional branch error times");
    Stats::registerStat(&indirectErrorTimes, "indirectErrorTimes", "Indirect branch error times");
    Stats::registerStat(&callErrorTimes, "callErrorTimes", "Call/ret branch error times");

    Stats::registerRatio(&condErrorTimes, &condPredTimes, "condErrorRatio", "Conditional branch error ratio");
    Stats::registerRatio(&indirectErrorTimes, &indirectPredTimes, "indirectErrorRatio", "Indirect branch error ratio");
    Stats::registerRatio(&callErrorTimes, &callPredTimes, "callErrorRatio", "Call/ret branch error ratio");
}

int Predictor::predict(uint64_t pc, DecodeInfo* info, uint64_t& next_pc, uint8_t& size, bool& taken, bool stall) {
    if (bubble != 0) {
        bubble--;
    } else if (!pred_valid) {
        BranchStream* stream = (BranchStream*)metas[meta_idx]->stream;
        stream->pc = pc;
        stream->type = INT;
        stream->taken = false;
        stream->indv = false;
        stream->rasv = false;
        history_manager->getMeta(metas[meta_idx]->history_meta);
        metas[meta_idx]->pred_addr = 0xdeadbeefdeadbeef;
        int idx = 0;
        for (int i = 0; i <= max_delay; i++) {
            uint64_t pre_pc = metas[meta_idx]->pred_addr;
            for (int j = 0; j < bp_layers_size[i]; j++) {
                bp_layers[i][j]->predict(stream, metas[meta_idx]->meta[idx]);
                idx++;
            }
            switch (stream->type) {
                case COND: {
                    if (stream->taken) metas[meta_idx]->pred_addr = stream->target; 
                    metas[meta_idx]->taken = stream->taken;
                    break;
                }
                case DIRECT:
                case PUSH:
                    metas[meta_idx]->pred_addr = stream->target;
                    metas[meta_idx]->taken = true;
                    break;
                case IND_CALL:
                case IND_PUSH:
                case INDIRECT: {
                    if (stream->indv) metas[meta_idx]->pred_addr = stream->ind_target;
                    else metas[meta_idx]->pred_addr = stream->target;
                    metas[meta_idx]->taken = true;
                    break;
                }
                case POP:
                case POP_PUSH: {
                    if (stream->rasv) metas[meta_idx]->pred_addr = stream->ras_target;
                    else metas[meta_idx]->pred_addr = stream->target;
                    metas[meta_idx]->taken = true;
                    break;
                }
                default:  {
                    metas[meta_idx]->taken = false;
                    break;
                }
            }
            if (pre_pc != metas[meta_idx]->pred_addr) bubble = i;
        }
        if (metas[meta_idx]->pred_addr == 0xdeadbeefdeadbeef) {
            metas[meta_idx]->pred_addr = pc + stream->size;
        }
        history_manager->update(true, stream->taken, metas[meta_idx]->pred_addr, stream->type, metas[meta_idx]->history_meta);
    }

    pred_valid = bubble == 0;

    if (!stall && pred_valid) {
        next_pc = metas[meta_idx]->pred_addr;
        size = metas[meta_idx]->stream->size;
        int res = meta_idx;
        meta_idx = (meta_idx + 1) % retire_size;
        pred_valid = false;
        taken = metas[meta_idx]->taken;
        
#ifdef LOG_PRED
        if (taken) Log::trace("pred", "pred 0x{:x} 0x{:x} {} {}", 
                    metas[meta_idx]->stream->pc, next_pc, size, (uint8_t)metas[meta_idx]->stream->type);
#endif
        return res;
    }
    return -1;
}

void Predictor::redirect(bool real_taken, uint64_t real_pc, int size, uint64_t target, InstType type, int meta_idx) {
#ifdef LOG_PRED
    Log::trace("pred", "redirect 0x{:x} 0x{:x} {} {}", real_pc, target, size, (uint8_t)type);
#endif
    history_manager->update(false, real_taken, real_pc, type, metas[meta_idx]->history_meta);
    for (int i = 0; i < bps_size; i++) {
        bps[i]->redirect(real_taken, real_pc, size, target, type, metas[meta_idx]->meta[i]);
    }
    bubble = 0;
    pred_valid = false;
}

void Predictor::update(bool real_taken, uint64_t pc, int size, uint64_t target, InstType type, int meta_idx) {
    for (int i = 0; i < bps_size; i++) {
        bps[i]->update(real_taken, pc, size, target, type, metas[meta_idx]->meta[i]);
    }
    switch (type) {
        case COND:
            condPredTimes++;
            condErrorTimes += real_taken != metas[meta_idx]->taken;
            break;
        case INDIRECT:
        case IND_CALL:
        case IND_PUSH:
            indirectPredTimes++;
            indirectErrorTimes += metas[meta_idx]->pred_addr != target;
            break;
        case POP:
        case POP_PUSH:
            callPredTimes++;
            callErrorTimes += metas[meta_idx]->pred_addr != target;
            break;
        default:
            break;
    }
    predTimes++;
}

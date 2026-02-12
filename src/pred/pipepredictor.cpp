#include "pred/pipepredictor.h"
#include "common/log.h"

int PipePredictor::predict(uint64_t pc, DecodeInfo* info, uint64_t& next_pc, uint8_t& size, bool& taken, bool stall) {
    BranchStream* stream = (BranchStream*)metas[meta_idx]->stream;
    stream->pc = pc;
    stream->type = info->type;
    stream->taken = false;
    stream->indv = false;
    stream->rasv = false;
    history_manager->getMeta(metas[meta_idx]->history_meta);
    metas[meta_idx]->pred_addr = pc + info->inst_size;
    for (int i = 0; i < bps_size; i++) {
        bps[i]->predict(stream, metas[meta_idx]->meta[i]);
    }
    switch (info->type) {
        case COND:
            if (stream->taken) metas[meta_idx]->pred_addr = info->dst_data[2]; 
            metas[meta_idx]->taken = stream->taken;
            break;
        case DIRECT:
        case PUSH:
            metas[meta_idx]->pred_addr = info->dst_data[2];
            metas[meta_idx]->taken = true;
            break;
        case IND_CALL:
        case IND_PUSH:
        case INDIRECT:
            if (stream->indv) metas[meta_idx]->pred_addr = stream->ind_target;
            else metas[meta_idx]->pred_addr = stream->target;
            metas[meta_idx]->taken = true;
            break;
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
    history_manager->update(true, stream->taken, metas[meta_idx]->pred_addr, info->type, metas[meta_idx]->history_meta);
    next_pc = metas[meta_idx]->pred_addr;
    size = stream->size;
    taken = metas[meta_idx]->taken;
    int res = meta_idx;

#ifdef LOG_PRED
    if (info->type >= BRANCH_START && info->type <= BRANCH_END)
        Log::trace("pred", "pred 0x{:x} 0x{:x} {} {}", 
                    metas[meta_idx]->stream->pc, next_pc, taken, (uint8_t)metas[meta_idx]->stream->type);
#endif
    meta_idx = (meta_idx + 1) % retire_size;
    pred_valid = false;
    return res;
}
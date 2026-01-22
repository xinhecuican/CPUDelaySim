#include "pred/history/ghr.h"

void GHR::afterLoad() {
    ghr.resize(ghr_size);
}

void GHR::update(bool speculative, bool real_taken, uint64_t real_pc, InstType type, void* meta) {
    bool type_cond = (type == COND);
    if (!speculative) {
        GHRMeta* meta_info = (GHRMeta*)meta;
        int shift_size = ghr_idx - meta_info->idx;
        ghr_idx = meta_info->idx;
        ghr >>= shift_size;
        if (!type_cond) return;
    }
    if (type_cond) {
        ghr = ghr << 1;
        ghr.set(0, real_taken);
        ghr_idx++;
    }
}

bool GHR::getLatest() {
    return ghr[0];
}

bool GHR::get(uint32_t idx) {
    return ghr[idx];
}

int GHR::getMetaSize() {
    return sizeof(GHRMeta);
}

void GHR::getMeta(void* meta) {
    GHRMeta* meta_info = (GHRMeta*)meta;
    meta_info->idx = ghr_idx;
}

uint64_t GHR::getLatestBlock() {
    return ghr.to_ulong();
}

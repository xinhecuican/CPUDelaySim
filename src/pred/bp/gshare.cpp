#include "pred/bp/gshare.h"
#include <bit>

GShareBP::~GShareBP() {
    delete[] table;
}

void GShareBP::afterLoad() {
    ghr = history_manager->getHistory<GHR>();
    assert(ghr != nullptr);
    table = new int8_t[table_size];
    memset(table, 0, table_size);
    table_pc_mask = (1 << clog2(table_size)) - 1;
    bit_mask = (1 << bit_size) - 1;
    min_size = (-bit_mask) - 1;
}

void GShareBP::predict(BranchStream* stream, void* meta) {
    if (stream->type == COND) {
        GShareMeta* meta_info = (GShareMeta*)meta;
        uint32_t ghist = ghr->getLatestBlock();
        meta_info->idx = (((stream->pc & table_pc_mask) >> offset) ^ ghist) & table_pc_mask;
        meta_info->ghr = ghist;
        bool taken = table[meta_info->idx] >= 0;
        bool change = stream->taken ^ taken;
        stream->taken = taken;
    }
}

void GShareBP::update(bool real_taken, uint64_t pc, uint64_t target, InstType type, void* meta_info) {
    if (type == COND) {
        GShareMeta* meta_info = (GShareMeta*)meta_info;
        uint32_t ghist = meta_info->ghr;
        uint32_t idx = (((pc & table_pc_mask) >> offset) ^ ghist) & table_pc_mask;
        table[idx] = updateCounter(table[idx], real_taken, bit_mask, min_size);
    }
}

int GShareBP::getMetaSize() {
    return sizeof(GShareMeta);
}
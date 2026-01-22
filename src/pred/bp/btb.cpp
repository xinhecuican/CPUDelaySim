#include "pred/bp/btb.h"

BTB::~BTB() {
    delete[] table;
}

void BTB::afterLoad() {
    table = new BTBEntry[table_size];
    memset(table, 0, table_size * sizeof(BTBEntry));
    tag_mask = (1 << tag_size) - 1;
    index_size = clog2(table_size);
    tag_shift = offset + index_size;
    index_mask = (1 << index_size) - 1;
}

void BTB::predict(BranchStream* stream, void* meta) {
    BTBMeta* meta_info = (BTBMeta*)meta;
    getIndexTag(stream->pc, meta_info->tag, meta_info->index);
    meta_info->hit = table[meta_info->index].valid && table[meta_info->index].tag == meta_info->tag;
    if (meta_info->hit) {
        stream->type = table[meta_info->index].type;
        stream->target = table[meta_info->index].target;
    }
}

void BTB::getIndexTag(uint64_t pc, uint64_t& tag, int& index) {
    tag = (pc >> tag_shift) & tag_mask;
    index = (pc >> offset) & index_mask;
}

void BTB::update(bool real_taken, uint64_t pc, uint64_t target, InstType type, void* meta_info) {
    if (type >= BRANCH_START && type <= BRANCH_END) {
        uint64_t tag;
        int index;
        getIndexTag(pc, tag, index);
        table[index].valid = true;
        table[index].tag = tag;
        table[index].target = target;
        table[index].type = type;
    }
}

int BTB::getMetaSize() {
    return sizeof(BTBEntry);
}

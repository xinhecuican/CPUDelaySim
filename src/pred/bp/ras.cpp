#include "pred/bp/ras.h"

RAS::~RAS() {
    delete[] ras;
}

void RAS::afterLoad() {
    ras = new uint64_t[size];
    top = 0;
}

void RAS::predict(BranchStream* stream, void* meta) {
    bool pop_push = stream->type == POP_PUSH;
    bool push = stream->type == PUSH || stream->type == IND_PUSH;
    bool pop = stream->type == POP;
    RASMeta* meta_info = (RASMeta*)meta;
    meta_info->top = top;
    stream->rasv = true;

    if (pop_push) {
        int topIdx = top == 0 ? size - 1 : top - 1;
        stream->ras_target = ras[topIdx];
        ras[topIdx] = stream->pc + size;
    } else if (push) {
        ras[top] = stream->pc + size;
        top = (top + 1) % size;
    } else if (pop) {
        top = top == 0 ? size - 1 : top - 1;
        stream->ras_target = ras[top];
    }

}

void RAS::redirect(bool real_taken, uint64_t pc, int size, uint64_t target, InstType type, void* meta_info) {
    bool pop_push = type == POP_PUSH;
    bool push = type == PUSH || type == IND_PUSH;
    bool pop = type == POP;
    RASMeta* meta = (RASMeta*)meta_info;
    top = meta->top;
    if (pop_push) {
        int topIdx = top == 0 ? size - 1 : top - 1;
        ras[topIdx] = pc + size;
    } else if (push) {
        ras[top] = pc + size;
        top = (top + 1) % size;
    } else if (pop) {
        top = top == 0 ? size - 1 : top - 1;
    }
}

int RAS::getMetaSize() {
    return sizeof(RASMeta);
}
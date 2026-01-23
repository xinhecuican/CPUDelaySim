#ifndef COMMON_BUNDLES_H_
#define COMMON_BUNDLES_H_
#include <cstdint>
#include <cstdio>
#include <cmath>
#include <vector>
#include <map>
#include <list>
#include <queue>
#include <unordered_map>
#include <string>
#include <functional>
// #include <json/json.h>
#include <boost/dynamic_bitset.hpp>
#include <boost/circular_buffer.hpp>
#include <spdlog/spdlog.h>
#include "common/reflect.h"
#include "common/defines.h"

#define likely(x)    __builtin_expect(!!(x), 1)
#define unlikely(x)  __builtin_expect(!!(x), 0)
#define prefetch(x)  __builtin_prefetch(x)

#define packed __attribute__((__packed__))


enum RetType {
    RET_SUCCESS = 0,
    RET_STALL = 1
};

enum packed InstType {
    INT = 0,
    BRANCH_START = 1,
    COND = 1,
    DIRECT = 2,
    PUSH = 3,
    INDIRECT = 4,
    IND_CALL = 5,
    IND_PUSH = 6,
    POP = 7,
    POP_PUSH = 8,
    BRANCH_END = 8,
    MEM_START = 9,
    LOAD = 9,
    STORE = 10,
    LR = 11,
    SC = 12,
    AMO = 13,
    MEM_END = 13,
    MULT = 14,
    DIV = 15,
    CSRWR = 16,
    SRET = 17,
    MRET = 18,
    FENCE = 19,
    IFENCE = 20,
    SFENCE = 21,
    FMISC_SIMPLE = 22,
    FMISC_COMPLEX = 23,
    FADD = 24,
    FMUL = 25,
    FMA = 26,
    FDIV = 27,
    FSQRT = 28,
    TYPE_NUM = 29
};

struct FetchStream {
    uint64_t tick;
    uint64_t pc;
    uint64_t target;
    void* meta;
    uint16_t size;
    bool taken;
    InstType type;
};

enum packed RedirectReason {
    RED_FRONT = 0,
    RED_BP = 1,
    RED_LSU = 2,
    RED_EXC = 3
};

struct DRAMMeta {
    uint8_t callback_id;
    uint16_t id[4];
    uint64_t addr;
    uint32_t size;
};

#define DSTF_REG_MASK 0x20
#define IRQ_MASK 0x8000000000000000
#define EXC_MASK 0x7FFFFFFFFFFFFFFF

struct FetchInfo {
    uint64_t pc;
    uint16_t size;
    void* meta;
};

struct DecodeInfo {
    uint64_t dst_mask[3];
    uint8_t src_reg[3];
    uint8_t dst_reg; // 解码出的目的寄存器
    uint64_t exception;
    uint32_t inst;
    uint64_t exc_data;
    uint64_t dst_data[3];
    uint16_t dst_idx[3]; // 在ArchState中的偏置
    uint8_t inst_size;
    InstType type;
};

struct FrontendStream {
    FetchInfo* stream;
    DecodeInfo* info;
    FrontendStream* next;
};

struct RenameInfo {
    uint8_t src_preg[3];
    uint8_t dst_preg;
    bool    src_vlds[3];
    bool    dst_vld;
};

struct BackendStream {
    FrontendStream* stream;
    RenameInfo* rename_info;
    BackendStream* next;
};

static constexpr size_t DEC_MEMSET_END = offsetof(DecodeInfo, inst);

static int clog2(int x) {
    return std::ceil(std::log2(x));
}

static int updateCounter(int old, bool taken, uint32_t mask, int min) {
    uint32_t offset = uint32_t(old - min);
    int taken_val = (taken << 1) - 1;
    offset = (offset + taken_val) & mask;
    return int(offset) + min;
}

#endif // COMMON_BUNDLES_H_
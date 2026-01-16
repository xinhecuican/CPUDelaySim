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
    DIRECT = 1,
    COND = 2,
    INDIRECT = 3,
    IND_CALL = 4,
    PUSH = 5,
    POP = 6,
    POP_PUSH = 7,
    BRANCH_END = 7,
    LOAD = 8,
    STORE = 9,
    LR = 10,
    SC = 11,
    AMO = 12,
    MULT = 13,
    DIV = 14,
    CSRWR = 15,
    SRET = 16,
    MRET = 17,
    FENCE = 18,
    IFENCE = 19,
    SFENCE = 20,
    FMISC_SIMPLE = 21,
    FMISC_COMPLEX = 22,
    FADD = 23,
    FMUL = 24,
    FMA = 25,
    FDIV = 26,
    FSQRT = 27,
    TYPE_NUM = 28
};

struct BTBEntry {
    uint64_t tag;
    uint64_t target;
    InstType type;
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
#define IRQ_MASK 0x80000000
#define EXC_MASK 0x7FFFFFFF

struct FetchInfo {
    uint64_t pc;
    uint16_t size;
    void* meta;
};

struct DecodeInfo {
    uint64_t dst_mask[3];
    uint8_t src_reg[3];
    uint8_t dst_reg; // 解码出的目的寄存器
    uint32_t exception;
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

#endif // COMMON_BUNDLES_H_
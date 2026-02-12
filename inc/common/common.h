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
#include "common/stats.h"
#include "common/exithandler.h"

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

enum packed InstResult {
    NORMAL = 0,
    EXCEPTION = 1,
    INTERRUPT = 2,
    PRED_FAIL = 3,
    MEM_REDIRECT = 4,
    CSR_REDIRECT = 5,
    RESULT_NUM = 6
};

extern std::string InstTypeName[];
extern std::string InstResultName[];

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

/**
 * update saturating counter
 * @param old old value of the counter
 * @param taken whether the branch is taken
 * @param mask the mask for the counter, e.g. for 2-bit saturating counter, mask should be 0b111
 * @param min the minimum value of the counter, e.g. for 2-bit saturating counter, min should be -4
 * @param max the maximum value of the counter, e.g. for 2-bit saturating counter, max should be 3
 */
static int updateCounter(int old, bool taken, uint32_t mask, int min, int max) {
    int taken_val = (taken << 1) - 1;
    int new_val = old + taken_val;
    
    // Saturate to min if new_val is less than min
    int below_min = (new_val < min);
    new_val = (new_val & ~(-below_min)) | (min & (-below_min));
    
    // Saturate to max if new_val is greater than max
    int above_max = (new_val > max);
    new_val = (new_val & ~(-above_max)) | (max & (-above_max));
    
    return new_val;
}

#endif // COMMON_BUNDLES_H_
#include "common/base.h"
#include "common/common.h"

// Define static members of Base class
uint64_t* Base::tick = nullptr;
Arch* Base::arch = nullptr;

// Define InstTypeName array
std::string InstTypeName[] = {
    "INT",
    "COND",
    "DIRECT",
    "PUSH",
    "INDIRECT",
    "IND_CALL",
    "IND_PUSH",
    "POP",
    "POP_PUSH",
    "LOAD",
    "STORE",
    "LR",
    "SC",
    "AMO",
    "MULT",
    "DIV",
    "CSRWR",
    "SRET",
    "MRET",
    "FENCE",
    "IFENCE",
    "SFENCE",
    "FMISC_SIMPLE",
    "FMISC_COMPLEX",
    "FADD",
    "FMUL",
    "FMA",
    "FDIV",
    "FSQRT"
};

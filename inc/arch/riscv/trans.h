#ifndef TRANS_H
#define TRANS_H
#include "arch/riscv/archstate.h"

namespace cds::arch::riscv {
bool interpreter(DisasContext *ctx, uint32_t insn, bool rvc);
} // namespace cds::arch::riscv
#endif
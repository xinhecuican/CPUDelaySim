#ifndef CPU_ATOMICCPU_H_
#define CPU_ATOMICCPU_H_

#include "cpu/cpu.h"

class AtomicCPU : public CPU {
public:
    void load() override;
    void afterLoad() override;
    void exec() override;

private:
    uint64_t pc;
    uint64_t inst_count;
    DecodeInfo* info;
};

REGISTER_CLASS(AtomicCPU)


#endif
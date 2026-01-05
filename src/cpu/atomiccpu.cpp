#include "cpu/atomiccpu.h"

void AtomicCPU::afterLoad() {
    info = new DecodeInfo();
    pc = Base::arch->getStartPC();
    inst_count = 0;
    Base::arch->setInstret(&inst_count);
}

void AtomicCPU::exec() {
    int inst_size = Base::arch->decode(pc, info);
    pc = Base::arch->updateEnv();
    Base::upTick();
    inst_count++;
}
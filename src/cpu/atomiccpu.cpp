#include "cpu/atomiccpu.h"

AtomicCPU::~AtomicCPU() {
    delete info;
}

void AtomicCPU::afterLoad() {
    info = new DecodeInfo();
    pc = Base::arch->getStartPC();
    inst_count = 0;
    Base::arch->setInstret(&inst_count);
}

void AtomicCPU::exec() {
    uint64_t paddr;
    uint64_t exception = Base::arch->getExceptionNone();
    Base::arch->translateAddr(pc, FETCH_TYPE::IFETCH, paddr, exception);
    if (exception) {
        Base::arch->handleException(exception, pc, info);
    } else {
        int inst_size = Base::arch->decode(pc, paddr, info);
    }
    pc = Base::arch->updateEnv();
    Base::upTick();
    inst_count++;
}
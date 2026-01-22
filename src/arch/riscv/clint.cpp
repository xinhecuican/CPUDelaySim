#include "arch/riscv/clint.h"
#include "arch/riscv/archstate.h"
#include "device/irqhandler.h"

#define MSIP_OFFSET 0x0000
#define MTIME_OFFSET 0xbff8
#define MTIMECMP_OFFSET 0x4000

void Clint::afterLoad() {
    end_addr = base_addr + 0x10000;
    msip = 0;
    mtime = 0;
    mtimecmp = 0;
    irqValid = false;
}

void Clint::tick() {
    mtime++;
    if (mtime >= mtimecmp) {
        if (!irqValid) {
            irq_handler->setIrqState(EXCI_MTIMER, true);
            irqValid = true;
        }
    } else if (irqValid) {
        irq_handler->setIrqState(EXCI_MTIMER, false);
        irqValid = false;
    }
}

bool Clint::addRequest(DeviceReq* req) {
    if (this->req != nullptr) {
        return false;
    }
    this->req = req;
    return true;
}

DeviceReq* Clint::checkResponse() {
    if (this->req == nullptr) {
        return nullptr;
    }
    DeviceReq* req = this->req;
    this->req = nullptr;
    return req;
}

bool Clint::inRange(uint64_t addr) {
    return addr >= base_addr && addr < end_addr;
}

void Clint::read(uint64_t addr, int size, uint8_t* data) {
    uint64_t offset = addr - base_addr;
    if (offset == MSIP_OFFSET) {
        *(uint64_t*)data = msip;
    } else if (offset == MTIME_OFFSET) {
        *(uint64_t*)data = mtime;
    } else if (offset == MTIMECMP_OFFSET) {
        *(uint64_t*)data = mtimecmp;
    }
}

void Clint::write(uint64_t addr, int size, uint8_t* data) {
    uint64_t offset = addr - base_addr;
    if (offset == MSIP_OFFSET) {
        msip = (*(uint64_t*)data) & 1;
        irq_handler->setIrqState(EXCI_MSI, msip);
    } else if (offset == MTIME_OFFSET) {
        mtime = *(uint64_t*)data;
    } else if (offset == MTIMECMP_OFFSET) {
        mtimecmp = *(uint64_t*)data;
    }
}

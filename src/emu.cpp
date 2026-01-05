#include "emu.h"
#include "params_EMU.h"
#ifdef ARCH_RISCV
#include "arch/riscv/riscvarch.h"
#endif
#include "cache/cachemanager.h"
#include "common/log.h"

EMU::EMU() {
}

void EMU::init(const std::string& path) {
    cpu = ObjectFactory::createObject<CPU>(CPU_NAME);
#ifdef ARCH_RISCV
    arch = new cds::arch::riscv::RiscvArch();
#endif
    config.setup(path);
    Base::setArch(arch);
    Base::setTick(&tick);
    arch->setTick(&tick);
    CacheManager::getInstance().memory = new Memory(config.memory_path);
    CacheManager::getInstance().load();
    CacheManager::getInstance().memory->load();
    cpu->load();

    CacheManager::getInstance().afterLoad();
    arch->afterLoad();
    cpu->afterLoad();
}

void EMU::run() {
    while (Base::getTick() <= config.end_tick && arch->getInstret() < config.inst_count) {
        cpu->exec();
    }
}
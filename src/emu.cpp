#include "emu.h"
#include "params_EMU.h"
#ifdef ARCH_RISCV
#include "arch/riscv/riscvarch.h"
#endif
#include "cache/cachemanager.h"
#include "common/log.h"

EMU::EMU() {
}

EMU::~EMU() {
    spdlog::shutdown();
}

void EMU::init(const std::string& path) {
    cpu = ObjectFactory::createObject<CPU>(CPU_NAME);
#ifdef ARCH_RISCV
    arch = new cds::arch::riscv::RiscvArch();
#endif
    config.setup(path);
    Log::initStdio(config.serial_stdio, config.log_stdio, config.log_path);
    Log::start_tick = config.log_start_tick;
    Log::end_tick = config.log_end_tick;
    Base::setArch(arch);
    Base::setTick(&tick);
    arch->setTick(&tick);
    CacheManager::getInstance().memory = new Memory(config.memory_path);
    CacheManager::getInstance().load();
    CacheManager::getInstance().memory->load();
    cpu->load();

    CacheManager::getInstance().afterLoad();
    arch->afterLoad();
    arch->initConfig(config.arch_path);
    cpu->afterLoad();
    CacheManager::getInstance().getIrqHandler()->addIrqListener([this](uint64_t irq) {
        this->arch->irqListener(irq);
    });
}

void EMU::run() {
    while (Base::getTick() <= config.end_tick && arch->getInstret() < config.inst_count) {
        CacheManager::getInstance().memory->tick();
        for (auto& cache : CacheManager::getInstance().cache_map) {
            cache.second->tick();
        }
        cpu->exec();
    }
}
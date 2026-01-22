#ifndef RISCVARCH_H
#define RISCVARCH_H
#include "arch/arch.h"
#include "cache/cachemanager.h"
#include "arch/riscv/archstate.h"

namespace cds::arch::riscv {

class RiscvArch : public Arch {
public:
    void translateAddr(uint64_t vaddr, FETCH_TYPE type, uint64_t& paddr, uint64_t& exception) override;
    void handleException(uint64_t exception, uint64_t paddr, DecodeInfo* info) override;
    int decode(uint64_t vaddr, uint64_t paddr, DecodeInfo* info) override;
    bool paddrRead(uint64_t paddr, int size, FETCH_TYPE type, uint8_t* data) override;
    bool paddrWrite(uint64_t paddr, int size, FETCH_TYPE type, uint8_t* data) override;
    void initConfig(const std::string& config_path) override;
    void afterLoad() override;
    uint64_t updateEnv() override;
    uint64_t getStartPC() override { return 0x80000000; }
    void flushCache(uint8_t id, uint64_t addr, uint32_t asid) override;
    bool exceptionValid(uint64_t exception) override;
    uint64_t getExceptionNone() override { return EXC_NONE; }
    bool needFlush(DecodeInfo* info) override;
    void irqListener(uint64_t irq) override;
    void printState() override;
private:
    void fetch(uint64_t paddr, uint32_t* inst, bool& rvc, uint8_t* size);
    bool checkPermission(PTE& pte, bool ok, uint64_t vaddr, int type);
    void initOps();
    void updateMMUState();
private:
    Memory* memory;
    ArchEnv* env;
    ArchState* state;

    int ifetch_mmu_state;
    int data_mmu_state;
};

REGISTER_CLASS(RiscvArch)
} // namespace cds::arch::riscv
#endif
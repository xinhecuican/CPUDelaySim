#ifndef CPU_CACHECPU_H_
#define CPU_CACHECPU_H_
#include "cpu/cpu.h"
#include "cache/cache.h"

class CacheCPU : public CPU {
public:
    ~CacheCPU();
    void load() override;
    void afterLoad() override;
    void exec() override;

private:
    void icacheCallback(uint16_t* id, CacheTagv* tag);

protected:
    uint64_t pc;
    uint64_t inst_count;
    DecodeInfo* info;
    Cache* icache;
    bool inst_valid = false;
    uint64_t paddr;
    CacheReq* req = nullptr;
};


#endif
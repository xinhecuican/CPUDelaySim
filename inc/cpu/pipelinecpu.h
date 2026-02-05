#ifndef CPU_PIPELINECPU_H
#define CPU_PIPELINECPU_H

#include "cpu/cpu.h"
#include "pred/predictor.h"
#include "cache/cache.h"
#include "common/linklist.h"
#include "common/dbhandler.h"

class PipelineCPU : public CPU {
public:
    ~PipelineCPU();
    void load() override;
    void afterLoad() override;
    void exec() override;

private:
    struct Inst {
        bool taken;
        bool pred_error = false;
        uint8_t size;
        uint16_t mem_id;
        int bp_meta_idx;
        uint64_t pc;
        uint64_t next_pc;
        uint64_t real_pc;
        uint64_t real_target;
        uint64_t paddr;
#ifdef DB_INST
        uint64_t start_tick;
        uint16_t delay[4];
#endif
        DecodeInfo* info;

        Inst() {
            info = new DecodeInfo();
        }

        ~Inst() {
            delete info;
        }

        void copy(Inst* other) {
            memcpy(this, other, offsetof(Inst, info));
            memcpy(info, other->info, sizeof(DecodeInfo));
        }
    };
    struct CacheReqWrapper {
        CacheReq* req;
        Inst* inst;

        CacheReqWrapper() {
            req = new CacheReq();
            inst = new Inst();
        }

        ~CacheReqWrapper() {
            delete req;
            delete inst;
        }
    };

    CacheReqWrapper* initCacheReq();
    void frontRedirect(Inst* inst);
    void brRedirect(Inst* inst);
    void excRedirect(Inst* inst);

private:
    uint64_t retire_size;
    uint32_t mult_delay;
    uint32_t div_delay;
    uint32_t fadd_delay;
    uint32_t fmul_delay;
    uint32_t fma_delay;
    uint32_t fdiv_delay;
    uint32_t fsqrt_delay;
    uint32_t fmisc_complex_delay;

    Cache* icache;
    Cache* dcache;
    Predictor* predictor;

    uint64_t pred_pc;
    uint64_t pc;
    uint64_t inst_count;
    bool fetch_valid = false;
    bool id_valid = false;
    bool exe_valid = false;
    bool mem_valid = false;
    bool wb_valid = false;

    LinkList<CacheReqWrapper> cache_req_list;
    CacheReqWrapper* fetch_cache_req;
    uint64_t id_exception = 0;
    bool id_stall_valid = false;
    Inst* id_inst;
    Inst* id_stall_inst;
    Inst** id_extra_insts;
    int id_extra_idx = 0;
    int id_remain_size = 0;
    bool id_wait_redirect = false;
    uint64_t wait_redirect_tick = 0;
    Inst* wait_redirect_inst;
    uint64_t exe_stall_cycle = 0;
    bool exe_end = false;
    Inst* exe_inst;
    Inst* mem_inst;
    
    LinkList<CacheReq> mem_req_list;
    bool* mem_end_map;
    uint64_t mem_paddr;
    bool mem_req_valid = false;
    
    Inst* wb_inst;
    uint64_t wb_tick = 0;

#ifdef DB_INST
    struct  packed DBInstData {
        uint64_t start_tick;
        uint64_t pc;
        uint64_t paddr;
        InstType type;
        uint16_t delay[4];

        DBInstData(Inst* inst) {
            start_tick = inst->start_tick;
            pc = inst->real_pc;
            paddr = inst->paddr;
            type = inst->info->type;
            *(uint64_t*)delay = *(uint64_t*)inst->delay;
        }
    };
    LogDB* log_db;
#endif
};

REGISTER_CLASS(PipelineCPU)

#endif

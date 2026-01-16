#include "arch/riscv/riscvarch.h"
#include "arch/riscv/trans.h"
#include "arch/riscv/csrdefines.h"
#include "common/log.h"
#include "config.h"
#include "common/emuproxy.h"
#include <bit>
namespace cds::arch::riscv {

void RiscvArch::afterLoad() {
    memory = CacheManager::getInstance().memory;
    state = new ArchState();
    env = new ArchEnv();
    env->state = state;
    env->arch = this;
    memset(state, 0, sizeof(ArchState));
    initOps();
#ifdef LOG_PC
    Log::init("pc", Config::getLogFilePath("pc.log"));
#endif
#ifdef LOG_INST
    Log::init("inst", Config::getLogFilePath("inst.log"));
#endif
}

std::map<std::string, size_t> name_offset_map = {
    {"misa", ARCH_MISA},
    {"priv", ARCH_PRIV},
    {"mstatus", ARCH_MSTATUS},
    {"pmpcfg", ARCH_PMPCFG},
    {"pmpaddr", ARCH_PMPADDR},
    {"mideleg", ARCH_MIDELEG},
    {"medeleg", ARCH_MEDELEG},
};

void RiscvArch::initConfig(const std::string& config_path) {
    Arch::initConfig(config_path);
    for (const auto& cfg : archConfigs) {
        if (name_offset_map.find(cfg.name) == name_offset_map.end()) {
            continue;
        }
        uint32_t offset = name_offset_map[cfg.name] + cfg.offset;
        uint64_t* addr = (uint64_t*)((uint8_t*)state + offset);
        *addr = cfg.value;
    }
}

int RiscvArch::decode(uint64_t vaddr, uint64_t paddr, DecodeInfo* info) {
    bool rvc;
    bool decode_valid;
    fetch(paddr, &info->inst, rvc, &info->inst_size);
    env->pc = vaddr;
    env->paddr = paddr;
    env->info = info;
    memset(info, 0, DEC_MEMSET_END);
    env->info->exception = EXC_NONE;
    if (paddr == 0x82259586) {
        Log::info("RiscvArch::decode: paddr hit 0x80669df0");
    }
    decode_valid = interpreter(env, info->inst, rvc);
    if(unlikely(!decode_valid)) {
        Log::error("RiscvArch::decode: invalid instruction at 0x{:x}", paddr);
    }
    bool irq_enable = (state->priv <= MODE_S) && (state->mstatus & MSTATUS_SIE) || 
                    (state->priv == MODE_M) && (state->mstatus & MSTATUS_MIE) || (state->priv == MODE_U);
    int64_t irq_enable_mask = -irq_enable;
    uint32_t irq_valids = state->mip & state->mie & irq_enable_mask;
    if (irq_valids != 0) {
        info->exception = 0x80000000 | (std::bit_width(irq_valids) - 1);
    }
    return info->inst_size;
}

void RiscvArch::handleException(uint64_t exception, uint64_t paddr, DecodeInfo* info) {
    if (exception == EXC_IPF) {
        env->pc = paddr;
        env->info = info;
        info->exc_data = paddr;
        info->exception = exception;
    }
}

bool RiscvArch::getStream(uint64_t pc, uint8_t pred, bool btbv, BTBEntry* btb_entry, FetchStream* stream) {
    return true;
}

// csr
#define PGSHFT 12
#define VPNMASK 0x1ff
#define PTE_SIZE 8
#define PGBASE(pn) (pn << PGSHFT)
static inline uint64_t VPNiSHFT(int i) {
  return (PGSHFT) + 9 * i;
}

static inline uint64_t VPNi(uint64_t va, int i) {
  return (va >> VPNiSHFT(i)) & VPNMASK;
}

inline bool RiscvArch::checkPermission(PTE& pte, bool ok, uint64_t vaddr, int type) {
    bool ifetch = type == FETCH_TYPE::IFETCH;
    uint32_t mode = (STATUS_MPRV(state->mstatus) && !ifetch) ? STATUS_MPP(state->mstatus) : state->priv;
    ok = ok && pte.v && !(state->priv == MODE_U && !pte.u);
    ok = ok && !(pte.u && ((state->priv == MODE_S) && (!STATUS_SUM(state->mstatus) || ifetch)));
    if (ifetch) {
        bool update_ad = !pte.a;
        if (!(ok && pte.x && !pte.pad) || update_ad) {
            return false;
        }
    }
    else if (type == FETCH_TYPE::LFETCH) {
        bool can_load = pte.r || (STATUS_MXR(state->mstatus) && pte.x);
        bool update_ad = !pte.a;
        if (!(ok && can_load && !pte.pad) || update_ad) {
            return false;
        }
    } else {
        bool update_ad = !pte.a || !pte.d;
        if (!(ok && pte.w && !pte.pad) || update_ad) {
            return false;
        }
    }
    return true;
}

inline bool RiscvArch::paddrRead(uint64_t paddr, int size, FETCH_TYPE type, uint8_t* data) {
// impl pmp check
    bool mmio;
    memory->paddrRead(paddr, size, data, mmio);
    return !mmio;
}

inline bool RiscvArch::paddrWrite(uint64_t paddr, int size, FETCH_TYPE type, uint8_t* data) {
// impl pmp check
    bool mmio;
    memory->paddrWrite(paddr, size, data, mmio);
    return !mmio;
}



void RiscvArch::translateAddr(uint64_t vaddr, FETCH_TYPE type, uint64_t& paddr, uint64_t& exception) {
    exception = 0;
    if (type == FETCH_TYPE::IFETCH && !ifetch_mmu_state ||
        type != FETCH_TYPE::IFETCH && !data_mmu_state) {
        paddr = vaddr;
        return;
    }
    paddr = (state->satp & 0xfffffffffff) << PGSHFT;
    int max_level = 3;
    PTE pte;
    int level;
    uint64_t p_pte;
    for (level = max_level - 1; level >= 0;) {
        p_pte = paddr + VPNi(vaddr, level) * PTE_SIZE;
        if(!paddrRead(p_pte, PTE_SIZE, type, (uint8_t*)(&pte.val))) {
            exception = EXC_IPF;
            return;
        }
        paddr = PGBASE((uint64_t)pte.ppn);
        if (!pte.v || (!pte.r && pte.w) || pte.pad) {
            exception = EXC_IPF;
            return;
        }
        if (pte.r || pte.x) break;
        else {
            if (pte.a || pte.d || pte.u || pte.pbmt || pte.n) {
                exception = EXC_IPF;
                return;
            }
            level--;
            if (unlikely(level < 0)) {
                exception = EXC_IPF;
                return;
            }
        }
    }
    if (!checkPermission(pte, true, vaddr, type)) {
        exception = EXC_IPF;
        return;
    }
    
    uint64_t pg_mask = ((1ull << VPNiSHFT(level)) - 1);
    if (level > 0) {
        if ((paddr & pg_mask) != 0) {
            exception = EXC_IPF;
            return;
        }
    }
    paddr = (paddr & ~pg_mask) | (vaddr & pg_mask);
}



inline void RiscvArch::fetch(uint64_t paddr, uint32_t* inst, bool& rvc, uint8_t* size) {
    bool mmio;
    memory->paddrRead(paddr, 4, (uint8_t*)inst, mmio);
    if (unlikely(mmio)) {
        Log::error("RiscvArch::fetch: mmio fetch at 0x{:x}", paddr);
    }
    rvc = ((*inst) & 3) != 3;
    int rvc_idx = rvc << 4;
    *inst = (*inst << rvc_idx) >> rvc_idx;
    *size = 4 >> rvc;
}   

uint64_t RiscvArch::updateEnv() {
    DecodeInfo* info = env->info;
    ArchState* state = env->state;
    uint64_t exception_idx = info->exception & EXC_MASK;
    uint64_t exception_mask = (1 << exception_idx);
    uint64_t irq = (int32_t)info->exception < 0;
    bool edeleg_valid = (((irq ? state->mideleg : state->medeleg) & exception_mask) != 0) && (state->priv != MODE_M);
#ifdef LOG_INST
    Log::trace("inst", "pc: {:x}, paddr: {:x}, inst: {:8x}", env->pc, env->paddr, info->inst);
#endif
#ifdef DIFFTEST
    uint64_t mpf_ptr[3];
    mpf_ptr[0] = info->dst_data[0];
    mpf_ptr[1] = state->mip;
    mpf_ptr[2] = 0;
    NemuProxy::getInstance().mpfcpy((void*)mpf_ptr, DUT_TO_REF);
    if (info->type == SC && info->dst_data[0] != 0) {
        struct SyncState sync;
        sync.lrscValid = 0;
        sync.lrscAddr = 0;
        NemuProxy::getInstance().uarchstatus_cpy((uint64_t*)&sync, DUT_TO_REF); // sync lr/sc microarchitectural regs
    }
    // if (info->exception == EXC_IPF || info->exception == EXC_LPF || info->exception == EXC_SPF) {
    //     struct ExecutionGuide guide;
    //     guide.force_raise_exception = true;
    //     guide.exception_num = info->exception;
    //     guide.mtval = info->exc_data;
    //     guide.stval = info->exc_data;
    //     guide.force_set_jump_target = false;
    //     NemuProxy::getInstance().guided_exec(&guide);
    // } else {
    if (irq) {
        NemuProxy::getInstance().raise_intr(exception_idx | (1ULL << 63ULL));
    } else {
        NemuProxy::getInstance().exec(1);
    }
    // }
    arch_core_state_t ref;
    NemuProxy::getInstance().regcpy((void*)&ref, REF_TO_DUT);
    if (NemuProxy::getInstance().pc != env->pc) {
        Log::error("RiscvArch::updateEnv: pc mismatch, ref: 0x{:x}, dut: 0x{:x}", NemuProxy::getInstance().pc, env->pc);
        exit(1);
    }
    NemuProxy::getInstance().pc = ref.csrs.this_pc;
    uint64_t dut_mstatus;
#endif
    if(unlikely(irq)) {
        if (edeleg_valid) state->stval = 0;
        else state->mtval = 0;
        goto xtval_end;
    } else {
        switch (info->exception) {
            case EXC_DT:
            case EXC_SC:
            case EXC_HE:
                Log::error("exception {} at 0x{:x}", info->exception, env->pc);
                exit(1);
            case EXC_II:
            case EXC_LAM:
            case EXC_LAF:
            case EXC_SAM:
            case EXC_SAF:
            case EXC_IPF:
            case EXC_LPF:
            case EXC_SPF:
                if (edeleg_valid) state->stval = info->exc_data;
                else state->mtval = info->exc_data;
                goto xtval_end;
            case EXC_ECU:
            case EXC_ECS:
            case EXC_ECM:
            case EXC_BP:
                if (edeleg_valid) state->stval = 0;
                else state->mtval = 0;
xtval_end:;
                if (edeleg_valid) {
                    state->sepc = env->pc;
                    state->scause = exception_idx | (irq << 63ULL);
                    state->mstatus = (state->mstatus & ~(MSTATUS_SPP | MSTATUS_SPIE | MSTATUS_SIE)) | (state->priv << MSTATUS_SPP_SHIFT) | ((state->mstatus << 4) & MSTATUS_SPIE);
                    state->priv = MODE_S;
                } else {
                    state->mepc = env->pc;
                    state->mcause = exception_idx | (irq << 63ULL);
                    state->mstatus = (state->mstatus & ~(MSTATUS_MPP | MSTATUS_MPIE | MSTATUS_MIE)) | (state->priv << MSTATUS_MPP_SHIFT) | ((state->mstatus << 4) & MSTATUS_MPIE);
                    state->priv = MODE_M;
                }
                goto mtvec_out; 
            case EXC_SRET:
                state->priv = (state->mstatus >> MSTATUS_SPP_SHIFT) & 0x3;
                state->mstatus = (state->mstatus & ~(MSTATUS_SPP | MSTATUS_SIE | MSTATUS_MPRV)) | ((state->mstatus >> 4) & MSTATUS_SIE) | MSTATUS_SPIE;
                updateMMUState();
                return state->sepc;
            case EXC_MRET:
                state->priv = (state->mstatus >> MSTATUS_MPP_SHIFT) & 0x3;
                state->mstatus = (state->mstatus & ~(MSTATUS_MPP | MSTATUS_MIE |
                (((state->priv != MODE_M) << MSTATUS_MPRV_SHIFT)))) | ((state->mstatus >> 4) & MSTATUS_MIE) | MSTATUS_MPIE;
                updateMMUState();
                return state->mepc;
            case EXC_NONE:
                for (int i = 0; i < 3; i++) {
                        uint64_t* dst_addr = (uint64_t*)(((uint8_t*)state)+info->dst_idx[i]);
                        *dst_addr = ((*dst_addr) & ~info->dst_mask[i]) | (info->dst_data[i] & info->dst_mask[i]);
                }
                switch (info->type) {
                    case AMO:
                    case STORE:
                        paddrWrite(info->dst_data[2], info->dst_idx[2], FETCH_TYPE::SFETCH, (uint8_t*)&info->dst_data[1]);
                        break;
                    case SC:
                        if (!info->dst_data[0]) {
                            uint64_t ha;
                            uint64_t exception;
                            translateAddr(info->exc_data, FETCH_TYPE::SFETCH, ha, exception);
                            paddrWrite(ha, info->dst_idx[2], FETCH_TYPE::SFETCH, (uint8_t*)&info->dst_data[2]);
                        }
                        break;
                }
                state->gpr[0] = 0;
                updateMMUState();
#ifdef DIFFTEST
                dut_mstatus = (state->mstatus & (MSTATUS_MASK | MSTATUS64_SXL | MSTATUS64_UXL)) | 
        ((uint64_t)((state->mstatus & MSTATUS_FS) == MSTATUS_FS) << MSTATUS64_SD);
                if (ref.csrs.mstatus != dut_mstatus) {
                    Log::error("RiscvArch::updateEnv: mstatus mismatch, ref: 0x{:x}, dut: 0x{:x}", ref.csrs.mstatus, dut_mstatus);
                    exit(1);
                }
                for (int i = 0; i < 32; i++) {
                    if (state->gpr[i] != ref.regs.gpr[i]) {
                        Log::error("RiscvArch::updateEnv: reg gpr[{}] mismatch, ref: 0x{:x}, dut: 0x{:x}", i, ref.regs.gpr[i], state->gpr[i]);
                        exit(1);
                    }
                }
#endif
                switch(info->type) {
                    case DIRECT:
                    case INDIRECT:
                    case IND_CALL:
                    case PUSH:
                    case POP:
                    case POP_PUSH:
#ifdef LOG_PC
                        Log::trace("pc", "0x{:x}", info->dst_data[2]);
#endif
                        return info->dst_data[2];
                    case COND:
#ifdef LOG_PC
                        if (info->dst_data[1]) Log::trace("pc", "0x{:x}", info->dst_data[2]);
#endif
                        if (info->dst_data[1]) return info->dst_data[2];
                        else return env->pc + info->inst_size;
                    default: return env->pc + info->inst_size;
                }
            default: 
                Log::error("RiscvArch::updateEnv: unknown exception {} at 0x{:x}", info->exception, env->pc);
                exit(1);
        }
    }

mtvec_out:;
    updateMMUState();
    bool mtvec_mode = irq && (state->mtvec & 3);
    uint64_t mtvec_base = state->mtvec & 0xfffffffffffffffc;
    uint64_t mtvec_addr = mtvec_mode ? mtvec_base + ((state->mcause & 0x1f) << 2) : mtvec_base;
    bool stvec_mode = irq && (state->stvec & 3);
    uint64_t stvec_base = state->stvec & 0xfffffffffffffffc;
    uint64_t stvec_addr = stvec_mode ? stvec_base + ((state->scause & 0x1f) << 2) : stvec_base;
    if (edeleg_valid) {
        return stvec_addr;
    } else  {
        return mtvec_addr;
    }
}

void RiscvArch::updateMMUState() {
    ifetch_mmu_state = (state->satp >> 60) == 8 && (state->priv != MODE_M);
    data_mmu_state = (state->satp >> 60) == 8 && (((state->mstatus & MSTATUS_MPRV) ? 
                                        (state->mstatus & MSTATUS_MPP) >> MSTATUS_MPP_SHIFT : state->priv) != MODE_M);
}

void RiscvArch::irqListener(uint64_t irq) {
    state->mip = irq;
}

void RiscvArch::printState() {
    Log::info("*******************************************************************************");
    
    const char* reg_name[32] = {
        "zero", "ra", "sp", "gp", "tp", "t0", "t1", "t2",
        "s0", "s1", "a0", "a1", "a2", "a3", "a4", "a5",
        "a6", "a7", "s2", "s3", "s4", "s5", "s6", "s7",
        "s8", "s9", "s10", "s11", "t3", "t4", "t5", "t6"
    };
    
    for (int i = 0; i < 32; i += 4) {
        Log::info("{}(x{:d}): 0x{:016x} {}(x{:d}): 0x{:016x} {}(x{:d}): 0x{:016x} {}(x{:d}): 0x{:016x}", reg_name[i], i, state->gpr[i], reg_name[i+1], i+1, state->gpr[i+1], reg_name[i+2], i+2, state->gpr[i+2], reg_name[i+3], i+3, state->gpr[i+3]);
    }
    
    Log::info("pc: 0x{:016x}", env->pc);
    Log::info("paddr: 0x{:016x}", env->paddr);
    Log::info("priv: 0x{:016x}", state->priv);
    Log::info("mstatus: 0x{:016x},    mie: 0x{:016x},    mip: 0x{:016x}", state->mstatus, state->mie, state->mip);
    Log::info("mtvec: 0x{:016x},    mepc: 0x{:016x},    mcause: 0x{:016x}", state->mtvec, state->mepc, state->mcause);
    Log::info("mtval: 0x{:016x},     satp: 0x{:016x},     misa: 0x{:016x}", state->mtval, state->satp, state->misa);
    Log::info("mideleg: 0x{:016x},  medeleg: 0x{:016x},  mhartid: 0x{:08x}", state->mideleg, state->medeleg, state->mhartid);
    Log::info("stvec: 0x{:016x},    sepc: 0x{:016x},    scause: 0x{:016x}", state->stvec, state->sepc, state->scause);
    Log::info("stval: 0x{:016x},     sie: 0x{:016x}", state->stval, state->sie);
    Log::info("badaddr: 0x{:016x}", state->badaddr);
    Log::info("*******************************************************************************");
}

void RiscvArch::flushCache(uint8_t id, uint64_t addr, uint32_t asid) {
    if (id == 0) {
        CacheManager::getInstance().getICache()->flush(addr, asid);
    } else if (id == 1) {
        CacheManager::getInstance().getDCache()->flush(addr, asid);
    } else if (id == 2) {
        // TODO: MMU flush
    }
}

constexpr uint64_t delegable_ints = S_MODE_INTERRUPTS;
constexpr uint64_t all_ints = M_MODE_INTERRUPTS | S_MODE_INTERRUPTS | LOCAL_INTERRUPTS;
constexpr uint64_t sip_writable_mask = MIP_SSIP | LOCAL_INTERRUPTS;

bool riscv_cpu_fp_enabled(DisasContext* ctx) {
    return (ctx->state->mstatus & MSTATUS_FS) != 0;
}

static void fs(DisasContext *ctx, int csrno) {
    if (riscv_cpu_fp_enabled(ctx)) {
        return;
    }
    ctx->info->exception = EXC_II;
}

static void ctr(DisasContext *ctx, int csrno) {
    if (csrno >= CSR_CYCLE && csrno <= CSR_INSTRET) {
        uint64_t ctr_mask;
        int ctr_index = csrno - CSR_CYCLE;
        ctr_mask = 1ULL << ctr_index;
        if (ctx->state->priv < MODE_M && !(ctx->state->mcounteren & ctr_mask)) {
            ctx->info->exception = EXC_II;
        }
        if (ctx->state->priv == MODE_U && !(ctx->state->scounteren & ctr_mask)) {
            ctx->info->exception = EXC_II;
        }
        return;
    }
    ctx->info->exception = EXC_II;
}

static void any(DisasContext *ctx, int csrno) {
}

static void umode(DisasContext *ctx, int csrno) {
}

static void smode(DisasContext *ctx, int csrno) {
}


static void satp(DisasContext *ctx, int csrno) {
}

static void pmp(DisasContext *ctx, int csrno) {
    if (csrno <= CSR_PMPCFG3) {
        uint32_t reg_index = csrno - CSR_PMPCFG0;
        if (reg_index & 1) {
            ctx->info->exception = EXC_II;
        }
    }
}

#define csrr_func_def(name) static void read_ ## name(DisasContext* ctx, int csrno, uint64_t *val)
#define csrw_func_def(name) static void write_ ## name(DisasContext* ctx, int csrno, uint64_t val)
#define csrrmw_func_def(name) static void rmw_ ## name(DisasContext *ctx, int csrno, uint64_t *ret_val, uint64_t new_val, uint64_t wr_mask)

csrr_func_def(fflags) {
    do {
        if (!(ctx->state->mstatus & MSTATUS_FS)) {
            ctx->info->exception = EXC_II;
            return;
        }
        ctx->state->mstatus |= MSTATUS_FS;
    } while (0);
    *val = ctx->state->fp_status.float_exception_flags;
}

csrw_func_def(fflags) {
    do {
        if (!(ctx->state->mstatus & MSTATUS_FS)) {
            ctx->info->exception = EXC_II;
            return;
        }
        ctx->state->mstatus |= MSTATUS_FS;
    } while (0);
    uint64_t dest = val & FSR_AEXC;
    ctx->info->dst_idx[1] = ARCH_FFLAGS;
    ctx->info->dst_mask[1] = FSR_AEXC;
    ctx->info->dst_data[1] = dest;

    ctx->info->dst_idx[2] = ARCH_MSTATUS;
    ctx->info->dst_mask[2] = MSTATUS_FS;
    ctx->info->dst_data[2] = MSTATUS_FS;
}

csrr_func_def(frm) {
    do {
        if (!(ctx->state->mstatus & MSTATUS_FS)) {
            ctx->info->exception = EXC_II;
            return;
        }
        ctx->state->mstatus |= MSTATUS_FS;
    } while (0);
    *val = ctx->state->frm;
}

csrw_func_def(frm) {
    do {
        if (!(ctx->state->mstatus & MSTATUS_FS)) {
            ctx->info->exception = EXC_II;
            return;
        }
        ctx->state->mstatus |= MSTATUS_FS;
    } while (0);
    uint64_t dest = (val & FSR_RD) >> FSR_RD_SHIFT;
    ctx->info->dst_idx[1] = ARCH_FRM;
    ctx->info->dst_mask[1] = FSR_RD;
    ctx->info->dst_data[1] = dest;

    ctx->info->dst_idx[2] = ARCH_MSTATUS;
    ctx->info->dst_mask[2] = MSTATUS_FS;
    ctx->info->dst_data[2] = MSTATUS_FS;
}

csrr_func_def(fcsr) {
    do {
        if (!(ctx->state->mstatus & MSTATUS_FS)) {
            ctx->info->exception = EXC_II;
            return;
        }
        ctx->state->mstatus |= MSTATUS_FS;
    } while (0);
    *val = ctx->state->fp_status.float_exception_flags | (ctx->state->frm << FSR_RD_SHIFT);
}

csrw_func_def(fcsr) {
    do {
        if (!(ctx->state->mstatus & MSTATUS_FS)) {
            ctx->info->exception = EXC_II;
            return;
        }
        ctx->state->mstatus |= MSTATUS_FS;
    } while (0);
    // ArchState的布局为
    // uint8_t frm
    // float_status fp_status
    // 而fflags在fp_status最低位
    uint64_t dest = ((val & FSR_AEXC) << 8) | ((val & FSR_RD) >> FSR_RD_SHIFT);
    ctx->info->dst_idx[1] = ARCH_FRM;
    ctx->info->dst_mask[1] = (FSR_AEXC << 8) | FSR_RD;
    ctx->info->dst_data[1] = dest;

    ctx->info->dst_idx[2] = ARCH_MSTATUS;
    ctx->info->dst_mask[2] = MSTATUS_FS;
    ctx->info->dst_data[2] = MSTATUS_FS;
}

static uint64_t get_ticks(DisasContext *ctx) {
    return ctx->arch->getTick();
}

static uint64_t get_instret(DisasContext *ctx) {
    return ctx->arch->getInstret();
}

csrr_func_def(time) {
    *val = get_ticks(ctx);
}

csrr_func_def(cycle) {
    *val = get_ticks(ctx);
}

csrr_func_def(noop) {
    *val = 0;
}

csrw_func_def(mhpmcounter) {
    assert(0);
}

csrr_func_def(instret) {
    *val = get_instret(ctx);
}

csrr_func_def(mvendorid) {
    *val = ctx->state->mvendorid;
}

csrr_func_def(marchid) {
    *val = ctx->state->marchid;
}

csrr_func_def(mimpid) {
    *val = ctx->state->mimpid;
}

csrr_func_def(mhartid) {
    *val = ctx->state->mhartid;
}

csrr_func_def(mstatus) {
    *val = (ctx->state->mstatus & (MSTATUS_MASK | MSTATUS64_UXL | MSTATUS64_SXL)) | 
            ((uint64_t)((ctx->state->mstatus & MSTATUS_FS) == MSTATUS_FS) << MSTATUS64_SD);
}

csrw_func_def(mstatus) {
    ctx->info->dst_idx[1] = ARCH_MSTATUS;
    ctx->info->dst_mask[1] = MSTATUS_MASK;
    ctx->info->dst_data[1] = val;
}

csrr_func_def(misa) {
    *val = ctx->state->misa;
}

csrw_func_def(misa) {
    ctx->info->exception = EXC_II;
}

csrrmw_func_def(mideleg) {
    if (ret_val) {
        *ret_val = ctx->state->mideleg;
    }
    ctx->info->dst_idx[1] = ARCH_MIDELEG;
    ctx->info->dst_mask[1] = delegable_ints & wr_mask;
    ctx->info->dst_data[1] = new_val;
}

csrr_func_def(medeleg) {
    *val = ctx->state->medeleg;
}

csrw_func_def(medeleg) {
    ctx->info->dst_idx[1] = ARCH_MEDELEG;
    ctx->info->dst_mask[1] = DELEGABLE_EXCPS;
    ctx->info->dst_data[1] = val;
}

csrrmw_func_def(mie) {
    if (ret_val) *ret_val = ctx->state->mie;
    ctx->info->dst_idx[1] = ARCH_MIE;
    ctx->info->dst_mask[1] = all_ints & wr_mask;
    ctx->info->dst_data[1] = new_val;
}

csrr_func_def(mtvec) {
    *val = ctx->state->mtvec;
}

csrw_func_def(mtvec) {
    ctx->info->dst_idx[1] = ARCH_MTVEC;
    ctx->info->dst_mask[1] = 0xfffffffffffffffd;
    ctx->info->dst_data[1] = val;
}

csrr_func_def(mcounteren) {
    *val = ctx->state->mcounteren;
}

csrw_func_def(mcounteren) {
    ctx->info->dst_idx[1] = ARCH_MCOUNTEREN;
    ctx->info->dst_mask[1] = 0x7;
    ctx->info->dst_data[1] = val;
}

csrr_func_def(mscratch) {
    *val = ctx->state->mscratch;
}

csrw_func_def(mscratch) {
    ctx->info->dst_idx[1] = ARCH_MSCRATCH;
    ctx->info->dst_mask[1] = 0xffffffffffffffff;
    ctx->info->dst_data[1] = val;
}

csrr_func_def(mepc) {
    *val = ctx->state->mepc;
}

csrw_func_def(mepc) {
    ctx->info->dst_idx[1] = ARCH_MEPC;
    ctx->info->dst_mask[1] = 0xffffffffffffffff;
    ctx->info->dst_data[1] = val;
}

csrr_func_def(mcause) {
    *val = ctx->state->mcause;
}

csrw_func_def(mcause) {
    ctx->info->dst_idx[1] = ARCH_MCAUSE;
    ctx->info->dst_mask[1] = 0xffffffffffffffff;
    ctx->info->dst_data[1] = val;
}

csrr_func_def(mtval) {
    *val = ctx->state->mtval;
}

csrw_func_def(mtval) {
    ctx->info->dst_idx[1] = ARCH_MTVAL;
    ctx->info->dst_mask[1] = 0xffffffffffffffff;
    ctx->info->dst_data[1] = val;
}

csrrmw_func_def(mip) {
    if (ret_val) *ret_val = ctx->state->mip;
    ctx->info->dst_idx[1] = ARCH_MIP;
    ctx->info->dst_mask[1] = wr_mask & delegable_ints;
    ctx->info->dst_data[1] = new_val;
}

csrr_func_def(menvcfg) {
    *val = ctx->state->menvcfg;
}

csrw_func_def(menvcfg) {
    ctx->info->dst_idx[1] = ARCH_MENVCFG;
    ctx->info->dst_mask[1] = MENVCFG_MASK;
    ctx->info->dst_data[1] = val;
}

csrr_func_def(senvcfg) {
    *val = ctx->state->senvcfg;
}

csrw_func_def(senvcfg) {
    ctx->info->dst_idx[1] = ARCH_SENVCFG;
    ctx->info->dst_mask[1] = MENVCFG_MASK;
    ctx->info->dst_data[1] = val;
}

csrr_func_def(sstatus) {
    *val = (ctx->state->mstatus & (SSTATUS_MASK | MSTATUS64_UXL)) | 
           ((uint64_t)((ctx->state->mstatus & MSTATUS_FS) == MSTATUS_FS) << MSTATUS64_SD);
}

csrw_func_def(sstatus) {
    ctx->info->dst_idx[1] = ARCH_MSTATUS;
    ctx->info->dst_mask[1] = SSTATUS_MASK;
    ctx->info->dst_data[1] = val;
}

csrrmw_func_def(sie) {
    rmw_mie(ctx, csrno, ret_val, new_val, wr_mask & ctx->state->mideleg & (S_MODE_INTERRUPTS | LOCAL_INTERRUPTS));
}

csrr_func_def(stvec) {
    *val = ctx->state->stvec;
}

csrw_func_def(stvec) {
    ctx->info->dst_idx[1] = ARCH_STVEC;
    ctx->info->dst_mask[1] = 0xfffffffffffffffd;
    ctx->info->dst_data[1] = val;
}

csrr_func_def(scounteren) {
    *val = ctx->state->scounteren;
}

csrw_func_def(scounteren) {
    ctx->info->dst_idx[1] = ARCH_SCOUNTEREN;
    ctx->info->dst_mask[1] = 0x7;
    ctx->info->dst_data[1] = val;
}

csrr_func_def(sscratch) {
    *val = ctx->state->sscratch;
}

csrw_func_def(sscratch) {
    ctx->info->dst_idx[1] = ARCH_SSCRATCH;
    ctx->info->dst_mask[1] = 0xffffffffffffffff;
    ctx->info->dst_data[1] = val;
}
csrr_func_def(sepc) {
    *val = ctx->state->sepc;
}

csrw_func_def(sepc) {
    ctx->info->dst_idx[1] = ARCH_SEPC;
    ctx->info->dst_mask[1] = 0xffffffffffffffff;
    ctx->info->dst_data[1] = val;
}

csrr_func_def(scause) {
    *val = ctx->state->scause;
}

csrw_func_def(scause) {
    ctx->info->dst_idx[1] = ARCH_SCAUSE;
    ctx->info->dst_mask[1] = 0xffffffffffffffff;
    ctx->info->dst_data[1] = val;
}

csrr_func_def(stval) {
    *val = ctx->state->stval;
}

csrw_func_def(stval) {
    ctx->info->dst_idx[1] = ARCH_STVAL;
    ctx->info->dst_mask[1] = 0xffffffffffffffff;
    ctx->info->dst_data[1] = val;
}

csrrmw_func_def(sip) {
    rmw_mip(ctx, csrno, ret_val, new_val, wr_mask & ctx->state->mideleg & (LOCAL_INTERRUPTS | S_MODE_INTERRUPTS));
}

csrr_func_def(satp) {
    *val = ctx->state->satp;
}

csrw_func_def(satp) {
    uint64_t mode = val >> 60;
    if (mode == 0 || mode == 8) {
        ctx->info->dst_idx[1] = ARCH_SATP;
        ctx->info->dst_mask[1] = 0x800fffffffffffff;
        ctx->info->dst_data[1] = val;
    }
}

csrr_func_def(pmpcfg) {
    uint32_t reg_index = csrno - CSR_PMPCFG0;
    if (reg_index & 1) {
        ctx->info->exception = EXC_II;
        return;
    }
    *val = ctx->state->pmp_table.pmp[reg_index >> 1];
}

csrw_func_def(pmpcfg) {
    uint32_t reg_index = csrno - CSR_PMPCFG0;
    if (reg_index & 1) {
        ctx->info->exception = EXC_II;
        return;
    }
    ctx->info->dst_idx[1] = ARCH_PMPCFG + (reg_index << 2);
    ctx->info->dst_mask[1] = 0xffffffffffffffff;
    ctx->info->dst_data[1] = val;
}

csrr_func_def(pmpaddr) {
    uint32_t reg_index = csrno - CSR_PMPADDR0;
    *val = ctx->state->pmp_table.addr[reg_index];
}

csrw_func_def(pmpaddr) {
    uint32_t reg_index = csrno - CSR_PMPADDR0;
    ctx->info->dst_idx[1] = ARCH_PMPADDR + (reg_index << 3);
    ctx->info->dst_mask[1] = 0xfffffffffffffc00;
    ctx->info->dst_data[1] = val;
}

csrw_func_def(noop) {}

csrr_func_def(mcountinhibit) {
    *val = ctx->state->mcountinhibit;
}

csrw_func_def(mcountinhibit) {
    // ctx->info->exception = EXC_II;
}

riscv_csr_operations csr_ops[CSR_TABLE_SIZE];

void RiscvArch::initOps() {
    /* User Floating-Point CSRs */
    csr_ops[CSR_FFLAGS]   = { "fflags",   fs,     read_fflags,  write_fflags };
    csr_ops[CSR_FRM]      = { "frm",      fs,     read_frm,     write_frm    };
    csr_ops[CSR_FCSR]     = { "fcsr",     fs,     read_fcsr,    write_fcsr   };
    /* User Timers and Counters */
    csr_ops[CSR_CYCLE]    = { "cycle",    ctr,    read_cycle    };
    csr_ops[CSR_INSTRET]  = { "instret",  ctr,    read_instret  };

    /*
     * In privileged mode, the monitor will have to emulate TIME CSRs only if
     * rdtime callback is not provided by machine/platform emulation.
     */
    csr_ops[CSR_TIME]  = { "time",  ctr,   read_time  };

    /* Machine Timers and Counters */
    csr_ops[CSR_MCYCLE]    = { "mcycle",    any,   read_cycle,
                        write_mhpmcounter                    };
    csr_ops[CSR_MINSTRET]  = { "minstret",  any,   read_instret,
                        write_mhpmcounter                    };

    /* Machine Information Registers */
    csr_ops[CSR_MVENDORID] = { "mvendorid", any,   read_mvendorid };
    csr_ops[CSR_MARCHID]   = { "marchid",   any,   read_marchid   };
    csr_ops[CSR_MIMPID]    = { "mimpid",    any,   read_mimpid    };
    csr_ops[CSR_MHARTID]   = { "mhartid",   any,   read_mhartid   };
    /* Machine Trap Setup */
    csr_ops[CSR_MSTATUS]     = { "mstatus",    any,   read_mstatus, write_mstatus };
    csr_ops[CSR_MISA]        = { "misa",       any,   read_misa,    write_misa    };
    csr_ops[CSR_MIDELEG]     = { "mideleg",    any,   NULL, NULL,   rmw_mideleg   };
    csr_ops[CSR_MEDELEG]     = { "medeleg",    any,   read_medeleg, write_medeleg };
    csr_ops[CSR_MIE]         = { "mie",        any,   NULL, NULL,   rmw_mie       };
    csr_ops[CSR_MTVEC]       = { "mtvec",      any,   read_mtvec,   write_mtvec   };
    csr_ops[CSR_MCOUNTEREN]  = { "mcounteren", umode, read_mcounteren,  
                          write_mcounteren                                 };

    /* Machine Trap Handling */
    csr_ops[CSR_MSCRATCH] = { "mscratch", any,  read_mscratch, write_mscratch };
    csr_ops[CSR_MEPC]     = { "mepc",     any,  read_mepc,     write_mepc     };
    csr_ops[CSR_MCAUSE]   = { "mcause",   any,  read_mcause,   write_mcause   };
    csr_ops[CSR_MTVAL]    = { "mtval",    any,  read_mtval,    write_mtval    };
    csr_ops[CSR_MIP]      = { "mip",      any,  NULL,    NULL, rmw_mip        };


    /* Execution environment configuration */
    csr_ops[CSR_MENVCFG]  = { "menvcfg",  umode, read_menvcfg,  write_menvcfg };
    csr_ops[CSR_SENVCFG]  = { "senvcfg",  smode, read_senvcfg,  write_senvcfg };



    /* Supervisor Trap Setup */
    csr_ops[CSR_SSTATUS]    = { "sstatus",    smode, read_sstatus,    write_sstatus };
    csr_ops[CSR_SIE]        = { "sie",        smode, NULL,   NULL,    rmw_sie       };
    csr_ops[CSR_STVEC]      = { "stvec",      smode, read_stvec,      write_stvec   };
    csr_ops[CSR_SCOUNTEREN] = { "scounteren", smode, read_scounteren,   
                         write_scounteren                                    };

    /* Supervisor Trap Handling */
    csr_ops[CSR_SSCRATCH] = { "sscratch", smode, read_sscratch, write_sscratch };
    csr_ops[CSR_SEPC]     = { "sepc",     smode, read_sepc,     write_sepc     };
    csr_ops[CSR_SCAUSE]   = { "scause",   smode, read_scause,   write_scause   };
    csr_ops[CSR_STVAL]    = { "stval",    smode, read_stval,    write_stval    };
    csr_ops[CSR_SIP]      = { "sip",      smode, NULL,    NULL, rmw_sip        };

    /* Supervisor Protection and Translation */
    csr_ops[CSR_SATP]     = { "satp",     satp, read_satp,     write_satp     };

    /* Physical Memory Protection */
    csr_ops[CSR_PMPCFG0]    = { "pmpcfg0",   pmp, read_pmpcfg,  write_pmpcfg  };
    csr_ops[CSR_PMPCFG1]    = { "pmpcfg1",   pmp, read_pmpcfg,  write_pmpcfg  };
    csr_ops[CSR_PMPCFG2]    = { "pmpcfg2",   pmp, read_pmpcfg,  write_pmpcfg  };
    csr_ops[CSR_PMPCFG3]    = { "pmpcfg3",   pmp, read_pmpcfg,  write_pmpcfg  };
    csr_ops[CSR_PMPADDR0]   = { "pmpaddr0",  pmp, read_pmpaddr, write_pmpaddr };
    csr_ops[CSR_PMPADDR1]   = { "pmpaddr1",  pmp, read_pmpaddr, write_pmpaddr };
    csr_ops[CSR_PMPADDR2]   = { "pmpaddr2",  pmp, read_pmpaddr, write_pmpaddr };
    csr_ops[CSR_PMPADDR3]   = { "pmpaddr3",  pmp, read_pmpaddr, write_pmpaddr };
    csr_ops[CSR_PMPADDR4]   = { "pmpaddr4",  pmp, read_pmpaddr, write_pmpaddr };
    csr_ops[CSR_PMPADDR5]   = { "pmpaddr5",  pmp, read_pmpaddr, write_pmpaddr };
    csr_ops[CSR_PMPADDR6]   = { "pmpaddr6",  pmp, read_pmpaddr, write_pmpaddr };
    csr_ops[CSR_PMPADDR7]   = { "pmpaddr7",  pmp, read_pmpaddr, write_pmpaddr };
    csr_ops[CSR_PMPADDR8]   = { "pmpaddr8",  pmp, read_pmpaddr, write_pmpaddr };
    csr_ops[CSR_PMPADDR9]   = { "pmpaddr9",  pmp, read_pmpaddr, write_pmpaddr };
    csr_ops[CSR_PMPADDR10]  = { "pmpaddr10", pmp, read_pmpaddr, write_pmpaddr };
    csr_ops[CSR_PMPADDR11]  = { "pmpaddr11", pmp, read_pmpaddr, write_pmpaddr };
    csr_ops[CSR_PMPADDR12]  = { "pmpaddr12", pmp, read_pmpaddr, write_pmpaddr };
    csr_ops[CSR_PMPADDR13]  = { "pmpaddr13", pmp, read_pmpaddr, write_pmpaddr };
    csr_ops[CSR_PMPADDR14] =  { "pmpaddr14", pmp, read_pmpaddr, write_pmpaddr };
    csr_ops[CSR_PMPADDR15] =  { "pmpaddr15", pmp, read_pmpaddr, write_pmpaddr };

    csr_ops[CSR_MCOUNTINHIBIT]  = { "mcountinhibit",  any, read_mcountinhibit,
                             write_mcountinhibit};

    for (int i = 0; i < 29; i++) {
        csr_ops[CSR_HPMCOUNTER3+i] = { "hpmcounter", any, read_noop,
                                 write_noop  };
        csr_ops[CSR_MHPMCOUNTER3+i] = { "mhpmcounter", any, read_noop,
                                 write_noop  };
    }
}

}
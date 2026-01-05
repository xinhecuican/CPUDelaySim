#include "arch/riscv/riscvarch.h"
#include "arch/riscv/trans.h"
#include "arch/riscv/csrdefines.h"
#include "common/log.h"
#include "config.h"

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

int RiscvArch::decode(uint64_t paddr, DecodeInfo* info) {
    bool rvc;
    bool decode_valid;
    int size;
    fetch(paddr, &info->inst, rvc, &info->inst_size);
    env->pc = paddr;
    env->info = info;
    memset(info, 0, DEC_MEMSET_END);
    env->info->exception = EXC_NONE;
    decode_valid = interpreter(env, info->inst, rvc);
    if(unlikely(!decode_valid)) {
        spdlog::error("RiscvArch::decode: invalid instruction at 0x{:x}", paddr);
    }
    return size;
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
        } else if (type == FETCH_TYPE::LFETCH) {
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



void RiscvArch::translateAddr(uint64_t vaddr, FETCH_TYPE type, uint64_t& paddr, bool& exception) {
    if (type == FETCH_TYPE::IFETCH && !ifetch_mmu_state ||
        type != FETCH_TYPE::IFETCH && !data_mmu_state) {
        paddr = vaddr;
        exception = false;
        return;
    }
    paddr = (state->satp & 0xfffffffffff) << PGSHFT;
    int max_level = 3 + ((state->satp >> 60) == 8);
    PTE pte;
    int level;
    uint64_t p_pte;
    for (level = max_level - 1; level >= 0; level--) {
        p_pte = paddr + VPNi(vaddr, level) * PTE_SIZE;
        if(!paddrRead(p_pte, PTE_SIZE, type, (uint8_t*)(&pte.val))) {
            exception = true;
            return;
        }
        paddr = PGBASE((uint64_t)pte.ppn);
        if (!pte.v || (!pte.r && pte.w) || pte.pad) {
            exception = true;
            return;
        }
        if (pte.r || pte.x) break;
        else {
            if (pte.a || pte.d || pte.u || pte.pbmt || pte.n) {
                exception = true;
                return;
            }
            level--;
            if (unlikely(level < 0)) {
                exception = true;
                return;
            }
        }
    }
    if (!checkPermission(pte, true, vaddr, type)) {
        exception = true;
        return;
    }
    
    uint64_t pg_mask = ((1ull << VPNiSHFT(level)) - 1);
    if (level > 0) {
        if ((paddr & pg_mask) != 0) {
            exception = true;
            return;
        }
    }
    paddr = (paddr & ~pg_mask) | (vaddr & pg_mask);
}



inline void RiscvArch::fetch(uint64_t paddr, uint32_t* inst, bool& rvc, uint8_t* size) {
    bool mmio;
    memory->paddrRead(paddr, 4, (uint8_t*)inst, mmio);
    if (unlikely(mmio)) {
        spdlog::error("RiscvArch::fetch: mmio fetch at 0x{:x}", paddr);
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
    uint64_t exception_mask = (1 << exception_idx) - 1;
    uint64_t irq = (int32_t)info->exception < 0;
    bool edeleg_valid = ((state->medeleg & exception_mask) != 0) && (state->priv != MODE_M) ||
                        (irq && (state->mideleg & exception_mask) != 0);
#ifdef LOG_INST
    Log::trace("inst", "pc: 0x{:x}, inst: {:8x}", env->pc, info->inst);
#endif
    switch (info->exception) {
        case EXC_II:
        case EXC_BP:
        case EXC_LAM:
        case EXC_LAF:
        case EXC_SAM:
        case EXC_SAF:
        case EXC_DT:
        case EXC_SC:
        case EXC_HE:
            spdlog::error("exception {} at 0x{:x}", info->exception, env->pc);
            exit(1);
        case EXC_IPF:
        case EXC_LPF:
        case EXC_SPF:
            if (edeleg_valid) state->stval = info->exc_data;
            else state->mtval = info->exc_data;
        case EXC_ECU:
        case EXC_ECS:
        case EXC_ECM:
            if (edeleg_valid) {
                state->sepc = env->pc;
                state->scause = exception_idx | (irq << 63ULL);
                state->mstatus = (state->mstatus & ~(MSTATUS_SPP | MSTATUS_SPIE | MSTATUS_SIE)) | (state->priv << MSTATUS_SPP_SHIFT) | ((state->mstatus << 4) & MSTATUS_SPIE);
            } else {
                state->mepc = env->pc;
                state->mcause = exception_idx | (irq << 63ULL);
                state->mstatus = (state->mstatus & ~(MSTATUS_MPP | MSTATUS_MPIE | MSTATUS_MIE)) | (state->priv << MSTATUS_MPP_SHIFT) | ((state->mstatus << 4) & MSTATUS_MPIE);
            }
            goto mtvec_out; 
        case EXC_SRET:
            state->mstatus = (state->mstatus & ~(MSTATUS_SPP | MSTATUS_SIE | MSTATUS_MPRV)) | ((state->mstatus >> 4) & MSTATUS_SIE) | MSTATUS_SPIE;
            return state->sepc;
        case EXC_MRET:
            state->mstatus = (state->mstatus & ~(MSTATUS_MPP | MSTATUS_MIE)) | ((state->mstatus >> 4) & MSTATUS_MIE) | MSTATUS_MPIE | 
            ~(MSTATUS_MPRV & ((((state->mstatus >> MSTATUS_MPP_SHIFT) & 0x3ULL) != MODE_M) << MSTATUS_MPRV_SHIFT));
            return state->mepc;
        case EXC_NONE:
            for (int i = 0; i < 3; i++) {
                    uint64_t* dst_addr = (uint64_t*)(((uint8_t*)state)+info->dst_idx[i]);
                    *dst_addr = ((*dst_addr) & ~info->dst_mask[i]) | (info->dst_data[i] & info->dst_mask[i]);
            }
            ifetch_mmu_state = (state->satp & SATP64_MODE) == 8 && (state->priv != MODE_M);
            data_mmu_state = (state->satp & SATP64_MODE) == 8 && (((state->mstatus & MSTATUS_MPRV) ? 
                                                (state->mstatus & MSTATUS_MPP) : state->priv) != MODE_M);
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
            spdlog::error("RiscvArch::updateEnv: unknown exception {} at 0x{:x}", info->exception, env->pc);
            exit(1);
    }

mtvec_out:;
    uint64_t mtvec_mode = irq && (state->mtvec & 3);
    if (mtvec_mode == 0) {
        return state->mtvec;
    } else {
        return (state->mtvec & 0xfffffffffffffffc) + ((state->mcause & 0x1f) << 2);
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
    *val = ctx->state->fp_status.float_exception_flags;
}

csrw_func_def(fflags) {
    uint64_t dest = val & FSR_AEXC;
    ctx->info->dst_idx[1] = ARCH_FFLAGS;
    ctx->info->dst_mask[1] = FSR_AEXC;
    ctx->info->dst_data[1] = dest;

    ctx->info->dst_idx[2] = ARCH_MSTATUS;
    ctx->info->dst_mask[2] = MSTATUS_FS;
    ctx->info->dst_data[2] = MSTATUS_FS;
}

csrr_func_def(frm) {
    *val = ctx->state->frm;
}

csrw_func_def(frm) {
    uint64_t dest = (val & FSR_RD) >> FSR_RD_SHIFT;
    ctx->info->dst_idx[1] = ARCH_FRM;
    ctx->info->dst_mask[1] = FSR_RD;
    ctx->info->dst_data[1] = dest;

    ctx->info->dst_idx[2] = ARCH_MSTATUS;
    ctx->info->dst_mask[2] = MSTATUS_FS;
    ctx->info->dst_data[2] = MSTATUS_FS;
}

csrr_func_def(fcsr) {
    *val = ctx->state->fp_status.float_exception_flags | (ctx->state->frm << FSR_RD_SHIFT);
}

csrw_func_def(fcsr) {
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
    *val = (ctx->state->mstatus & MSTATUS_MASK) | 
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
    *val = (ctx->state->mstatus & SSTATUS_MASK) | 
           ((uint64_t)((ctx->state->mstatus & MSTATUS_FS) == MSTATUS_FS) << MSTATUS64_SD);
}

csrw_func_def(sstatus) {
    ctx->info->dst_idx[1] = ARCH_MSTATUS;
    ctx->info->dst_mask[1] = SSTATUS_MASK;
    ctx->info->dst_data[1] = val;
}

csrrmw_func_def(sie) {
    uint64_t nalias_mask = (S_MODE_INTERRUPTS | LOCAL_INTERRUPTS) &
        (~ctx->state->mideleg);
    uint64_t alias_mask = (S_MODE_INTERRUPTS | LOCAL_INTERRUPTS) & ctx->state->mideleg;
    uint64_t sie_mask = wr_mask & nalias_mask;
    if (ret_val) *ret_val = ctx->state->sie & alias_mask;
    ctx->info->dst_idx[1] = ARCH_SIE;
    ctx->info->dst_mask[1] = all_ints & wr_mask & nalias_mask;
    ctx->info->dst_data[1] = new_val;
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
    uint64_t mask = (ctx->state->mideleg) & sip_writable_mask;
    if (ret_val) *ret_val = ctx->state->mip & ctx->state->mideleg & (S_MODE_INTERRUPTS | LOCAL_INTERRUPTS);
    ctx->info->dst_idx[1] = ARCH_MIP;
    ctx->info->dst_mask[1] = wr_mask & mask;
    ctx->info->dst_data[1] = new_val;
}

csrr_func_def(satp) {
    *val = ctx->state->satp;
}

csrw_func_def(satp) {
    ctx->info->dst_idx[1] = ARCH_SATP;
    ctx->info->dst_mask[1] = 0xffffffffffffffff;
    ctx->info->dst_data[1] = val;
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
    ctx->info->dst_mask[1] = 0xffffffffffffffff;
    ctx->info->dst_data[1] = val;
}

csrr_func_def(mcountinhibit) {
    *val = ctx->state->mcountinhibit;
}

csrw_func_def(mcountinhibit) {
    ctx->info->exception = EXC_II;
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
}

}
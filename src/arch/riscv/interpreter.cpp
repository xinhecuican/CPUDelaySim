#include "common/common.h"
#include "arch/riscv/trans.h"
#include "arch/riscv/csrdefines.h"
#include "common/log.h"
#include <stdio.h>

namespace cds::arch::riscv {

static int ex_plus_1(DisasContext *ctx, int nf)
{
    return nf + 1;
}

#define EX_SH(amount) \
    __attribute__((unused)) static int ex_shift_##amount(DisasContext *ctx, int imm) \
    {                                         \
        return imm << amount;                 \
    }
EX_SH(1)
EX_SH(2)
EX_SH(3)
EX_SH(4)
EX_SH(12)

#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif
static inline void cpu_put_ic(ArchEnv *env, bool (*trans_func)(void*, void*), void* arg, uint32_t insn) {

}
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic pop
#endif

static int ex_rvc_register(DisasContext *ctx, int reg) {
    return reg + 8;
}

static int ex_sreg_register(DisasContext *ctx, int reg) {
    return reg + (8 << (reg < 2));
}

static int ex_rvc_shiftli(DisasContext *ctx, int imm) {
    return imm;
}

static int ex_rvc_shiftri(DisasContext *ctx, int imm) {
    return imm;
}

#define g_assert_not_reached abort
#include "arch/riscv/trans_rv32.h"
#include "arch/riscv/trans_rv16.h"

extern riscv_csr_operations csr_ops[CSR_TABLE_SIZE];

typedef int64_t TCGv;
typedef int32_t TCGv_i32;
typedef int64_t TCGv_i64;
# define INT64_MIN		(-__INT64_C(9223372036854775807)-1)
#define __NOT_IMPLEMENTED__ do {printf("LA_EMU NOT IMPLEMENTED %s\n", __func__); return false;} while(0);
#define __NOT_CORRECTED_IMPLEMENTED__ do {printf("LA_EMU NOT CORRECTED IMPLEMENTED %s\n", __func__);} while(0);
#define __NOT_IMPLEMENTED_EXIT__ do {printf("LA_EMU NOT IMPLEMENTED %s\n", __func__); exit(1); return false;} while(0);

static int64_t get_gpr_32(DisasContext *ctx, int reg_num, DisasExtend ext) {
    switch (ext) {
        case EXT_NONE:
            return ctx->state->gpr[reg_num];
        case EXT_SIGN:
            return (int32_t)ctx->state->gpr[reg_num];
        case EXT_ZERO:
            return (uint32_t)ctx->state->gpr[reg_num];
    }
    return 0;
}

static int64_t get_gpr_64(DisasContext *ctx, int reg_num, DisasExtend ext)
{
    return ctx->state->gpr[reg_num];
}

static void gen_set_gpr_32(DisasContext *ctx, int reg_num, int64_t t) {
    if (reg_num != 0) {
        ctx->state->gpr[reg_num] = (int32_t)t;
    }
}

static void gen_set_gpr_64(DisasContext *ctx, int reg_num, int64_t t) {
    if (reg_num != 0) {
        ctx->state->gpr[reg_num] = t;
    }
}

static void gen_set_gpri_32(DisasContext *ctx, int reg_num, int64_t imm) {
    if (reg_num != 0) {
        ctx->state->gpr[reg_num] = (int32_t)imm;
    }
}

static void gen_set_gpri_64(DisasContext *ctx, int reg_num, int64_t imm) {
    if (reg_num != 0) {
        ctx->state->gpr[reg_num] = imm;
    }
}

static uint64_t get_fpr_hs(DisasContext *ctx, int reg_num) {
    return ctx->state->fpr[reg_num];
}

static uint64_t get_fpr_d(DisasContext *ctx, int reg_num) {
    return ctx->state->fpr[reg_num];
}

static void gen_set_fpr_hs(DisasContext *ctx, int reg_num, int64_t t) {
    ctx->state->fpr[reg_num] = t;
}

static void gen_set_fpr_d(DisasContext *ctx, int reg_num, int64_t t) {
    ctx->state->fpr[reg_num] = t;
}

static uint64_t get_address(DisasContext *ctx, int rs1, int imm) {
    return get_gpr_64(ctx, rs1, EXT_NONE) + imm;
}

static uint64_t gen_pc_plus_diff(DisasContext* ctx, uint64_t diff) {
    return ctx->pc + diff;
}

static int8_t ld_b(DisasContext* ctx, uint64_t va) {
    uint64_t ha;
    uint64_t exception;
    ctx->arch->translateAddr(va, FETCH_TYPE::LFETCH, ha, exception);
    if (unlikely(exception)) {
        ctx->info->exception = EXC_LPF;
        return 0;
    }
    uint8_t data;
    ctx->arch->paddrRead(ha, 1, FETCH_TYPE::LFETCH, &data);
    return data;
}

static int16_t ld_h(DisasContext* ctx, uint64_t va) {
    if (unlikely(va & 0x1)) {
        ctx->info->exception = EXC_LAM;
        return 0;
    }
    uint64_t ha;
    uint64_t exception;
    ctx->arch->translateAddr(va, FETCH_TYPE::LFETCH, ha, exception);
    if (unlikely(exception)) {
        ctx->info->exception = EXC_LPF;
        return 0;
    }
    uint16_t data;
    ctx->arch->paddrRead(ha, 2, FETCH_TYPE::LFETCH, (uint8_t*)&data);
    return data;
}

static int32_t ld_w(DisasContext* ctx, uint64_t va) {
    if (unlikely(va & 0x3)) {
        ctx->info->exception = EXC_LAM;
        return 0;
    }
    uint64_t ha;
    uint64_t exception;
    ctx->arch->translateAddr(va, FETCH_TYPE::LFETCH, ha, exception);
    if (unlikely(exception)) {
        ctx->info->exception = EXC_LPF;
        return 0;
    }
    uint32_t data;
    ctx->arch->paddrRead(ha, 4, FETCH_TYPE::LFETCH, (uint8_t*)&data);
    return data;
}

static int64_t ld_d(DisasContext* ctx, uint64_t va) {
    if (unlikely(va & 0x7)) {
        ctx->info->exception = EXC_LAM;
        return 0;
    }
    uint64_t ha;
    uint64_t exception;
    ctx->arch->translateAddr(va, FETCH_TYPE::LFETCH, ha, exception);
    if (unlikely(exception)) {
        ctx->info->exception = EXC_LPF;
        return 0;
    }
    uint64_t data;
    ctx->arch->paddrRead(ha, 8, FETCH_TYPE::LFETCH, (uint8_t*)&data);
    return data;
}

static bool st_b(DisasContext* ctx, uint64_t va, int8_t data) {
    uint64_t ha;
    uint64_t exception;
    ctx->arch->translateAddr(va, FETCH_TYPE::SFETCH, ha, exception);
    if (unlikely(exception)) {
        ctx->info->exception = EXC_SPF;
        return false;
    }
    ctx->info->dst_idx[2] = 1;
    ctx->info->dst_data[2] = ha;
    return true;
}

static bool st_h(DisasContext* ctx, uint64_t va, int16_t data) {
    if (unlikely(va & 0x1)) {
        ctx->info->exception = EXC_SAM;
        return false;
    }
    uint64_t ha;
    uint64_t exception;
    ctx->arch->translateAddr(va, FETCH_TYPE::SFETCH, ha, exception);
    if (unlikely(exception)) {
        ctx->info->exception = EXC_SPF;
        return false;
    }
    ctx->info->dst_idx[2] = 2;
    ctx->info->dst_data[2] = ha;
    return true;
}

static bool st_w(DisasContext* ctx, uint64_t va, int32_t data) {
    if (unlikely(va & 0x3)) {
        ctx->info->exception = EXC_SAM;
        return false;
    }
    uint64_t ha;
    uint64_t exception;
    ctx->arch->translateAddr(va, FETCH_TYPE::SFETCH, ha, exception);
    if (unlikely(exception)) {
        ctx->info->exception = EXC_SPF;
        return false;
    }
    ctx->info->dst_idx[2] = 4;
    ctx->info->dst_data[2] = ha;
    return true;
}

static bool st_d(DisasContext* ctx, uint64_t va, int64_t data) {
    if (unlikely(va & 0x7)) {
        ctx->info->exception = EXC_SAM;
        return false;
    }
    uint64_t ha;
    uint64_t exception;
    ctx->arch->translateAddr(va, FETCH_TYPE::SFETCH, ha, exception);
    if (unlikely(exception)) {
        ctx->info->exception = EXC_SPF;
        return false;
    }
    ctx->info->dst_idx[2] = 8;
    ctx->info->dst_data[2] = ha;
    return true;
}



#define SET_GPR_64 \
    ctx->info->dst_idx[0] = a->rd * 8; \
    ctx->info->dst_mask[0] = 0xffffffffffffffff; \
    ctx->info->dst_data[0] = dest;

#define SET_GPR_32 \
    ctx->info->dst_idx[0] = a->rd * 8; \
    ctx->info->dst_mask[0] = 0xffffffffffffffff; \
    ctx->info->dst_data[0] = (int32_t)dest;

#define SET_FPR_64 \
    ctx->info->dst_idx[0] = a->rd * 8 + ARCH_FPR; \
    ctx->info->dst_mask[0] = 0xffffffffffffffff; \
    ctx->info->dst_data[0] = dest;

#define SET_SRC2 \
    ctx->info->src_reg[0] = a->rs1; \
    ctx->info->src_reg[1] = a->rs2; 

#define SET_SRC0_DST \
    ctx->info->dst_reg = a->rd; 

#define SET_SRC1_DST \
    ctx->info->src_reg[0] = a->rs1; \
    ctx->info->dst_reg = a->rd; 

#define SET_SRC2_DST \
    ctx->info->src_reg[0] = a->rs1; \
    ctx->info->src_reg[1] = a->rs2; \
    ctx->info->dst_reg = a->rd; 
#define SET_SRC2_DST_ADDR \
    ctx->info->src_reg[0] = a->rs1; \
    ctx->info->src_reg[1] = a->rs2; \
    ctx->info->dst_reg = a->rd;
#define SET_SRC1_DSTF \
    ctx->info->src_reg[0] = a->rs1; \
    ctx->info->dst_reg = a->rd | DSTF_REG_MASK; 
#define SET_SRC1F_DST \
    ctx->info->src_reg[0] = a->rs1 | DSTF_REG_MASK; \
    ctx->info->dst_reg = a->rd;
#define SET_SRC1F_DSTF \
    ctx->info->src_reg[0] = a->rs1 | DSTF_REG_MASK; \
    ctx->info->dst_reg = a->rd | DSTF_REG_MASK; 
#define SET_SRC2F_DSTF \
    ctx->info->src_reg[0] = a->rs1 | DSTF_REG_MASK; \
    ctx->info->src_reg[1] = a->rs2 | DSTF_REG_MASK; \
    ctx->info->dst_reg = a->rd | DSTF_REG_MASK;
#define SET_SRC2F_DST \
    ctx->info->src_reg[0] = a->rs1 | DSTF_REG_MASK; \
    ctx->info->src_reg[1] = a->rs2 | DSTF_REG_MASK; \
    ctx->info->dst_reg = a->rd;
#define SET_SRC3F_DSTF \
    ctx->info->src_reg[0] = a->rs1 | DSTF_REG_MASK; \
    ctx->info->src_reg[1] = a->rs2 | DSTF_REG_MASK; \
    ctx->info->src_reg[2] = a->rs3 | DSTF_REG_MASK; \
    ctx->info->dst_reg = a->rd | DSTF_REG_MASK;
#define SET_SRC1_DSTF_ADDR \
    ctx->info->src_reg[0] = a->rs1; \
    ctx->info->dst_reg = a->rd | DSTF_REG_MASK;
#define SET_SRC1_SRC2F_ADDR \
    ctx->info->src_reg[0] = a->rs1; \
    ctx->info->src_reg[1] = a->rs2 | DSTF_REG_MASK;
#define SET_SRC1_DST_ADDR \
    ctx->info->src_reg[0] = a->rs1; \
    ctx->info->dst_reg = a->rd;
#define SET_SRC1_SRC2_ADDR \
    ctx->info->src_reg[0] = a->rs1; \
    ctx->info->src_reg[1] = a->rs2;


static bool trans_addd(DisasContext *ctx, arg_addd *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_addid(DisasContext *ctx, arg_addid *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_subd(DisasContext *ctx, arg_subd *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_slld(DisasContext *ctx, arg_slld *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_sllid(DisasContext *ctx, arg_sllid *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_srad(DisasContext *ctx, arg_srad *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_sraid(DisasContext *ctx, arg_sraid *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_srld(DisasContext *ctx, arg_srld *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_muld(DisasContext *ctx, arg_muld *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_srlid(DisasContext *ctx, arg_srlid *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_divd(DisasContext *ctx, arg_divd *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_divud(DisasContext *ctx, arg_divud *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_remud(DisasContext *ctx, arg_remud *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_remd(DisasContext *ctx, arg_remd *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_slli_uw(DisasContext *ctx, arg_slli_uw *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_ldu(DisasContext *ctx, arg_ldu *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_lq(DisasContext *ctx, arg_lq *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_sq(DisasContext *ctx, arg_sq *a) {__NOT_IMPLEMENTED_EXIT__}

static bool trans_addi(DisasContext *ctx, arg_addi *a) {
    TCGv src1 = get_gpr_64(ctx, a->rs1, EXT_NONE);
    TCGv dest = src1 + a->imm;
    SET_GPR_64
    SET_SRC1_DST
    ctx->info->type = INT;
    return true;
}
static bool trans_addiw(DisasContext *ctx, arg_addiw *a) {
    TCGv src1 = get_gpr_32(ctx, a->rs1, EXT_NONE);
    TCGv dest = src1 + a->imm;
    SET_GPR_32
    SET_SRC1_DST
    ctx->info->type = INT;
    return true;
}
static bool trans_addw(DisasContext *ctx, arg_addw *a) {
    TCGv src1 = get_gpr_32(ctx, a->rs1, EXT_NONE);
    TCGv src2 = get_gpr_32(ctx, a->rs2, EXT_NONE);
    TCGv dest = (int32_t)src1 + (int32_t)src2;
    SET_GPR_32
    SET_SRC2_DST
    ctx->info->type = INT;
    return true;
}
static bool trans_subw(DisasContext *ctx, arg_subw *a) {
    TCGv src1 = get_gpr_32(ctx, a->rs1, EXT_NONE);
    TCGv src2 = get_gpr_32(ctx, a->rs2, EXT_NONE);
    TCGv dest = (int32_t)src1 - (int32_t)src2;
    SET_GPR_32
    SET_SRC2_DST
    ctx->info->type = INT;
    return true;
}
static bool trans_slliw(DisasContext *ctx, arg_slliw *a) {
    TCGv src1 = get_gpr_32(ctx, a->rs1, EXT_NONE);
    TCGv dest = src1 << a->shamt;
    SET_GPR_32
    SET_SRC1_DST
    ctx->info->type = INT;
    return true;
}
static bool trans_sllw(DisasContext *ctx, arg_sllw *a) {
    TCGv src1 = get_gpr_32(ctx, a->rs1, EXT_NONE);
    TCGv src2 = get_gpr_32(ctx, a->rs2, EXT_NONE);
    TCGv dest = src1 << (src2 & 0x1f);
    SET_GPR_32
    SET_SRC2_DST
    ctx->info->type = INT;
    return true;
}
static bool trans_srliw(DisasContext *ctx, arg_srliw *a) {
    TCGv src1 = get_gpr_32(ctx, a->rs1, EXT_NONE);
    TCGv dest = (uint32_t)src1 >> a->shamt;
    SET_GPR_32
    SET_SRC1_DST
    ctx->info->type = INT;
    return true;
}
static bool trans_srlw(DisasContext *ctx, arg_srlw *a) {
    TCGv src1 = get_gpr_32(ctx, a->rs1, EXT_NONE);
    TCGv src2 = get_gpr_32(ctx, a->rs2, EXT_NONE);
    TCGv dest = (uint32_t)src1 >> (src2 & 0x1f);
    SET_GPR_32
    SET_SRC2_DST
    ctx->info->type = INT;
    return true;
}
static bool trans_sraiw(DisasContext *ctx, arg_sraiw *a) {
    TCGv src1 = get_gpr_32(ctx, a->rs1, EXT_NONE);
    TCGv dest = (int32_t)src1 >> a->shamt;
    SET_GPR_32
    SET_SRC1_DST
    ctx->info->type = INT;
    return true;
}
static bool trans_sraw(DisasContext *ctx, arg_sraw *a) {
    TCGv src1 = get_gpr_32(ctx, a->rs1, EXT_NONE);
    TCGv src2 = get_gpr_32(ctx, a->rs2, EXT_NONE);
    TCGv dest = (int32_t)src1 >> (src2 & 0x1f);
    SET_GPR_32
    SET_SRC2_DST
    ctx->info->type = INT;
    return true;
}
static bool trans_add(DisasContext *ctx, arg_add *a) {
    TCGv src1 = get_gpr_64(ctx, a->rs1, EXT_NONE);
    TCGv src2 = get_gpr_64(ctx, a->rs2, EXT_NONE);
    TCGv dest = src1 + src2;
    SET_GPR_64
    SET_SRC2_DST
    ctx->info->type = INT;
    return true;
}
static bool trans_sub(DisasContext *ctx, arg_sub *a) {
    TCGv src1 = get_gpr_64(ctx, a->rs1, EXT_NONE);
    TCGv src2 = get_gpr_64(ctx, a->rs2, EXT_NONE);
    TCGv dest = src1 - src2;
    SET_GPR_64
    SET_SRC2_DST
    ctx->info->type = INT;
    return true;
}
// rvb
static bool trans_add_uw(DisasContext *ctx, arg_add_uw *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_orn(DisasContext *ctx, arg_orn *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_xnor(DisasContext *ctx, arg_xnor *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_andn(DisasContext *ctx, arg_andn *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_bclr(DisasContext *ctx, arg_bclr *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_bclri(DisasContext *ctx, arg_bclri *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_bext(DisasContext *ctx, arg_bext *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_bexti(DisasContext *ctx, arg_bexti *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_binv(DisasContext *ctx, arg_binv *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_binvi(DisasContext *ctx, arg_binvi *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_brev8(DisasContext *ctx, arg_brev8 *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_bset(DisasContext *ctx, arg_bset *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_bseti(DisasContext *ctx, arg_bseti *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_max(DisasContext *ctx, arg_max *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_maxu(DisasContext *ctx, arg_maxu *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_min(DisasContext *ctx, arg_min *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_minu(DisasContext *ctx, arg_minu *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_orc_b(DisasContext *ctx, arg_orc_b *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_pack(DisasContext *ctx, arg_pack *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_packh(DisasContext *ctx, arg_packh *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_packw(DisasContext *ctx, arg_packw *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_rev8_32(DisasContext *ctx, arg_rev8_32 *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_rev8_64(DisasContext *ctx, arg_rev8_64 *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_rol(DisasContext *ctx, arg_rol *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_rolw(DisasContext *ctx, arg_rolw *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_ror(DisasContext *ctx, arg_ror *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_rori(DisasContext *ctx, arg_rori *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_roriw(DisasContext *ctx, arg_roriw *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_rorw(DisasContext *ctx, arg_rorw *a) {__NOT_IMPLEMENTED_EXIT__}

//aes
static bool trans_aes32dsi(DisasContext *ctx, arg_aes32dsi *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_aes32dsmi(DisasContext *ctx, arg_aes32dsmi *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_aes32esi(DisasContext *ctx, arg_aes32esi *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_aes32esmi(DisasContext *ctx, arg_aes32esmi *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_aes64ds(DisasContext *ctx, arg_aes64ds *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_aes64dsm(DisasContext *ctx, arg_aes64dsm *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_aes64es(DisasContext *ctx, arg_aes64es *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_aes64esm(DisasContext *ctx, arg_aes64esm *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_aes64im(DisasContext *ctx, arg_aes64im *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_aes64ks1i(DisasContext *ctx, arg_aes64ks1i *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_aes64ks2(DisasContext *ctx, arg_aes64ks2 *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_amoadd_b(DisasContext *ctx, arg_amoadd_b *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_amoadd_h(DisasContext *ctx, arg_amoadd_h *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_amoadd_w(DisasContext *ctx, arg_amoadd_w *a) {
    uint64_t addr = get_address(ctx, a->rs1, 0);
    ctx->info->exc_data = addr;
    uint64_t dest = ld_w(ctx, addr);
    uint64_t data = get_gpr_32(ctx, a->rs2, EXT_NONE);
    uint32_t res = dest + data;
    st_w(ctx, addr, res);
    ctx->info->dst_data[1] = res;
    SET_GPR_32
    SET_SRC2_DST_ADDR
    ctx->info->type = AMO;
    return true;
}
static bool trans_amoadd_d(DisasContext *ctx, arg_amoadd_d *a) {
    uint64_t addr = get_address(ctx, a->rs1, 0);
    ctx->info->exc_data = addr;
    uint64_t dest = ld_d(ctx, addr);
    uint64_t data = get_gpr_64(ctx, a->rs2, EXT_NONE);
    uint64_t res = dest + data;
    st_d(ctx, addr, res);
    ctx->info->dst_data[1] = res;
    SET_GPR_64
    SET_SRC2_DST_ADDR
    ctx->info->type = AMO;
    return true;
}
static bool trans_amoand_b(DisasContext *ctx, arg_amoand_b *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_amoand_h(DisasContext *ctx, arg_amoand_h *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_amoand_w(DisasContext *ctx, arg_amoand_w *a) {
    uint64_t addr = get_address(ctx, a->rs1, 0);
    ctx->info->exc_data = addr;
    uint64_t dest = ld_w(ctx, addr);
    uint64_t data = get_gpr_32(ctx, a->rs2, EXT_NONE);
    uint32_t res = dest & data;
    st_w(ctx, addr, res);
    ctx->info->dst_data[1] = res;
    SET_GPR_32
    SET_SRC2_DST_ADDR
    ctx->info->type = AMO;
    return true;
}
static bool trans_amoand_d(DisasContext *ctx, arg_amoand_d *a) {
    uint64_t addr = get_address(ctx, a->rs1, 0);
    ctx->info->exc_data = addr;
    uint64_t dest = ld_d(ctx, addr);
    uint64_t data = get_gpr_64(ctx, a->rs2, EXT_NONE);
    uint64_t res = dest & data;
    st_d(ctx, addr, res);
    ctx->info->dst_data[1] = res;
    SET_GPR_64
    SET_SRC2_DST_ADDR
    ctx->info->type = AMO;
    return true;
}
static bool trans_amoor_b(DisasContext *ctx, arg_amoor_b *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_amoor_h(DisasContext *ctx, arg_amoor_h *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_amoor_w(DisasContext *ctx, arg_amoor_w *a) {
    uint64_t addr = get_address(ctx, a->rs1, 0);
    ctx->info->exc_data = addr;
    uint64_t dest = ld_w(ctx, addr);
    uint64_t data = get_gpr_32(ctx, a->rs2, EXT_NONE);
    uint32_t res = dest | data;
    st_w(ctx, addr, res);
    ctx->info->dst_data[1] = res;
    SET_GPR_32
    SET_SRC2_DST_ADDR
    ctx->info->type = AMO;
    return true;
}
static bool trans_amoor_d(DisasContext *ctx, arg_amoor_d *a) {
    uint64_t addr = get_address(ctx, a->rs1, 0);
    ctx->info->exc_data = addr;
    uint64_t dest = ld_d(ctx, addr);
    uint64_t data = get_gpr_64(ctx, a->rs2, EXT_NONE);
    uint64_t res = dest | data;
    st_d(ctx, addr, res);
    ctx->info->dst_data[1] = res;
    SET_GPR_64
    SET_SRC2_DST_ADDR
    ctx->info->type = AMO;
    return true;
}
static bool trans_amoxor_b(DisasContext *ctx, arg_amoxor_b *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_amoxor_h(DisasContext *ctx, arg_amoxor_h *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_amoxor_w(DisasContext *ctx, arg_amoxor_w *a) {
    uint64_t addr = get_address(ctx, a->rs1, 0);
    ctx->info->exc_data = addr;
    uint64_t dest = ld_w(ctx, addr);
    uint64_t data = get_gpr_32(ctx, a->rs2, EXT_NONE);
    uint32_t res = dest ^ data;
    st_w(ctx, addr, res);
    ctx->info->dst_data[1] = res;
    SET_GPR_32
    SET_SRC2_DST_ADDR
    ctx->info->type = AMO;
    return true;
}
static bool trans_amoxor_d(DisasContext *ctx, arg_amoxor_d *a) {
    uint64_t addr = get_address(ctx, a->rs1, 0);
    ctx->info->exc_data = addr;
    uint64_t dest = ld_d(ctx, addr);
    uint64_t data = get_gpr_64(ctx, a->rs2, EXT_NONE);
    uint64_t res = dest ^ data;
    st_d(ctx, addr, res);
    ctx->info->dst_data[1] = res;
    SET_GPR_64
    SET_SRC2_DST_ADDR
    ctx->info->type = AMO;
    return true;
}
static bool trans_amocas_b(DisasContext *ctx, arg_amocas_b *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_amocas_h(DisasContext *ctx, arg_amocas_h *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_amocas_w(DisasContext *ctx, arg_amocas_w *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_amocas_d(DisasContext *ctx, arg_amocas_d *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_amocas_q(DisasContext *ctx, arg_amocas_q *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_amomax_b(DisasContext *ctx, arg_amomax_b *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_amomax_h(DisasContext *ctx, arg_amomax_h *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_amomax_w(DisasContext *ctx, arg_amomax_w *a) {
    uint64_t addr = get_address(ctx, a->rs1, 0);
    ctx->info->exc_data = addr;
    uint64_t dest = ld_w(ctx, addr);
    uint64_t data = get_gpr_32(ctx, a->rs2, EXT_NONE);
    uint32_t res = std::max((int32_t)dest, (int32_t)data);
    st_w(ctx, addr, res);
    ctx->info->dst_data[1] = res;
    SET_GPR_32
    SET_SRC2_DST_ADDR
    ctx->info->type = AMO;
    return true;
}
static bool trans_amomax_d(DisasContext *ctx, arg_amomax_d *a) {
    uint64_t addr = get_address(ctx, a->rs1, 0);
    ctx->info->exc_data = addr;
    uint64_t dest = ld_d(ctx, addr);
    uint64_t data = get_gpr_64(ctx, a->rs2, EXT_NONE);
    uint64_t res = std::max((int64_t)dest, (int64_t)data);
    st_d(ctx, addr, res);
    ctx->info->dst_data[1] = res;
    SET_GPR_64
    SET_SRC2_DST_ADDR
    ctx->info->type = AMO;
    return true;
}
static bool trans_amomaxu_b(DisasContext *ctx, arg_amomaxu_b *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_amomaxu_h(DisasContext *ctx, arg_amomaxu_h *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_amomaxu_w(DisasContext *ctx, arg_amomaxu_w *a) {
    uint64_t addr = get_address(ctx, a->rs1, 0);
    ctx->info->exc_data = addr;
    uint64_t dest = ld_w(ctx, addr);
    uint64_t data = get_gpr_32(ctx, a->rs2, EXT_NONE);
    uint32_t res = std::max((uint32_t)dest, (uint32_t)data);
    st_w(ctx, addr, res);
    ctx->info->dst_data[1] = res;
    SET_GPR_32
    SET_SRC2_DST_ADDR
    ctx->info->type = AMO;
    return true;
}
static bool trans_amomaxu_d(DisasContext *ctx, arg_amomaxu_d *a) {
    uint64_t addr = get_address(ctx, a->rs1, 0);
    ctx->info->exc_data = addr;
    uint64_t dest = ld_d(ctx, addr);
    uint64_t data = get_gpr_64(ctx, a->rs2, EXT_NONE);
    uint64_t res = std::max((uint64_t)dest, (uint64_t)data);
    st_d(ctx, addr, res);
    ctx->info->dst_data[1] = res;
    SET_GPR_64
    SET_SRC2_DST_ADDR
    ctx->info->type = AMO;
    return true;
}
static bool trans_amomin_b(DisasContext *ctx, arg_amomin_b *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_amomin_h(DisasContext *ctx, arg_amomin_h *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_amomin_w(DisasContext *ctx, arg_amomin_w *a) {
    uint64_t addr = get_address(ctx, a->rs1, 0);
    ctx->info->exc_data = addr;
    uint64_t dest = ld_w(ctx, addr);
    uint64_t data = get_gpr_32(ctx, a->rs2, EXT_NONE);
    uint32_t res = std::min((int32_t)dest, (int32_t)data);
    st_w(ctx, addr, res);
    ctx->info->dst_data[1] = res;
    SET_GPR_32
    SET_SRC2_DST_ADDR
    ctx->info->type = AMO;
    return true;
}
static bool trans_amomin_d(DisasContext *ctx, arg_amomin_d *a) {
    uint64_t addr = get_address(ctx, a->rs1, 0);
    ctx->info->exc_data = addr;
    uint64_t dest = ld_d(ctx, addr);
    uint64_t data = get_gpr_64(ctx, a->rs2, EXT_NONE);
    uint64_t res = std::min((int64_t)dest, (int64_t)data);
    st_d(ctx, addr, res);
    ctx->info->dst_data[1] = res;
    SET_GPR_64
    SET_SRC2_DST_ADDR
    ctx->info->type = AMO;
    return true;
}
static bool trans_amominu_b(DisasContext *ctx, arg_amominu_b *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_amominu_h(DisasContext *ctx, arg_amominu_h *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_amominu_w(DisasContext *ctx, arg_amominu_w *a) {
    uint64_t addr = get_address(ctx, a->rs1, 0);
    ctx->info->exc_data = addr;
    uint64_t dest = ld_w(ctx, addr);
    uint64_t data = get_gpr_32(ctx, a->rs2, EXT_NONE);
    uint32_t res = std::min((uint32_t)dest, (uint32_t)data);
    st_w(ctx, addr, res);
    ctx->info->dst_data[1] = res;
    SET_GPR_32
    SET_SRC2_DST_ADDR
    ctx->info->type = AMO;
    return true;
}
static bool trans_amominu_d(DisasContext *ctx, arg_amominu_d *a) {
    uint64_t addr = get_address(ctx, a->rs1, 0);
    ctx->info->exc_data = addr;
    uint64_t dest = ld_d(ctx, addr);
    uint64_t data = get_gpr_64(ctx, a->rs2, EXT_NONE);
    uint64_t res = std::min((uint64_t)dest, (uint64_t)data);
    st_d(ctx, addr, res);
    ctx->info->dst_data[1] = res;
    SET_GPR_64
    SET_SRC2_DST_ADDR
    ctx->info->type = AMO;
    return true;
}
static bool trans_amoswap_b(DisasContext *ctx, arg_amoswap_b *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_amoswap_h(DisasContext *ctx, arg_amoswap_h *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_amoswap_w(DisasContext *ctx, arg_amoswap_w *a) {
    uint64_t addr = get_address(ctx, a->rs1, 0);
    ctx->info->exc_data = addr;
    uint64_t dest = ld_w(ctx, addr);
    uint64_t data = get_gpr_32(ctx, a->rs2, EXT_NONE);
    st_w(ctx, addr, data);
    ctx->info->dst_data[1] = (uint32_t)data;
    SET_GPR_32
    SET_SRC2_DST_ADDR
    ctx->info->type = AMO;
    return true;
}
static bool trans_amoswap_d(DisasContext *ctx, arg_amoswap_d *a) {
    uint64_t addr = get_address(ctx, a->rs1, 0);
    ctx->info->exc_data = addr;
    uint64_t dest = ld_d(ctx, addr);
    uint64_t data = get_gpr_64(ctx, a->rs2, EXT_NONE);
    st_d(ctx, addr, data);
    ctx->info->dst_data[1] = data;
    SET_GPR_64
    SET_SRC2_DST_ADDR
    ctx->info->type = AMO;
    return true;
}
static bool trans_and(DisasContext *ctx, arg_and *a) {
    uint64_t src1 = get_gpr_64(ctx, a->rs1, EXT_NONE);
    uint64_t src2 = get_gpr_64(ctx, a->rs2, EXT_NONE);
    uint64_t dest = src1 & src2;
    SET_GPR_64
    SET_SRC2_DST
    ctx->info->type = INT;
    return true;
}
static bool trans_andi(DisasContext *ctx, arg_andi *a) {
    uint64_t src1 = get_gpr_64(ctx, a->rs1, EXT_NONE);
    uint64_t dest = src1 & (int64_t)a->imm;
    SET_GPR_64
    SET_SRC1_DST
    ctx->info->type = INT;
    return true;
}
static bool trans_or(DisasContext *ctx, arg_or *a) {
    uint64_t src1 = get_gpr_64(ctx, a->rs1, EXT_NONE);
    uint64_t src2 = get_gpr_64(ctx, a->rs2, EXT_NONE);
    uint64_t dest = src1 | src2;
    SET_GPR_64
    SET_SRC2_DST
    ctx->info->type = INT;
    return true;
}
static bool trans_ori(DisasContext *ctx, arg_ori *a) {
    uint64_t src1 = get_gpr_64(ctx, a->rs1, EXT_NONE);
    uint64_t dest = src1 | (int64_t)a->imm;
    SET_GPR_64
    SET_SRC1_DST
    ctx->info->type = INT;
    return true;
}
static bool trans_xor(DisasContext *ctx, arg_xor *a) {
    uint64_t src1 = get_gpr_64(ctx, a->rs1, EXT_NONE);
    uint64_t src2 = get_gpr_64(ctx, a->rs2, EXT_NONE);
    uint64_t dest = src1 ^ src2;
    SET_GPR_64
    SET_SRC2_DST
    ctx->info->type = INT;
    return true;
}
static bool trans_xori(DisasContext *ctx, arg_xori *a) {
    uint64_t src1 = get_gpr_64(ctx, a->rs1, EXT_NONE);
    uint64_t dest = src1 ^ (int64_t)a->imm;
    SET_GPR_64
    SET_SRC1_DST
    ctx->info->type = INT;
    return true;
}
static bool trans_auipc(DisasContext *ctx, arg_auipc *a) {
    uint64_t dest = gen_pc_plus_diff(ctx, a->imm);
    SET_GPR_64
    SET_SRC0_DST
    ctx->info->type = INT;
    return true;
}
static bool trans_beq(DisasContext *ctx, arg_beq *a) {
    uint64_t src1 = get_gpr_64(ctx, a->rs1, EXT_NONE);
    uint64_t src2 = get_gpr_64(ctx, a->rs2, EXT_NONE);
    bool taken = src1 == src2;
    ctx->info->dst_data[1] = taken;
    ctx->info->dst_data[2] = gen_pc_plus_diff(ctx, a->imm);
    ctx->info->type = COND;
    SET_SRC2
    return true;
}
static bool trans_bne(DisasContext *ctx, arg_bne *a) {
    uint64_t src1 = get_gpr_64(ctx, a->rs1, EXT_NONE);
    uint64_t src2 = get_gpr_64(ctx, a->rs2, EXT_NONE);
    bool taken = src1 != src2;
    ctx->info->dst_data[1] = taken;
    ctx->info->dst_data[2] = gen_pc_plus_diff(ctx, a->imm);
    ctx->info->type = COND;
    SET_SRC2
    return true;
}
static bool trans_blt(DisasContext *ctx, arg_blt *a) {
    uint64_t src1 = get_gpr_64(ctx, a->rs1, EXT_NONE);
    uint64_t src2 = get_gpr_64(ctx, a->rs2, EXT_NONE);
    bool taken = (int64_t)src1 < (int64_t)src2;
    ctx->info->dst_data[1] = taken;
    ctx->info->dst_data[2] = gen_pc_plus_diff(ctx, a->imm);
    ctx->info->type = COND;
    SET_SRC2
    return true;
}
static bool trans_bltu(DisasContext *ctx, arg_bltu *a) {
    uint64_t src1 = get_gpr_64(ctx, a->rs1, EXT_NONE);
    uint64_t src2 = get_gpr_64(ctx, a->rs2, EXT_NONE);
    bool taken = src1 < src2;
    ctx->info->dst_data[1] = taken;
    ctx->info->dst_data[2] = gen_pc_plus_diff(ctx, a->imm);
    ctx->info->type = COND;
    SET_SRC2
    return true;
}
static bool trans_bge(DisasContext *ctx, arg_bge *a) {
    uint64_t src1 = get_gpr_64(ctx, a->rs1, EXT_NONE);
    uint64_t src2 = get_gpr_64(ctx, a->rs2, EXT_NONE);
    bool taken = (int64_t)src1 >= (int64_t)src2;
    ctx->info->dst_data[1] = taken;
    ctx->info->dst_data[2] = gen_pc_plus_diff(ctx, a->imm);
    ctx->info->type = COND;
    SET_SRC2
    return true;
}
static bool trans_bgeu(DisasContext *ctx, arg_bgeu *a) {
    uint64_t src1 = get_gpr_64(ctx, a->rs1, EXT_NONE);
    uint64_t src2 = get_gpr_64(ctx, a->rs2, EXT_NONE);
    bool taken = src1 >= src2;
    ctx->info->dst_data[1] = taken;
    ctx->info->dst_data[2] = gen_pc_plus_diff(ctx, a->imm);
    ctx->info->type = COND;
    SET_SRC2
    return true;
}
static bool trans_cbo_clean(DisasContext *ctx, arg_cbo_clean *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_cbo_flush(DisasContext *ctx, arg_cbo_flush *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_cbo_inval(DisasContext *ctx, arg_cbo_inval *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_cbo_zero(DisasContext *ctx, arg_cbo_zero *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_clmul(DisasContext *ctx, arg_clmul *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_clmulh(DisasContext *ctx, arg_clmulh *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_clmulr(DisasContext *ctx, arg_clmulr *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_clz(DisasContext *ctx, arg_clz *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_clzw(DisasContext *ctx, arg_clzw *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_cpop(DisasContext *ctx, arg_cpop *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_cpopw(DisasContext *ctx, arg_cpopw *a) {__NOT_IMPLEMENTED_EXIT__}

static inline bool riscv_csrrw_check(DisasContext *ctx, int csrno, bool write) {
    bool readOnly = (csrno >> 10) == 3;
    if(!csr_ops[csrno].predicate || write && readOnly) {
        ctx->info->exception = EXC_II;
        ctx->info->exc_data = ctx->info->inst;
        return false;
    }
    return true;
}

static void riscv_csrrw(DisasContext *ctx, int csrno, uint64_t* ret_value, uint64_t new_value, uint64_t write_mask) {
    uint64_t old_value = 0;
    if (csr_ops[csrno].op) {
        csr_ops[csrno].op(ctx, csrno, ret_value, new_value, write_mask);
        return;
    }

    if (ret_value) {
        csr_ops[csrno].read(ctx, csrno, &old_value);
        if (unlikely(ctx->info->exception != EXC_NONE)) {
            return;
        }
    }

    if (write_mask) {
        new_value = (old_value & ~write_mask) | (new_value & write_mask);
        csr_ops[csrno].write(ctx, csrno, new_value);
    }

    if (ret_value) *ret_value = old_value;
}

static void do_csrr(DisasContext *ctx, int rd, int rc) {
    uint64_t dest;
    int32_t csr = rc;
    if(!riscv_csrrw_check(ctx, csr, false)) {
        return;
    }
    riscv_csrrw(ctx, csr, &dest, 0, 0);
    ctx->info->dst_idx[0] = rd * 8;
    ctx->info->dst_mask[0] = 0xffffffffffffffff;
    ctx->info->dst_data[0] = dest;
}

static void do_csrrw(DisasContext *ctx, int rd, int rc, uint64_t src, uint64_t mask) {
    uint64_t dest;
    int32_t csr = rc;
    if(!riscv_csrrw_check(ctx, csr, true)) {
        return;
    }
    riscv_csrrw(ctx, csr, &dest, src, mask);
    ctx->info->dst_idx[0] = rd * 8;
    ctx->info->dst_mask[0] = 0xffffffffffffffff;
    ctx->info->dst_data[0] = dest;
}

static bool trans_csrrc(DisasContext *ctx, arg_csrrc *a) {
    if (a->rs1 == 0) {
        do_csrr(ctx, a->rd, a->csr);
    } else {
        TCGv mask = get_gpr_64(ctx, a->rs1, EXT_NONE);
        do_csrrw(ctx, a->rd, a->csr, 0, mask);
    }
    SET_SRC1_DST
    ctx->info->type = CSRWR;
    return true;
}
static bool trans_csrrci(DisasContext *ctx, arg_csrrci *a) {
    if (a->rs1 == 0) {
        do_csrr(ctx, a->rd, a->csr);
    } else {
        TCGv mask = int64_t(a->rs1);
        do_csrrw(ctx, a->rd, a->csr, 0, mask);
    }
    SET_SRC0_DST
    ctx->info->type = CSRWR;
    return true;
}
static bool trans_csrrs(DisasContext *ctx, arg_csrrs *a) {
    if (a->rs1 == 0) {
        do_csrr(ctx, a->rd, a->csr);
    } else {
        TCGv mask = get_gpr_64(ctx, a->rs1, EXT_NONE);
        do_csrrw(ctx, a->rd, a->csr, 0xffffffffffffffff, mask);
    }
    SET_SRC1_DST
    ctx->info->type = CSRWR;
    return true;
}
static bool trans_csrrsi(DisasContext *ctx, arg_csrrsi *a) {
    if (a->rs1 == 0) {
        do_csrr(ctx, a->rd, a->csr);
    } else {
        TCGv mask = int64_t(a->rs1);
        do_csrrw(ctx, a->rd, a->csr, 0xffffffffffffffff, mask);
    }
    SET_SRC0_DST
    ctx->info->type = CSRWR;
    return true;
}
static bool trans_csrrw(DisasContext *ctx, arg_csrrw *a) {
    TCGv src = get_gpr_64(ctx, a->rs1, EXT_NONE);
    do_csrrw(ctx, a->rd, a->csr, src, 0xffffffffffffffff);
    SET_SRC1_DST
    ctx->info->type = CSRWR;
    return true;
}
static bool trans_csrrwi(DisasContext *ctx, arg_csrrwi *a) {
    TCGv mask = int64_t(a->rs1);
    do_csrrw(ctx, a->rd, a->csr, mask, 0xffffffffffffffff);
    SET_SRC0_DST
    ctx->info->type = CSRWR;
    return true;
}
static bool trans_ctz(DisasContext *ctx, arg_ctz *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_ctzw(DisasContext *ctx, arg_ctzw *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_czero_eqz(DisasContext *ctx, arg_czero_eqz *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_czero_nez(DisasContext *ctx, arg_czero_nez *a) {__NOT_IMPLEMENTED_EXIT__}

static inline uint32_t check_nanbox_s(DisasContext *ctx, uint64_t f) {
    uint64_t mask = 0xffffffff00000000;
    if (likely((f & mask) == mask)) {
        return f;
    }
    return 0x7fc00000u;
}
static inline uint64_t nanbox_s(uint64_t f) {
    return f | 0xffffffff00000000;
}
static inline uint64_t get_fpr_64(DisasContext *ctx, int rs1) {
    return ctx->state->fpr[rs1];
}
static inline void setrm(DisasContext *ctx, int rm) {
    ctx->state->fp_status.float_rounding_mode = (FloatRoundMode)rm;
}
static bool trans_fclass_d(DisasContext *ctx, arg_fclass_d *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_fclass_h(DisasContext *ctx, arg_fclass_h *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_fclass_s(DisasContext *ctx, arg_fclass_s *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_fcvt_bf16_s(DisasContext *ctx, arg_fcvt_bf16_s *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_fcvt_d_h(DisasContext *ctx, arg_fcvt_d_h *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_fcvt_d_l(DisasContext *ctx, arg_fcvt_d_l *a) {
    TCGv src = get_gpr_64(ctx, a->rs1, EXT_NONE);
    setrm(ctx, a->rm);
    TCGv dest = int64_to_float64(src, &ctx->state->fp_status);
    SET_FPR_64
    SET_SRC1_DSTF
    ctx->info->type = FMISC_SIMPLE;
    return true;    
}
static bool trans_fcvt_d_lu(DisasContext *ctx, arg_fcvt_d_lu *a) {
    TCGv src = get_gpr_64(ctx, a->rs1, EXT_NONE);
    setrm(ctx, a->rm);
    TCGv dest = uint64_to_float64(src, &ctx->state->fp_status);
    SET_FPR_64
    SET_SRC1_DSTF
    ctx->info->type = FMISC_SIMPLE;
    return true;    
}
static bool trans_fcvt_d_s(DisasContext *ctx, arg_fcvt_d_s *a) {
    TCGv src = check_nanbox_s(ctx, get_gpr_64(ctx, a->rs1, EXT_NONE));
    setrm(ctx, a->rm);
    TCGv dest = float32_to_float64(src, &ctx->state->fp_status);
    SET_FPR_64
    SET_SRC1F_DSTF
    ctx->info->type = FMISC_SIMPLE;
    return true;    
}
static bool trans_fcvt_d_w(DisasContext *ctx, arg_fcvt_d_w *a) {
    TCGv src = get_gpr_64(ctx, a->rs1, EXT_NONE);
    setrm(ctx, a->rm);
    TCGv dest = int32_to_float64(src, &ctx->state->fp_status);
    SET_FPR_64
    SET_SRC1_DSTF
    ctx->info->type = FMISC_SIMPLE;
    return true;    
}
static bool trans_fcvt_d_wu(DisasContext *ctx, arg_fcvt_d_wu *a) {
    TCGv src = get_gpr_64(ctx, a->rs1, EXT_NONE);
    setrm(ctx, a->rm);
    TCGv dest = uint32_to_float64(src, &ctx->state->fp_status);
    SET_FPR_64
    SET_SRC1_DSTF
    ctx->info->type = FMISC_SIMPLE;
    return true;    
}
static bool trans_fcvt_h_d(DisasContext *ctx, arg_fcvt_h_d *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_fcvt_h_l(DisasContext *ctx, arg_fcvt_h_l *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_fcvt_h_lu(DisasContext *ctx, arg_fcvt_h_lu *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_fcvt_h_s(DisasContext *ctx, arg_fcvt_h_s *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_fcvt_h_w(DisasContext *ctx, arg_fcvt_h_w *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_fcvt_h_wu(DisasContext *ctx, arg_fcvt_h_wu *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_fcvt_l_d(DisasContext *ctx, arg_fcvt_l_d *a) {
    TCGv src = get_fpr_64(ctx, a->rs1);
    setrm(ctx, a->rm);
    TCGv dest = float64_to_int64(src, &ctx->state->fp_status);
    SET_GPR_64
    SET_SRC1F_DST
    ctx->info->type = FMISC_COMPLEX;
    return true;
}
static bool trans_fcvt_l_h(DisasContext *ctx, arg_fcvt_l_h *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_fcvt_l_s(DisasContext *ctx, arg_fcvt_l_s *a) {
    TCGv src = check_nanbox_s(ctx, get_fpr_64(ctx, a->rs1));
    setrm(ctx, a->rm);
    TCGv dest = float32_to_int64(src, &ctx->state->fp_status);
    SET_GPR_64
    SET_SRC1F_DST
    ctx->info->type = FMISC_COMPLEX;
    return true;    
}
static bool trans_fcvt_lu_d(DisasContext *ctx, arg_fcvt_lu_d *a) {
    TCGv src = get_fpr_64(ctx, a->rs1);
    setrm(ctx, a->rm);
    TCGv dest = float64_to_uint64(src, &ctx->state->fp_status);
    SET_GPR_64
    SET_SRC1F_DST
    ctx->info->type = FMISC_COMPLEX;
    return true;    
}
static bool trans_fcvt_lu_h(DisasContext *ctx, arg_fcvt_lu_h *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_fcvt_lu_s(DisasContext *ctx, arg_fcvt_lu_s *a) {
    TCGv src = check_nanbox_s(ctx, get_fpr_64(ctx, a->rs1));
    setrm(ctx, a->rm);
    TCGv dest = float32_to_uint64(src, &ctx->state->fp_status);
    SET_GPR_64
    SET_SRC1F_DST
    ctx->info->type = FMISC_COMPLEX;
    return true;    
}
static bool trans_fcvtmod_w_d(DisasContext *ctx, arg_fcvtmod_w_d *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_fcvt_s_bf16(DisasContext *ctx, arg_fcvt_s_bf16 *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_fcvt_s_d(DisasContext *ctx, arg_fcvt_s_d *a) {
    TCGv src = get_fpr_64(ctx, a->rs1);
    setrm(ctx, a->rm);
    TCGv dest = nanbox_s(float64_to_float32(src, &ctx->state->fp_status));
    SET_FPR_64
    SET_SRC1F_DSTF
    ctx->info->type = FMISC_SIMPLE;
    return true;    
}
static bool trans_fcvt_s_h(DisasContext *ctx, arg_fcvt_s_h *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_fcvt_s_l(DisasContext *ctx, arg_fcvt_s_l *a) {
    TCGv src = get_gpr_64(ctx, a->rs1, EXT_NONE);
    setrm(ctx, a->rm);
    TCGv dest = nanbox_s(int64_to_float32(src, &ctx->state->fp_status));
    SET_FPR_64
    SET_SRC1_DSTF
    ctx->info->type = FMISC_SIMPLE;
    return true;    
}
static bool trans_fcvt_s_lu(DisasContext *ctx, arg_fcvt_s_lu *a) {
    TCGv src = get_gpr_64(ctx, a->rs1, EXT_NONE);
    setrm(ctx, a->rm);
    TCGv dest = nanbox_s(uint64_to_float32(src, &ctx->state->fp_status));
    SET_FPR_64
    SET_SRC1_DSTF
    ctx->info->type = FMISC_SIMPLE;
    return true;    
}
static bool trans_fcvt_s_w(DisasContext *ctx, arg_fcvt_s_w *a) {
    TCGv src = get_gpr_64(ctx, a->rs1, EXT_NONE);
    setrm(ctx, a->rm);
    TCGv dest = nanbox_s(int32_to_float32(src, &ctx->state->fp_status));
    SET_FPR_64
    SET_SRC1_DSTF
    ctx->info->type = FMISC_SIMPLE;
    return true;    
}
static bool trans_fcvt_s_wu(DisasContext *ctx, arg_fcvt_s_wu *a) {
    TCGv src = get_gpr_64(ctx, a->rs1, EXT_NONE);
    setrm(ctx, a->rm);
    TCGv dest = nanbox_s(uint32_to_float32(src, &ctx->state->fp_status));
    SET_FPR_64
    SET_SRC1_DSTF
    ctx->info->type = FMISC_SIMPLE;
    return true;    
}
static bool trans_fcvt_w_d(DisasContext *ctx, arg_fcvt_w_d *a) {
    TCGv src = get_fpr_64(ctx, a->rs1);
    setrm(ctx, a->rm);
    TCGv dest = float64_to_int32(src, &ctx->state->fp_status);
    SET_GPR_64
    SET_SRC1F_DST
    ctx->info->type = FMISC_COMPLEX;
    return true;    
}
static bool trans_fcvt_wu_d(DisasContext *ctx, arg_fcvt_wu_d *a) {
    TCGv src = get_fpr_64(ctx, a->rs1);
    setrm(ctx, a->rm);
    TCGv dest = float64_to_uint32(src, &ctx->state->fp_status);
    SET_GPR_64
    SET_SRC1F_DST
    ctx->info->type = FMISC_COMPLEX;
    return true;    
}
static bool trans_fcvt_w_h(DisasContext *ctx, arg_fcvt_w_h *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_fcvt_w_s(DisasContext *ctx, arg_fcvt_w_s *a) {
    TCGv src = check_nanbox_s(ctx, get_fpr_64(ctx, a->rs1));
    setrm(ctx, a->rm);
    TCGv dest = float32_to_int32(src, &ctx->state->fp_status);
    SET_GPR_64
    SET_SRC1F_DST
    ctx->info->type = FMISC_COMPLEX;
    return true;    
}
static bool trans_fcvt_wu_h(DisasContext *ctx, arg_fcvt_wu_h *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_fcvt_wu_s(DisasContext *ctx, arg_fcvt_wu_s *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_fadd_d(DisasContext *ctx, arg_fadd_d *a) {
    TCGv src1 = get_fpr_64(ctx, a->rs1);
    TCGv src2 = get_fpr_64(ctx, a->rs2);
    setrm(ctx, a->rm);
    TCGv dest = float64_add(src1, src2, &ctx->state->fp_status);
    SET_FPR_64
    SET_SRC2F_DSTF
    ctx->info->type = FADD;
    return true;    
}
static bool trans_fadd_h(DisasContext *ctx, arg_fadd_h *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_fadd_s(DisasContext *ctx, arg_fadd_s *a) {
    TCGv src1 = check_nanbox_s(ctx, get_fpr_64(ctx, a->rs1));
    TCGv src2 = check_nanbox_s(ctx, get_fpr_64(ctx, a->rs2));
    setrm(ctx, a->rm);
    TCGv dest = nanbox_s(float32_add(src1, src2, &ctx->state->fp_status));
    SET_FPR_64
    SET_SRC2F_DSTF
    ctx->info->type = FADD;
    return true;    
}
static bool trans_fsub_d(DisasContext *ctx, arg_fsub_d *a) {
    TCGv src1 = get_fpr_64(ctx, a->rs1);
    TCGv src2 = get_fpr_64(ctx, a->rs2);
    setrm(ctx, a->rm);
    TCGv dest = float64_sub(src1, src2, &ctx->state->fp_status);
    SET_FPR_64
    SET_SRC2F_DSTF
    ctx->info->type = FADD;
    return true;    
}
static bool trans_fsub_h(DisasContext *ctx, arg_fsub_h *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_fsub_s(DisasContext *ctx, arg_fsub_s *a) {
    TCGv src1 = check_nanbox_s(ctx, get_fpr_64(ctx, a->rs1));
    TCGv src2 = check_nanbox_s(ctx, get_fpr_64(ctx, a->rs2));
    setrm(ctx, a->rm);
    TCGv dest = nanbox_s(float32_sub(src1, src2, &ctx->state->fp_status));
    SET_FPR_64
    SET_SRC2F_DSTF
    ctx->info->type = FADD;
    return true;    
}
static bool trans_fmul_d(DisasContext *ctx, arg_fmul_d *a) {
    TCGv src1 = get_fpr_64(ctx, a->rs1);
    TCGv src2 = get_fpr_64(ctx, a->rs2);
    setrm(ctx, a->rm);
    TCGv dest = float64_mul(src1, src2, &ctx->state->fp_status);
    SET_FPR_64
    SET_SRC2F_DSTF
    ctx->info->type = FMUL;
    return true;    
}
static bool trans_fmul_h(DisasContext *ctx, arg_fmul_h *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_fmul_s(DisasContext *ctx, arg_fmul_s *a) {
    TCGv src1 = check_nanbox_s(ctx, get_fpr_64(ctx, a->rs1));
    TCGv src2 = check_nanbox_s(ctx, get_fpr_64(ctx, a->rs2));
    setrm(ctx, a->rm);
    TCGv dest = nanbox_s(float32_mul(src1, src2, &ctx->state->fp_status));
    SET_FPR_64
    SET_SRC2F_DSTF
    ctx->info->type = FMUL;
    return true;    
}
static bool trans_fdiv_d(DisasContext *ctx, arg_fdiv_d *a) {
    TCGv src1 = get_fpr_64(ctx, a->rs1);
    TCGv src2 = get_fpr_64(ctx, a->rs2);
    setrm(ctx, a->rm);
    TCGv dest = float64_div(src1, src2, &ctx->state->fp_status);
    SET_FPR_64
    SET_SRC2F_DSTF
    ctx->info->type = FDIV;
    return true;    
}
static bool trans_fdiv_h(DisasContext *ctx, arg_fdiv_h *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_fdiv_s(DisasContext *ctx, arg_fdiv_s *a) {
    TCGv src1 = check_nanbox_s(ctx, get_fpr_64(ctx, a->rs1));
    TCGv src2 = check_nanbox_s(ctx, get_fpr_64(ctx, a->rs2));
    setrm(ctx, a->rm);
    TCGv dest = nanbox_s(float32_div(src1, src2, &ctx->state->fp_status));
    SET_FPR_64
    SET_SRC2F_DSTF
    ctx->info->type = FDIV;
    return true;    
}
static bool trans_fence(DisasContext *ctx, arg_fence *a) {ctx->info->type=INT;return true;}
static bool trans_fence_i(DisasContext *ctx, arg_fence_i *a) {ctx->info->type=INT;return true;}
static bool trans_feq_d(DisasContext *ctx, arg_feq_d *a) {
    TCGv src1 = get_fpr_64(ctx, a->rs1);
    TCGv src2 = get_fpr_64(ctx, a->rs2);
    TCGv dest = float64_eq_quiet(src1, src2, &ctx->state->fp_status);
    SET_GPR_64
    SET_SRC2F_DST
    ctx->info->type = FMISC_SIMPLE;
    return true;    
}
static bool trans_feq_h(DisasContext *ctx, arg_feq_h *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_feq_s(DisasContext *ctx, arg_feq_s *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_fle_d(DisasContext *ctx, arg_fle_d *a) {
    TCGv src1 = get_fpr_64(ctx, a->rs1);
    TCGv src2 = get_fpr_64(ctx, a->rs2);
    TCGv dest = float64_le(src1, src2, &ctx->state->fp_status);
    SET_GPR_64
    SET_SRC2F_DST
    ctx->info->type = FMISC_SIMPLE;
    return true;    
}
static bool trans_fle_h(DisasContext *ctx, arg_fle_h *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_fleq_d(DisasContext *ctx, arg_fleq_d *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_fleq_h(DisasContext *ctx, arg_fleq_h *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_fleq_s(DisasContext *ctx, arg_fleq_s *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_fle_s(DisasContext *ctx, arg_fle_s *a) {
    TCGv src1 = check_nanbox_s(ctx, get_fpr_64(ctx, a->rs1));
    TCGv src2 = check_nanbox_s(ctx, get_fpr_64(ctx, a->rs2));
    TCGv dest = float32_le(src1, src2, &ctx->state->fp_status);
    SET_GPR_64
    SET_SRC2F_DST
    ctx->info->type = FMISC_SIMPLE;
    return true;
}
static bool trans_flh(DisasContext *ctx, arg_flh *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_fli_d(DisasContext *ctx, arg_fli_d *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_fli_h(DisasContext *ctx, arg_fli_h *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_fli_s(DisasContext *ctx, arg_fli_s *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_flt_d(DisasContext *ctx, arg_flt_d *a) {
    TCGv src1 = get_fpr_64(ctx, a->rs1);
    TCGv src2 = get_fpr_64(ctx, a->rs2);
    TCGv dest = float64_lt(src1, src2, &ctx->state->fp_status);
    SET_GPR_64
    SET_SRC2F_DST
    ctx->info->type = FMISC_SIMPLE;
    return true;
}
static bool trans_flt_h(DisasContext *ctx, arg_flt_h *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_fltq_d(DisasContext *ctx, arg_fltq_d *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_fltq_h(DisasContext *ctx, arg_fltq_h *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_fltq_s(DisasContext *ctx, arg_fltq_s *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_flt_s(DisasContext *ctx, arg_flt_s *a) {
    TCGv src1 = check_nanbox_s(ctx, get_fpr_64(ctx, a->rs1));
    TCGv src2 = check_nanbox_s(ctx, get_fpr_64(ctx, a->rs2));
    TCGv dest = float32_lt(src1, src2, &ctx->state->fp_status);
    SET_GPR_64
    SET_SRC2F_DST
    ctx->info->type = FMISC_SIMPLE;
    return true;
}
static bool trans_fmadd_d(DisasContext *ctx, arg_fmadd_d *a) {
    TCGv src1 = get_fpr_64(ctx, a->rs1);
    TCGv src2 = get_fpr_64(ctx, a->rs2);
    TCGv src3 = get_fpr_64(ctx, a->rs3);
    setrm(ctx, a->rm);
    TCGv dest = float64_muladd(src1, src2, src3, 0, &ctx->state->fp_status);
    SET_FPR_64
    SET_SRC3F_DSTF
    ctx->info->type = FMA;
    return true;
}
static bool trans_fmadd_h(DisasContext *ctx, arg_fmadd_h *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_fmadd_s(DisasContext *ctx, arg_fmadd_s *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_fmax_d(DisasContext *ctx, arg_fmax_d *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_fmax_h(DisasContext *ctx, arg_fmax_h *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_fmaxm_d(DisasContext *ctx, arg_fmaxm_d *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_fmaxm_h(DisasContext *ctx, arg_fmaxm_h *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_fmaxm_s(DisasContext *ctx, arg_fmaxm_s *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_fmax_s(DisasContext *ctx, arg_fmax_s *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_fmin_d(DisasContext *ctx, arg_fmin_d *a) {
    TCGv src1 = get_fpr_64(ctx, a->rs1);
    TCGv src2 = get_fpr_64(ctx, a->rs2);
    TCGv dest = float64_minimum_number(src1, src2, &ctx->state->fp_status);
    SET_FPR_64
    SET_SRC2F_DSTF
    ctx->info->type = FMISC_SIMPLE;
    return true;
}
static bool trans_fmin_h(DisasContext *ctx, arg_fmin_h *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_fminm_d(DisasContext *ctx, arg_fminm_d *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_fminm_h(DisasContext *ctx, arg_fminm_h *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_fminm_s(DisasContext *ctx, arg_fminm_s *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_fmin_s(DisasContext *ctx, arg_fmin_s *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_fmsub_d(DisasContext *ctx, arg_fmsub_d *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_fmsub_h(DisasContext *ctx, arg_fmsub_h *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_fmsub_s(DisasContext *ctx, arg_fmsub_s *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_fmv_d_x(DisasContext *ctx, arg_fmv_d_x *a) {
    TCGv dest = get_gpr_64(ctx, a->rs1, EXT_NONE);
    SET_FPR_64
    SET_SRC1_DSTF
    ctx->info->type = FMISC_SIMPLE;
    return true;
}
static bool trans_fmvh_x_d(DisasContext *ctx, arg_fmvh_x_d *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_fmv_h_x(DisasContext *ctx, arg_fmv_h_x *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_fmvp_d_x(DisasContext *ctx, arg_fmvp_d_x *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_fmv_w_x(DisasContext *ctx, arg_fmv_w_x *a) {
    TCGv dest = nanbox_s(get_gpr_32(ctx, a->rs1, EXT_NONE));
    SET_FPR_64
    SET_SRC1_DSTF
    ctx->info->type = FMISC_SIMPLE;
    return true;
}
static bool trans_fmv_x_d(DisasContext *ctx, arg_fmv_x_d *a) {
    TCGv dest = get_fpr_64(ctx, a->rs1);
    SET_GPR_64
    SET_SRC1F_DST
    ctx->info->type = FMISC_SIMPLE;
    return true;
}
static bool trans_fmv_x_h(DisasContext *ctx, arg_fmv_x_h *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_fmv_x_w(DisasContext *ctx, arg_fmv_x_w *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_fnmadd_d(DisasContext *ctx, arg_fnmadd_d *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_fnmadd_h(DisasContext *ctx, arg_fnmadd_h *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_fnmadd_s(DisasContext *ctx, arg_fnmadd_s *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_fnmsub_d(DisasContext *ctx, arg_fnmsub_d *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_fnmsub_h(DisasContext *ctx, arg_fnmsub_h *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_fnmsub_s(DisasContext *ctx, arg_fnmsub_s *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_fround_d(DisasContext *ctx, arg_fround_d *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_fround_h(DisasContext *ctx, arg_fround_h *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_froundnx_d(DisasContext *ctx, arg_froundnx_d *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_froundnx_h(DisasContext *ctx, arg_froundnx_h *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_froundnx_s(DisasContext *ctx, arg_froundnx_s *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_fround_s(DisasContext *ctx, arg_fround_s *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_flw(DisasContext *ctx, arg_flw *a) {
    TCGv addr = get_address(ctx, a->rs1, a->imm);
    TCGv dest = nanbox_s(ld_w(ctx, addr));
    SET_FPR_64
    SET_SRC1_DSTF_ADDR
    ctx->info->type = LOAD;
    return true;
}
static bool trans_fld(DisasContext *ctx, arg_fld *a) {
    TCGv addr = get_address(ctx, a->rs1, a->imm);
    TCGv dest = ld_d(ctx, addr);
    SET_FPR_64
    SET_SRC1_DSTF_ADDR
    ctx->info->type = LOAD;
    return true;
}
static bool trans_fsw(DisasContext *ctx, arg_fsw *a) {
    TCGv addr = get_address(ctx, a->rs1, a->imm);
    TCGv src = get_fpr_64(ctx, a->rs2);
    st_w(ctx, addr, src);
    SET_SRC1_SRC2F_ADDR
    ctx->info->type = STORE;
    return true;
}
static bool trans_fsd(DisasContext *ctx, arg_fsd *a) {
    TCGv addr = get_address(ctx, a->rs1, a->imm);
    TCGv src = get_fpr_64(ctx, a->rs2);
    st_d(ctx, addr, src);
    SET_SRC1_SRC2F_ADDR
    ctx->info->type = STORE;
    return true;
}
static bool trans_fsgnj_d(DisasContext *ctx, arg_fsgnj_d *a) {
    TCGv src1 = get_fpr_64(ctx, a->rs1);
    uint64_t dest;
    if (a->rs1 == a->rs2) { /* FMOV */
        dest = src1;
        SET_SRC1F_DSTF
    } else {
        int64_t src2 = get_fpr_64(ctx, a->rs2);
        dest = deposit64(src2, 0, 63, src1);
        SET_SRC2F_DSTF
    }
    SET_FPR_64
    ctx->info->type = FMISC_SIMPLE;
    return true;
}
static bool trans_fsgnj_h(DisasContext *ctx, arg_fsgnj_h *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_fsgnjn_d(DisasContext *ctx, arg_fsgnjn_d *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_fsgnjn_h(DisasContext *ctx, arg_fsgnjn_h *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_fsgnjn_s(DisasContext *ctx, arg_fsgnjn_s *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_fsgnj_s(DisasContext *ctx, arg_fsgnj_s *a) {
    TCGv src1 = get_fpr_64(ctx, a->rs1);
    uint64_t dest;
    ctx->info->src_reg[0] = a->rs1 | DSTF_REG_MASK;
    if (a->rs1 == a->rs2) { /* FMOV */
        dest = src1;
        SET_SRC1F_DSTF
    } else {
        int64_t src2 = get_fpr_64(ctx, a->rs2);
        dest = deposit64(src2, 0, 31, src1);
        SET_SRC2F_DSTF
    }
    SET_FPR_64
    ctx->info->type = FMISC_SIMPLE;
    return true;
}
static bool trans_fsgnjx_h(DisasContext *ctx, arg_fsgnjx_h *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_fsgnjx_s(DisasContext *ctx, arg_fsgnjx_s *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_fsgnjx_d(DisasContext *ctx, arg_fsgnjx_d *a) {
    TCGv src1 = get_fpr_64(ctx, a->rs1);
    uint64_t dest;
    ctx->info->src_reg[0] = a->rs1 | DSTF_REG_MASK;
    if (a->rs1 == a->rs2) { /* FABS */
        dest = src1 & (~INT64_MIN);
        SET_SRC1F_DSTF
    } else {
        int64_t src2 = get_fpr_64(ctx, a->rs2);
        dest = src1 ^ (src2 & INT64_MIN);
        SET_SRC2F_DSTF
    }
    SET_FPR_64
    ctx->info->type = FMISC_SIMPLE;
    return true;
}
static bool trans_fsh(DisasContext *ctx, arg_fsh *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_fsqrt_d(DisasContext *ctx, arg_fsqrt_d *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_fsqrt_h(DisasContext *ctx, arg_fsqrt_h *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_fsqrt_s(DisasContext *ctx, arg_fsqrt_s *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_hfence_gvma(DisasContext *ctx, arg_hfence_gvma *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_hfence_vvma(DisasContext *ctx, arg_hfence_vvma *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_hinval_gvma(DisasContext *ctx, arg_hinval_gvma *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_hinval_vvma(DisasContext *ctx, arg_hinval_vvma *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_hlv_b(DisasContext *ctx, arg_hlv_b *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_hlv_bu(DisasContext *ctx, arg_hlv_bu *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_hlv_d(DisasContext *ctx, arg_hlv_d *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_hlv_h(DisasContext *ctx, arg_hlv_h *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_hlv_hu(DisasContext *ctx, arg_hlv_hu *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_hlv_w(DisasContext *ctx, arg_hlv_w *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_hlv_wu(DisasContext *ctx, arg_hlv_wu *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_hlvx_hu(DisasContext *ctx, arg_hlvx_hu *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_hlvx_wu(DisasContext *ctx, arg_hlvx_wu *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_hsv_b(DisasContext *ctx, arg_hsv_b *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_hsv_d(DisasContext *ctx, arg_hsv_d *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_hsv_h(DisasContext *ctx, arg_hsv_h *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_hsv_w(DisasContext *ctx, arg_hsv_w *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_jal(DisasContext *ctx, arg_jal *a) {
    TCGv target_pc = gen_pc_plus_diff(ctx, a->imm);
    TCGv succ_pc = gen_pc_plus_diff(ctx, ctx->info->inst_size);
    ctx->info->dst_idx[0] = a->rd * 8;
    ctx->info->dst_mask[0] = 0xffffffffffffffff;
    ctx->info->dst_data[0] = succ_pc;
    ctx->info->dst_data[2] = target_pc;
    ctx->info->dst_reg = a->rd;
    if (a->rd == 0) {
        ctx->info->type = DIRECT;
    } else {
        ctx->info->type = PUSH;
    }
    return true;
}
static bool trans_jalr(DisasContext *ctx, arg_jalr *a) {
    TCGv target_pc = get_gpr_64(ctx, a->rs1, EXT_NONE) + (int64_t)a->imm;
    TCGv succ_pc = gen_pc_plus_diff(ctx, ctx->info->inst_size);
    ctx->info->dst_idx[0] = a->rd * 8;
    ctx->info->dst_mask[0] = 0xffffffffffffffff;
    ctx->info->dst_data[0] = succ_pc;
    ctx->info->dst_data[2] = target_pc;
    SET_SRC1_DST
    bool rd_valid = a->rd == 1 || a->rd == 5;
    bool rs1_valid = a->rs1 == 1 || a->rs1 == 5;
    if (rd_valid && rs1_valid) {
        ctx->info->type = a->rd == a->rs1 ? PUSH : POP_PUSH;
    } else if (rd_valid) {
        if (a->rs1 != 0) ctx->info->type = IND_CALL;
        else ctx->info->type = PUSH;
    } else if (rs1_valid) {
        ctx->info->type = POP;
    } else if (a->rs1 != 0) {
        ctx->info->type = INDIRECT;
    } else {
        Log::error("jalr with rs1 = 0 is not supported");
    }
    return true;
}
static bool trans_lb(DisasContext *ctx, arg_lb *a) {
    TCGv addr = get_address(ctx, a->rs1, a->imm);
    ctx->info->exc_data = addr;
    TCGv dest = (int64_t)ld_b(ctx, addr);
    SET_GPR_64
    SET_SRC1_DST_ADDR
    ctx->info->type = LOAD;
    return true;
}
static bool trans_lbu(DisasContext *ctx, arg_lbu *a) {
    TCGv addr = get_address(ctx, a->rs1, a->imm);
    ctx->info->exc_data = addr;
    TCGv dest = (uint8_t)ld_b(ctx, addr);
    SET_GPR_64
    SET_SRC1_DST_ADDR
    ctx->info->type = LOAD;
    return true;
}
static bool trans_lh(DisasContext *ctx, arg_lh *a) {
    TCGv addr = get_address(ctx, a->rs1, a->imm);
    ctx->info->exc_data = addr;
    TCGv dest = (int64_t)ld_h(ctx, addr);
    SET_GPR_64
    SET_SRC1_DST_ADDR
    ctx->info->type = LOAD;
    return true;
}
static bool trans_lhu(DisasContext *ctx, arg_lhu *a) {
    TCGv addr = get_address(ctx, a->rs1, a->imm);
    ctx->info->exc_data = addr;
    TCGv dest = (uint16_t)ld_h(ctx, addr);
    SET_GPR_64
    SET_SRC1_DST_ADDR
    ctx->info->type = LOAD;
    return true;
}
static bool trans_lw(DisasContext *ctx, arg_lw *a) {
    TCGv addr = get_address(ctx, a->rs1, a->imm);
    ctx->info->exc_data = addr;
    TCGv dest = (int64_t)ld_w(ctx, addr);
    SET_GPR_64
    SET_SRC1_DST_ADDR
    ctx->info->type = LOAD;
    return true;
}
static bool trans_lwu(DisasContext *ctx, arg_lwu *a) {
    TCGv addr = get_address(ctx, a->rs1, a->imm);
    ctx->info->exc_data = addr;
    TCGv dest = (uint32_t)ld_w(ctx, addr);
    SET_GPR_64
    SET_SRC1_DST_ADDR
    ctx->info->type = LOAD;
    return true;
}
static bool trans_ld(DisasContext *ctx, arg_ld *a) {
    TCGv addr = get_address(ctx, a->rs1, a->imm);
    ctx->info->exc_data = addr;
    TCGv dest = (int64_t)ld_d(ctx, addr);
    SET_GPR_64
    SET_SRC1_DST_ADDR
    ctx->info->type = LOAD;
    return true;
}
static bool trans_lpad(DisasContext *ctx, arg_lpad *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_lr_w(DisasContext *ctx, arg_lr_w *a) {
    TCGv addr = get_address(ctx, a->rs1, 0);
    ctx->info->exc_data = addr;
    TCGv dest = (int64_t)ld_w(ctx, addr);
    ctx->info->dst_idx[0] = a->rd * 8;
    ctx->info->dst_mask[0] = 0xffffffffffffffff;
    ctx->info->dst_data[0] = dest;
    ctx->info->dst_idx[1] = ARCH_LOAD_VAL;
    ctx->info->dst_mask[1] = 0xffffffffffffffff;
    ctx->info->dst_data[1] = dest;
    ctx->info->dst_idx[2] = ARCH_LOAD_RES;
    ctx->info->dst_mask[2] = 0xffffffffffffffff;
    ctx->info->dst_data[2] = addr;
    SET_SRC1_DST_ADDR
    ctx->info->type = LR;
    return true;
}
static bool trans_lr_d(DisasContext *ctx, arg_lr_d *a) {
    TCGv addr = get_address(ctx, a->rs1, 0);
    ctx->info->exc_data = addr;
    TCGv dest = (int64_t)ld_d(ctx, addr);
    ctx->info->dst_idx[0] = a->rd * 8;
    ctx->info->dst_mask[0] = 0xffffffffffffffff;
    ctx->info->dst_data[0] = dest;
    ctx->info->dst_idx[1] = ARCH_LOAD_VAL;
    ctx->info->dst_mask[1] = 0xffffffffffffffff;
    ctx->info->dst_data[1] = dest;
    ctx->info->dst_idx[2] = ARCH_LOAD_RES;
    ctx->info->dst_mask[2] = 0xffffffffffffffff;
    ctx->info->dst_data[2] = addr;
    SET_SRC1_DST_ADDR
    ctx->info->type = LR;
    return true;
}
static bool trans_sc_w(DisasContext *ctx, arg_sc_w *a) {
    TCGv addr = get_address(ctx, a->rs1, 0);
    ctx->info->exc_data = addr;
    TCGv src = get_gpr_64(ctx, a->rs2, EXT_NONE);
    if (addr != ctx->state->load_res || ld_w(ctx, addr) != ctx->state->load_val) {
        ctx->info->dst_idx[0] = a->rd * 8;
        ctx->info->dst_mask[0] = 0xffffffffffffffff;
        ctx->info->dst_data[0] = 1;
    } else {
        st_w(ctx, addr, src);
        ctx->info->dst_idx[0] = a->rd * 8;
        ctx->info->dst_mask[0] = 0xffffffffffffffff;
        ctx->info->dst_data[0] = 0;
        ctx->info->dst_data[2] = src;
    }
    ctx->info->dst_idx[1] = ARCH_LOAD_RES;
    ctx->info->dst_mask[1] = 0xffffffffffffffff;
    ctx->info->dst_data[1] = 0xffffffffffffffff;
    SET_SRC1_SRC2_ADDR
    ctx->info->type = SC;
    return true;
}
static bool trans_sc_d(DisasContext *ctx, arg_sc_d *a) {
    TCGv addr = get_address(ctx, a->rs1, 0);
    ctx->info->exc_data = addr;
    TCGv src = get_gpr_64(ctx, a->rs2, EXT_NONE);
    if (addr != ctx->state->load_res || ld_d(ctx, addr) != ctx->state->load_val) {
        ctx->info->dst_idx[0] = a->rd * 8;
        ctx->info->dst_mask[0] = 0xffffffffffffffff;
        ctx->info->dst_data[0] = 1;
    } else {
        st_d(ctx, addr, src);
        ctx->info->dst_idx[0] = a->rd * 8;
        ctx->info->dst_mask[0] = 0xffffffffffffffff;
        ctx->info->dst_data[0] = 0;
        ctx->info->dst_data[2] = src;
    }
    ctx->info->dst_idx[1] = ARCH_LOAD_RES;
    ctx->info->dst_mask[1] = 0xffffffffffffffff;
    ctx->info->dst_data[1] = 0xffffffffffffffff;
    SET_SRC1_SRC2_ADDR
    ctx->info->type = SC;
    return true;
}
static bool trans_lui(DisasContext *ctx, arg_lui *a) {
    TCGv dest = a->imm;
    SET_GPR_64
    SET_SRC0_DST
    ctx->info->type = INT;
    return true;
}
static bool trans_mop_r_n(DisasContext *ctx, arg_mop_r_n *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_mop_rr_n(DisasContext *ctx, arg_mop_rr_n *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_mul(DisasContext *ctx, arg_mul *a) {
    TCGv src1 = get_gpr_64(ctx, a->rs1, EXT_NONE);
    TCGv src2 = get_gpr_64(ctx, a->rs2, EXT_NONE);
    int64_t dest = (int64_t)src1 * (int64_t)src2;
    SET_GPR_64
    SET_SRC2_DST
    ctx->info->type = MULT;
    return true;
}
static bool trans_div(DisasContext *ctx, arg_div *a) {
    TCGv src1 = get_gpr_64(ctx, a->rs1, EXT_NONE);
    TCGv src2 = get_gpr_64(ctx, a->rs2, EXT_NONE);
    int64_t dest;
    if (src2 == 0) {
        dest = -1;
    } else if (src1 == INT64_MIN && (int64_t)src2 == -1) {
        dest = src1;
    } else {
        dest = (int64_t)src1 / (int64_t)src2;
    }
    SET_GPR_64
    SET_SRC2_DST
    ctx->info->type = DIV;
    return true;
}
static bool trans_divu(DisasContext *ctx, arg_divu *a) {
    TCGv src1 = get_gpr_64(ctx, a->rs1, EXT_NONE);
    TCGv src2 = get_gpr_64(ctx, a->rs2, EXT_NONE);
    uint64_t dest;
    dest = (uint64_t)src1 / (uint64_t)src2;
    SET_GPR_64
    SET_SRC2_DST
    ctx->info->type = DIV;
    return true;
}
static bool trans_rem(DisasContext *ctx, arg_rem *a) {
    TCGv src1 = get_gpr_64(ctx, a->rs1, EXT_NONE);
    TCGv src2 = get_gpr_64(ctx, a->rs2, EXT_NONE);
    int64_t dest = (int64_t)src1 % (int64_t)src2;
    SET_GPR_64
    SET_SRC2_DST
    ctx->info->type = DIV;
    return true;
}
static bool trans_remu(DisasContext *ctx, arg_remu *a) {
    TCGv src1 = get_gpr_64(ctx, a->rs1, EXT_NONE);
    TCGv src2 = get_gpr_64(ctx, a->rs2, EXT_NONE);
    uint64_t dest = (uint64_t)src1 % (uint64_t)src2;
    SET_GPR_64
    SET_SRC2_DST
    ctx->info->type = DIV;
    return true;
}
static bool trans_mulh(DisasContext *ctx, arg_mulh *a) {
    TCGv src1 = get_gpr_64(ctx, a->rs1, EXT_NONE);
    TCGv src2 = get_gpr_64(ctx, a->rs2, EXT_NONE);
    int64_t dest;
    __int128_t t = (__int128_t)(int64_t)src1 * (__int128_t)(int64_t)src2;
    dest = t >> 64;
    SET_GPR_64
    SET_SRC2_DST
    ctx->info->type = MULT;
    return true;
}
static bool trans_mulhsu(DisasContext *ctx, arg_mulhsu *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_mulhu(DisasContext *ctx, arg_mulhu *a) {
    TCGv src1 = get_gpr_64(ctx, a->rs1, EXT_NONE);
    TCGv src2 = get_gpr_64(ctx, a->rs2, EXT_NONE);
    uint64_t dest;
    __uint128_t t = (__uint128_t)(uint64_t)src1 * (__uint128_t)(uint64_t)src2;
    dest = t >> 64;
    SET_GPR_64
    SET_SRC2_DST
    ctx->info->type = MULT;
    return true;
}
static bool trans_mulw(DisasContext *ctx, arg_mulw *a) {
    TCGv src1 = get_gpr_64(ctx, a->rs1, EXT_NONE);
    TCGv src2 = get_gpr_64(ctx, a->rs2, EXT_NONE);
    int32_t dest = (int32_t)src1 * (int32_t)src2;
    SET_GPR_32
    SET_SRC2_DST
    ctx->info->type = MULT;
    return true;
}
static bool trans_divw(DisasContext *ctx, arg_divw *a) {
    TCGv src1 = get_gpr_64(ctx, a->rs1, EXT_NONE);
    TCGv src2 = get_gpr_64(ctx, a->rs2, EXT_NONE);
    int32_t dest = (int32_t)src1 / (int32_t)src2;
    SET_GPR_32
    SET_SRC2_DST
    ctx->info->type = DIV;
    return true;
}
static bool trans_divuw(DisasContext *ctx, arg_divuw *a) {
    TCGv src1 = get_gpr_64(ctx, a->rs1, EXT_NONE);
    TCGv src2 = get_gpr_64(ctx, a->rs2, EXT_NONE);
    uint32_t dest = (uint32_t)src1 / (uint32_t)src2;
    SET_GPR_32
    SET_SRC2_DST
    ctx->info->type = DIV;
    return true;
}
static bool trans_remw(DisasContext *ctx, arg_remw *a) {
    TCGv src1 = get_gpr_64(ctx, a->rs1, EXT_NONE);
    TCGv src2 = get_gpr_64(ctx, a->rs2, EXT_NONE);
    int32_t dest = (int32_t)src1 % (int32_t)src2;
    SET_GPR_32
    SET_SRC2_DST
    ctx->info->type = DIV;
    SET_SRC2_DST
    return true;
}
static bool trans_remuw(DisasContext *ctx, arg_remuw *a) {
    TCGv src1 = get_gpr_64(ctx, a->rs1, EXT_NONE);
    TCGv src2 = get_gpr_64(ctx, a->rs2, EXT_NONE);
    uint32_t dest = (uint32_t)src1 % (uint32_t)src2;
    SET_GPR_32
    SET_SRC2_DST
    ctx->info->type = DIV;
    return true;
}
static bool trans_sb(DisasContext *ctx, arg_sb *a) {
    uint64_t addr = get_address(ctx, a->rs1, a->imm);
    ctx->info->exc_data = addr;
    uint64_t data = get_gpr_64(ctx, a->rs2, EXT_NONE);
    bool mem = st_b(ctx, addr, data);
    ctx->info->dst_data[1] = (uint8_t)data;
    SET_SRC1_SRC2_ADDR
    ctx->info->type = STORE;
    return true;
}
static bool trans_sh(DisasContext *ctx, arg_sh *a) {
    uint64_t addr = get_address(ctx, a->rs1, a->imm);
    ctx->info->exc_data = addr;
    uint64_t data = get_gpr_64(ctx, a->rs2, EXT_NONE);
    bool mem = st_h(ctx, addr, data);
    ctx->info->dst_data[1] = (uint16_t)data;
    SET_SRC1_SRC2_ADDR
    ctx->info->type = STORE;
    return true;
}
static bool trans_sw(DisasContext *ctx, arg_sw *a) {
    uint64_t addr = get_address(ctx, a->rs1, a->imm);
    ctx->info->exc_data = addr;
    uint64_t data = get_gpr_64(ctx, a->rs2, EXT_NONE);
    bool mem = st_w(ctx, addr, data);
    ctx->info->dst_data[1] = (uint32_t)data;
    SET_SRC1_SRC2_ADDR
    ctx->info->type = STORE;
    return true;
}
static bool trans_sd(DisasContext *ctx, arg_sd *a) {
    uint64_t addr = get_address(ctx, a->rs1, a->imm);
    ctx->info->exc_data = addr;
    uint64_t data = get_gpr_64(ctx, a->rs2, EXT_NONE);
    bool mem = st_d(ctx, addr, data);
    ctx->info->dst_data[1] = data;
    SET_SRC1_SRC2_ADDR
    ctx->info->type = STORE;
    return true;
}
static bool trans_sext_b(DisasContext *ctx, arg_sext_b *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_sext_h(DisasContext *ctx, arg_sext_h *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_sh1add(DisasContext *ctx, arg_sh1add *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_sh1add_uw(DisasContext *ctx, arg_sh1add_uw *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_sh2add(DisasContext *ctx, arg_sh2add *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_sh2add_uw(DisasContext *ctx, arg_sh2add_uw *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_sh3add(DisasContext *ctx, arg_sh3add *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_sh3add_uw(DisasContext *ctx, arg_sh3add_uw *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_sha256sig0(DisasContext *ctx, arg_sha256sig0 *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_sha256sig1(DisasContext *ctx, arg_sha256sig1 *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_sha256sum0(DisasContext *ctx, arg_sha256sum0 *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_sha256sum1(DisasContext *ctx, arg_sha256sum1 *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_sha512sig0(DisasContext *ctx, arg_sha512sig0 *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_sha512sig0h(DisasContext *ctx, arg_sha512sig0h *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_sha512sig0l(DisasContext *ctx, arg_sha512sig0l *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_sha512sig1(DisasContext *ctx, arg_sha512sig1 *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_sha512sig1h(DisasContext *ctx, arg_sha512sig1h *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_sha512sig1l(DisasContext *ctx, arg_sha512sig1l *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_sha512sum0(DisasContext *ctx, arg_sha512sum0 *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_sha512sum0r(DisasContext *ctx, arg_sha512sum0r *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_sha512sum1(DisasContext *ctx, arg_sha512sum1 *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_sha512sum1r(DisasContext *ctx, arg_sha512sum1r *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_sinval_vma(DisasContext *ctx, arg_sinval_vma *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_sll(DisasContext *ctx, arg_sll *a) {
    uint64_t src1 = get_gpr_64(ctx, a->rs1, EXT_NONE);
    uint64_t src2 = get_gpr_64(ctx, a->rs2, EXT_NONE) & 0x3f;
    uint64_t dest = src1 << src2;
    SET_GPR_64
    SET_SRC2_DST
    ctx->info->type = INT;
    return true;
}
static bool trans_slli(DisasContext *ctx, arg_slli *a) {
    uint64_t src1 = get_gpr_64(ctx, a->rs1, EXT_NONE);
    uint64_t dest = src1 << a->shamt;
    SET_GPR_64
    SET_SRC1_DST
    ctx->info->type = INT;
    return true;
}
static bool trans_sra(DisasContext *ctx, arg_sra *a) {
    uint64_t src1 = get_gpr_64(ctx, a->rs1, EXT_NONE);
    uint64_t src2 = get_gpr_64(ctx, a->rs2, EXT_NONE) & 0x3f;
    uint64_t dest = (int64_t)src1 >> src2;
    SET_GPR_64
    SET_SRC2_DST
    ctx->info->type = INT;
    return true;
}
static bool trans_srai(DisasContext *ctx, arg_srai *a) {
    uint64_t src1 = get_gpr_64(ctx, a->rs1, EXT_NONE);
    uint64_t dest = (int64_t)src1 >> a->shamt;
    SET_GPR_64
    SET_SRC1_DST
    ctx->info->type = INT;
    return true;
}
static bool trans_srl(DisasContext *ctx, arg_srl *a) {
    uint64_t src1 = get_gpr_64(ctx, a->rs1, EXT_NONE);
    uint64_t src2 = get_gpr_64(ctx, a->rs2, EXT_NONE) & 0x3f;
    uint64_t dest = src1 >> src2;
    SET_GPR_64
    SET_SRC2_DST
    ctx->info->type = INT;
    return true;
}
static bool trans_srli(DisasContext *ctx, arg_srli *a) {
    uint64_t src1 = get_gpr_64(ctx, a->rs1, EXT_NONE);
    uint64_t dest = src1 >> a->shamt;
    SET_GPR_64
    SET_SRC1_DST
    ctx->info->type = INT;
    return true;
}
static bool trans_slt(DisasContext *ctx, arg_slt *a) {
    uint64_t src1 = get_gpr_64(ctx, a->rs1, EXT_NONE);
    uint64_t src2 = get_gpr_64(ctx, a->rs2, EXT_NONE);
    uint64_t dest = (int64_t)src1 < (int64_t)src2;
    SET_GPR_64
    SET_SRC2_DST
    ctx->info->type = INT;
    return true;
}
static bool trans_slti(DisasContext *ctx, arg_slti *a) {
    uint64_t src1 = get_gpr_64(ctx, a->rs1, EXT_NONE);
    uint64_t dest = (int64_t)src1 < (int64_t)a->imm;
    SET_GPR_64
    SET_SRC1_DST
    ctx->info->type = INT;
    return true;
}
static bool trans_sltu(DisasContext *ctx, arg_sltu *a) {
    uint64_t src1 = get_gpr_64(ctx, a->rs1, EXT_NONE);
    uint64_t src2 = get_gpr_64(ctx, a->rs2, EXT_NONE);
    uint64_t dest = src1 < src2;
    SET_GPR_64
    SET_SRC2_DST
    ctx->info->type = INT;
    return true;
}
static bool trans_sltiu(DisasContext *ctx, arg_sltiu *a) {
    uint64_t src1 = get_gpr_64(ctx, a->rs1, EXT_NONE);
    uint64_t dest = src1 < (uint64_t)a->imm;
    SET_GPR_64
    SET_SRC1_DST
    ctx->info->type = INT;
    return true;
}
static bool trans_sm3p0(DisasContext *ctx, arg_sm3p0 *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_sm3p1(DisasContext *ctx, arg_sm3p1 *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_sm4ed(DisasContext *ctx, arg_sm4ed *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_sm4ks(DisasContext *ctx, arg_sm4ks *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_ssamoswap_d(DisasContext *ctx, arg_ssamoswap_d *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_ssamoswap_w(DisasContext *ctx, arg_ssamoswap_w *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_sspopchk(DisasContext *ctx, arg_sspopchk *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_sspush(DisasContext *ctx, arg_sspush *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_ssrdp(DisasContext *ctx, arg_ssrdp *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_unzip(DisasContext *ctx, arg_unzip *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_uret(DisasContext *ctx, arg_uret *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_pause(DisasContext *ctx, arg_pause *a) {ctx->info->type=INT; return true;}
static bool trans_sret(DisasContext *ctx, arg_sret *a) {
    if (unlikely(!(ctx->state->priv >= MODE_S))) {
        ctx->info->exception = EXC_II;
        ctx->info->exc_data = ctx->info->inst;
    } else {
        ctx->info->exception = EXC_SRET;
    }
    ctx->info->type = SRET;
    return true;
}
static bool trans_mret(DisasContext *ctx, arg_mret *a) {
    if (unlikely(!(ctx->state->priv >= MODE_M))) {
        ctx->info->exception = EXC_II;
        ctx->info->exc_data = ctx->info->inst;
    } else {
        ctx->info->exception = EXC_MRET;
    }
    ctx->info->type = MRET;
    return true;
}
static bool trans_wfi(DisasContext *ctx, arg_wfi *a) {
    ctx->info->type = INT;
    return true;
}
static bool trans_ebreak(DisasContext *ctx, arg_ebreak *a) {
    ctx->info->exc_data = ctx->pc;
    ctx->info->exception = EXC_BP;
    ctx->info->type = CSRWR;
    return true;
}
static bool trans_ecall(DisasContext *ctx, arg_ecall *a) {
    ctx->info->exception = EXC_ECU + ctx->state->priv;
    ctx->info->exc_data = ctx->info->inst;
    ctx->info->type = CSRWR;
    return true;
}

static bool trans_sfence_inval_ir(DisasContext *ctx, arg_sfence_inval_ir *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_sfence_vma(DisasContext *ctx, arg_sfence_vma *a) {
    ctx->info->type = SFENCE;
    return true;
}
static bool trans_sfence_vm(DisasContext *ctx, arg_sfence_vm *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_sfence_w_inval(DisasContext *ctx, arg_sfence_w_inval *a) {__NOT_IMPLEMENTED_EXIT__}

static bool trans_vaaddu_vv(DisasContext *ctx, arg_vaaddu_vv *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vaaddu_vx(DisasContext *ctx, arg_vaaddu_vx *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vaadd_vv(DisasContext *ctx, arg_vaadd_vv *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vaadd_vx(DisasContext *ctx, arg_vaadd_vx *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vadc_vim(DisasContext *ctx, arg_vadc_vim *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vadc_vvm(DisasContext *ctx, arg_vadc_vvm *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vadc_vxm(DisasContext *ctx, arg_vadc_vxm *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vadd_vi(DisasContext *ctx, arg_vadd_vi *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vadd_vv(DisasContext *ctx, arg_vadd_vv *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vadd_vx(DisasContext *ctx, arg_vadd_vx *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vaesdf_vs(DisasContext *ctx, arg_vaesdf_vs *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vaesdf_vv(DisasContext *ctx, arg_vaesdf_vv *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vaesdm_vs(DisasContext *ctx, arg_vaesdm_vs *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vaesdm_vv(DisasContext *ctx, arg_vaesdm_vv *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vaesef_vs(DisasContext *ctx, arg_vaesef_vs *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vaesef_vv(DisasContext *ctx, arg_vaesef_vv *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vaesem_vs(DisasContext *ctx, arg_vaesem_vs *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vaesem_vv(DisasContext *ctx, arg_vaesem_vv *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vaeskf1_vi(DisasContext *ctx, arg_vaeskf1_vi *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vaeskf2_vi(DisasContext *ctx, arg_vaeskf2_vi *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vaesz_vs(DisasContext *ctx, arg_vaesz_vs *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vandn_vv(DisasContext *ctx, arg_vandn_vv *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vandn_vx(DisasContext *ctx, arg_vandn_vx *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vand_vi(DisasContext *ctx, arg_vand_vi *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vand_vv(DisasContext *ctx, arg_vand_vv *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vand_vx(DisasContext *ctx, arg_vand_vx *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vasubu_vv(DisasContext *ctx, arg_vasubu_vv *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vasubu_vx(DisasContext *ctx, arg_vasubu_vx *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vasub_vv(DisasContext *ctx, arg_vasub_vv *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vasub_vx(DisasContext *ctx, arg_vasub_vx *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vbrev8_v(DisasContext *ctx, arg_vbrev8_v *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vbrev_v(DisasContext *ctx, arg_vbrev_v *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vclmulh_vv(DisasContext *ctx, arg_vclmulh_vv *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vclmulh_vx(DisasContext *ctx, arg_vclmulh_vx *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vclmul_vv(DisasContext *ctx, arg_vclmul_vv *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vclmul_vx(DisasContext *ctx, arg_vclmul_vx *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vclz_v(DisasContext *ctx, arg_vclz_v *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vcompress_vm(DisasContext *ctx, arg_vcompress_vm *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vcpop_m(DisasContext *ctx, arg_vcpop_m *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vcpop_v(DisasContext *ctx, arg_vcpop_v *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vctz_v(DisasContext *ctx, arg_vctz_v *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vdivu_vv(DisasContext *ctx, arg_vdivu_vv *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vdivu_vx(DisasContext *ctx, arg_vdivu_vx *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vdiv_vv(DisasContext *ctx, arg_vdiv_vv *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vdiv_vx(DisasContext *ctx, arg_vdiv_vx *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vfadd_vf(DisasContext *ctx, arg_vfadd_vf *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vfadd_vv(DisasContext *ctx, arg_vfadd_vv *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vfclass_v(DisasContext *ctx, arg_vfclass_v *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vfcvt_f_xu_v(DisasContext *ctx, arg_vfcvt_f_xu_v *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vfcvt_f_x_v(DisasContext *ctx, arg_vfcvt_f_x_v *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vfcvt_rtz_x_f_v(DisasContext *ctx, arg_vfcvt_rtz_x_f_v *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vfcvt_rtz_xu_f_v(DisasContext *ctx, arg_vfcvt_rtz_xu_f_v *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vfcvt_x_f_v(DisasContext *ctx, arg_vfcvt_x_f_v *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vfcvt_xu_f_v(DisasContext *ctx, arg_vfcvt_xu_f_v *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vfdiv_vf(DisasContext *ctx, arg_vfdiv_vf *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vfdiv_vv(DisasContext *ctx, arg_vfdiv_vv *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vfirst_m(DisasContext *ctx, arg_vfirst_m *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vfmacc_vf(DisasContext *ctx, arg_vfmacc_vf *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vfmacc_vv(DisasContext *ctx, arg_vfmacc_vv *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vfmadd_vf(DisasContext *ctx, arg_vfmadd_vf *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vfmadd_vv(DisasContext *ctx, arg_vfmadd_vv *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vfmax_vf(DisasContext *ctx, arg_vfmax_vf *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vfmax_vv(DisasContext *ctx, arg_vfmax_vv *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vfmerge_vfm(DisasContext *ctx, arg_vfmerge_vfm *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vfmin_vf(DisasContext *ctx, arg_vfmin_vf *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vfmin_vv(DisasContext *ctx, arg_vfmin_vv *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vfmsac_vf(DisasContext *ctx, arg_vfmsac_vf *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vfmsac_vv(DisasContext *ctx, arg_vfmsac_vv *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vfmsub_vf(DisasContext *ctx, arg_vfmsub_vf *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vfmsub_vv(DisasContext *ctx, arg_vfmsub_vv *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vfmul_vf(DisasContext *ctx, arg_vfmul_vf *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vfmul_vv(DisasContext *ctx, arg_vfmul_vv *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vfmv_f_s(DisasContext *ctx, arg_vfmv_f_s *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vfmv_s_f(DisasContext *ctx, arg_vfmv_s_f *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vfmv_v_f(DisasContext *ctx, arg_vfmv_v_f *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vfncvtbf16_f_f_w(DisasContext *ctx, arg_vfncvtbf16_f_f_w *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vfncvt_f_f_w(DisasContext *ctx, arg_vfncvt_f_f_w *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vfncvt_f_xu_w(DisasContext *ctx, arg_vfncvt_f_xu_w *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vfncvt_f_x_w(DisasContext *ctx, arg_vfncvt_f_x_w *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vfncvt_rod_f_f_w(DisasContext *ctx, arg_vfncvt_rod_f_f_w *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vfncvt_rtz_x_f_w(DisasContext *ctx, arg_vfncvt_rtz_x_f_w *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vfncvt_rtz_xu_f_w(DisasContext *ctx, arg_vfncvt_rtz_xu_f_w *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vfncvt_x_f_w(DisasContext *ctx, arg_vfncvt_x_f_w *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vfncvt_xu_f_w(DisasContext *ctx, arg_vfncvt_xu_f_w *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vfnmacc_vf(DisasContext *ctx, arg_vfnmacc_vf *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vfnmacc_vv(DisasContext *ctx, arg_vfnmacc_vv *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vfnmadd_vf(DisasContext *ctx, arg_vfnmadd_vf *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vfnmadd_vv(DisasContext *ctx, arg_vfnmadd_vv *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vfnmsac_vf(DisasContext *ctx, arg_vfnmsac_vf *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vfnmsac_vv(DisasContext *ctx, arg_vfnmsac_vv *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vfnmsub_vf(DisasContext *ctx, arg_vfnmsub_vf *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vfnmsub_vv(DisasContext *ctx, arg_vfnmsub_vv *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vfrdiv_vf(DisasContext *ctx, arg_vfrdiv_vf *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vfrec7_v(DisasContext *ctx, arg_vfrec7_v *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vfredmax_vs(DisasContext *ctx, arg_vfredmax_vs *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vfredmin_vs(DisasContext *ctx, arg_vfredmin_vs *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vfredosum_vs(DisasContext *ctx, arg_vfredosum_vs *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vfredusum_vs(DisasContext *ctx, arg_vfredusum_vs *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vfrsqrt7_v(DisasContext *ctx, arg_vfrsqrt7_v *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vfrsub_vf(DisasContext *ctx, arg_vfrsub_vf *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vfsgnjn_vf(DisasContext *ctx, arg_vfsgnjn_vf *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vfsgnjn_vv(DisasContext *ctx, arg_vfsgnjn_vv *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vfsgnj_vf(DisasContext *ctx, arg_vfsgnj_vf *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vfsgnj_vv(DisasContext *ctx, arg_vfsgnj_vv *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vfsgnjx_vf(DisasContext *ctx, arg_vfsgnjx_vf *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vfsgnjx_vv(DisasContext *ctx, arg_vfsgnjx_vv *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vfslide1down_vf(DisasContext *ctx, arg_vfslide1down_vf *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vfslide1up_vf(DisasContext *ctx, arg_vfslide1up_vf *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vfsqrt_v(DisasContext *ctx, arg_vfsqrt_v *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vfsub_vf(DisasContext *ctx, arg_vfsub_vf *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vfsub_vv(DisasContext *ctx, arg_vfsub_vv *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vfwadd_vf(DisasContext *ctx, arg_vfwadd_vf *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vfwadd_vv(DisasContext *ctx, arg_vfwadd_vv *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vfwadd_wf(DisasContext *ctx, arg_vfwadd_wf *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vfwadd_wv(DisasContext *ctx, arg_vfwadd_wv *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vfwcvtbf16_f_f_v(DisasContext *ctx, arg_vfwcvtbf16_f_f_v *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vfwcvt_f_f_v(DisasContext *ctx, arg_vfwcvt_f_f_v *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vfwcvt_f_xu_v(DisasContext *ctx, arg_vfwcvt_f_xu_v *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vfwcvt_f_x_v(DisasContext *ctx, arg_vfwcvt_f_x_v *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vfwcvt_rtz_x_f_v(DisasContext *ctx, arg_vfwcvt_rtz_x_f_v *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vfwcvt_rtz_xu_f_v(DisasContext *ctx, arg_vfwcvt_rtz_xu_f_v *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vfwcvt_x_f_v(DisasContext *ctx, arg_vfwcvt_x_f_v *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vfwcvt_xu_f_v(DisasContext *ctx, arg_vfwcvt_xu_f_v *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vfwmaccbf16_vf(DisasContext *ctx, arg_vfwmaccbf16_vf *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vfwmaccbf16_vv(DisasContext *ctx, arg_vfwmaccbf16_vv *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vfwmacc_vf(DisasContext *ctx, arg_vfwmacc_vf *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vfwmacc_vv(DisasContext *ctx, arg_vfwmacc_vv *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vfwmsac_vf(DisasContext *ctx, arg_vfwmsac_vf *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vfwmsac_vv(DisasContext *ctx, arg_vfwmsac_vv *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vfwmul_vf(DisasContext *ctx, arg_vfwmul_vf *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vfwmul_vv(DisasContext *ctx, arg_vfwmul_vv *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vfwnmacc_vf(DisasContext *ctx, arg_vfwnmacc_vf *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vfwnmacc_vv(DisasContext *ctx, arg_vfwnmacc_vv *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vfwnmsac_vf(DisasContext *ctx, arg_vfwnmsac_vf *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vfwnmsac_vv(DisasContext *ctx, arg_vfwnmsac_vv *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vfwredosum_vs(DisasContext *ctx, arg_vfwredosum_vs *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vfwredusum_vs(DisasContext *ctx, arg_vfwredusum_vs *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vfwsub_vf(DisasContext *ctx, arg_vfwsub_vf *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vfwsub_vv(DisasContext *ctx, arg_vfwsub_vv *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vfwsub_wf(DisasContext *ctx, arg_vfwsub_wf *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vfwsub_wv(DisasContext *ctx, arg_vfwsub_wv *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vghsh_vv(DisasContext *ctx, arg_vghsh_vv *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vgmul_vv(DisasContext *ctx, arg_vgmul_vv *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vid_v(DisasContext *ctx, arg_vid_v *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_viota_m(DisasContext *ctx, arg_viota_m *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vl1re16_v(DisasContext *ctx, arg_vl1re16_v *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vl1re32_v(DisasContext *ctx, arg_vl1re32_v *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vl1re64_v(DisasContext *ctx, arg_vl1re64_v *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vl1re8_v(DisasContext *ctx, arg_vl1re8_v *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vl2re16_v(DisasContext *ctx, arg_vl2re16_v *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vl2re32_v(DisasContext *ctx, arg_vl2re32_v *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vl2re64_v(DisasContext *ctx, arg_vl2re64_v *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vl2re8_v(DisasContext *ctx, arg_vl2re8_v *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vl4re16_v(DisasContext *ctx, arg_vl4re16_v *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vl4re32_v(DisasContext *ctx, arg_vl4re32_v *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vl4re64_v(DisasContext *ctx, arg_vl4re64_v *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vl4re8_v(DisasContext *ctx, arg_vl4re8_v *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vl8re16_v(DisasContext *ctx, arg_vl8re16_v *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vl8re32_v(DisasContext *ctx, arg_vl8re32_v *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vl8re64_v(DisasContext *ctx, arg_vl8re64_v *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vl8re8_v(DisasContext *ctx, arg_vl8re8_v *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vle16ff_v(DisasContext *ctx, arg_vle16ff_v *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vle16_v(DisasContext *ctx, arg_vle16_v *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vle32ff_v(DisasContext *ctx, arg_vle32ff_v *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vle32_v(DisasContext *ctx, arg_vle32_v *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vle64ff_v(DisasContext *ctx, arg_vle64ff_v *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vle64_v(DisasContext *ctx, arg_vle64_v *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vle8ff_v(DisasContext *ctx, arg_vle8ff_v *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vle8_v(DisasContext *ctx, arg_vle8_v *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vlm_v(DisasContext *ctx, arg_vlm_v *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vlse16_v(DisasContext *ctx, arg_vlse16_v *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vlse32_v(DisasContext *ctx, arg_vlse32_v *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vlse64_v(DisasContext *ctx, arg_vlse64_v *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vlse8_v(DisasContext *ctx, arg_vlse8_v *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vlxei16_v(DisasContext *ctx, arg_vlxei16_v *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vlxei32_v(DisasContext *ctx, arg_vlxei32_v *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vlxei64_v(DisasContext *ctx, arg_vlxei64_v *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vlxei8_v(DisasContext *ctx, arg_vlxei8_v *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vmacc_vv(DisasContext *ctx, arg_vmacc_vv *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vmacc_vx(DisasContext *ctx, arg_vmacc_vx *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vmadc_vim(DisasContext *ctx, arg_vmadc_vim *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vmadc_vvm(DisasContext *ctx, arg_vmadc_vvm *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vmadc_vxm(DisasContext *ctx, arg_vmadc_vxm *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vmadd_vv(DisasContext *ctx, arg_vmadd_vv *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vmadd_vx(DisasContext *ctx, arg_vmadd_vx *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vmand_mm(DisasContext *ctx, arg_vmand_mm *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vmandn_mm(DisasContext *ctx, arg_vmandn_mm *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vmaxu_vv(DisasContext *ctx, arg_vmaxu_vv *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vmaxu_vx(DisasContext *ctx, arg_vmaxu_vx *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vmax_vv(DisasContext *ctx, arg_vmax_vv *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vmax_vx(DisasContext *ctx, arg_vmax_vx *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vmerge_vim(DisasContext *ctx, arg_vmerge_vim *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vmerge_vvm(DisasContext *ctx, arg_vmerge_vvm *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vmerge_vxm(DisasContext *ctx, arg_vmerge_vxm *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vmfeq_vf(DisasContext *ctx, arg_vmfeq_vf *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vmfeq_vv(DisasContext *ctx, arg_vmfeq_vv *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vmfge_vf(DisasContext *ctx, arg_vmfge_vf *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vmfgt_vf(DisasContext *ctx, arg_vmfgt_vf *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vmfle_vf(DisasContext *ctx, arg_vmfle_vf *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vmfle_vv(DisasContext *ctx, arg_vmfle_vv *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vmflt_vf(DisasContext *ctx, arg_vmflt_vf *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vmflt_vv(DisasContext *ctx, arg_vmflt_vv *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vmfne_vf(DisasContext *ctx, arg_vmfne_vf *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vmfne_vv(DisasContext *ctx, arg_vmfne_vv *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vminu_vv(DisasContext *ctx, arg_vminu_vv *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vminu_vx(DisasContext *ctx, arg_vminu_vx *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vmin_vv(DisasContext *ctx, arg_vmin_vv *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vmin_vx(DisasContext *ctx, arg_vmin_vx *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vmnand_mm(DisasContext *ctx, arg_vmnand_mm *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vmnor_mm(DisasContext *ctx, arg_vmnor_mm *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vmor_mm(DisasContext *ctx, arg_vmor_mm *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vmorn_mm(DisasContext *ctx, arg_vmorn_mm *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vmsbc_vvm(DisasContext *ctx, arg_vmsbc_vvm *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vmsbc_vxm(DisasContext *ctx, arg_vmsbc_vxm *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vmsbf_m(DisasContext *ctx, arg_vmsbf_m *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vmseq_vi(DisasContext *ctx, arg_vmseq_vi *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vmseq_vv(DisasContext *ctx, arg_vmseq_vv *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vmseq_vx(DisasContext *ctx, arg_vmseq_vx *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vmsgtu_vi(DisasContext *ctx, arg_vmsgtu_vi *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vmsgtu_vx(DisasContext *ctx, arg_vmsgtu_vx *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vmsgt_vi(DisasContext *ctx, arg_vmsgt_vi *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vmsgt_vx(DisasContext *ctx, arg_vmsgt_vx *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vmsif_m(DisasContext *ctx, arg_vmsif_m *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vmsleu_vi(DisasContext *ctx, arg_vmsleu_vi *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vmsleu_vv(DisasContext *ctx, arg_vmsleu_vv *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vmsleu_vx(DisasContext *ctx, arg_vmsleu_vx *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vmsle_vi(DisasContext *ctx, arg_vmsle_vi *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vmsle_vv(DisasContext *ctx, arg_vmsle_vv *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vmsle_vx(DisasContext *ctx, arg_vmsle_vx *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vmsltu_vv(DisasContext *ctx, arg_vmsltu_vv *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vmsltu_vx(DisasContext *ctx, arg_vmsltu_vx *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vmslt_vv(DisasContext *ctx, arg_vmslt_vv *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vmslt_vx(DisasContext *ctx, arg_vmslt_vx *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vmsne_vi(DisasContext *ctx, arg_vmsne_vi *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vmsne_vv(DisasContext *ctx, arg_vmsne_vv *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vmsne_vx(DisasContext *ctx, arg_vmsne_vx *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vmsof_m(DisasContext *ctx, arg_vmsof_m *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vmulhsu_vv(DisasContext *ctx, arg_vmulhsu_vv *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vmulhsu_vx(DisasContext *ctx, arg_vmulhsu_vx *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vmulhu_vv(DisasContext *ctx, arg_vmulhu_vv *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vmulhu_vx(DisasContext *ctx, arg_vmulhu_vx *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vmulh_vv(DisasContext *ctx, arg_vmulh_vv *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vmulh_vx(DisasContext *ctx, arg_vmulh_vx *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vmul_vv(DisasContext *ctx, arg_vmul_vv *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vmul_vx(DisasContext *ctx, arg_vmul_vx *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vmv1r_v(DisasContext *ctx, arg_vmv1r_v *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vmv2r_v(DisasContext *ctx, arg_vmv2r_v *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vmv4r_v(DisasContext *ctx, arg_vmv4r_v *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vmv8r_v(DisasContext *ctx, arg_vmv8r_v *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vmv_s_x(DisasContext *ctx, arg_vmv_s_x *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vmv_v_i(DisasContext *ctx, arg_vmv_v_i *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vmv_v_v(DisasContext *ctx, arg_vmv_v_v *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vmv_v_x(DisasContext *ctx, arg_vmv_v_x *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vmv_x_s(DisasContext *ctx, arg_vmv_x_s *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vmxnor_mm(DisasContext *ctx, arg_vmxnor_mm *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vmxor_mm(DisasContext *ctx, arg_vmxor_mm *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vnclipu_wi(DisasContext *ctx, arg_vnclipu_wi *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vnclipu_wv(DisasContext *ctx, arg_vnclipu_wv *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vnclipu_wx(DisasContext *ctx, arg_vnclipu_wx *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vnclip_wi(DisasContext *ctx, arg_vnclip_wi *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vnclip_wv(DisasContext *ctx, arg_vnclip_wv *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vnclip_wx(DisasContext *ctx, arg_vnclip_wx *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vnmsac_vv(DisasContext *ctx, arg_vnmsac_vv *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vnmsac_vx(DisasContext *ctx, arg_vnmsac_vx *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vnmsub_vv(DisasContext *ctx, arg_vnmsub_vv *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vnmsub_vx(DisasContext *ctx, arg_vnmsub_vx *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vnsra_wi(DisasContext *ctx, arg_vnsra_wi *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vnsra_wv(DisasContext *ctx, arg_vnsra_wv *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vnsra_wx(DisasContext *ctx, arg_vnsra_wx *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vnsrl_wi(DisasContext *ctx, arg_vnsrl_wi *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vnsrl_wv(DisasContext *ctx, arg_vnsrl_wv *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vnsrl_wx(DisasContext *ctx, arg_vnsrl_wx *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vor_vi(DisasContext *ctx, arg_vor_vi *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vor_vv(DisasContext *ctx, arg_vor_vv *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vor_vx(DisasContext *ctx, arg_vor_vx *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vredand_vs(DisasContext *ctx, arg_vredand_vs *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vredmaxu_vs(DisasContext *ctx, arg_vredmaxu_vs *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vredmax_vs(DisasContext *ctx, arg_vredmax_vs *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vredminu_vs(DisasContext *ctx, arg_vredminu_vs *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vredmin_vs(DisasContext *ctx, arg_vredmin_vs *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vredor_vs(DisasContext *ctx, arg_vredor_vs *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vredsum_vs(DisasContext *ctx, arg_vredsum_vs *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vredxor_vs(DisasContext *ctx, arg_vredxor_vs *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vremu_vv(DisasContext *ctx, arg_vremu_vv *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vremu_vx(DisasContext *ctx, arg_vremu_vx *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vrem_vv(DisasContext *ctx, arg_vrem_vv *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vrem_vx(DisasContext *ctx, arg_vrem_vx *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vrev8_v(DisasContext *ctx, arg_vrev8_v *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vrgatherei16_vv(DisasContext *ctx, arg_vrgatherei16_vv *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vrgather_vi(DisasContext *ctx, arg_vrgather_vi *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vrgather_vv(DisasContext *ctx, arg_vrgather_vv *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vrgather_vx(DisasContext *ctx, arg_vrgather_vx *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vrol_vv(DisasContext *ctx, arg_vrol_vv *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vrol_vx(DisasContext *ctx, arg_vrol_vx *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vror_vi(DisasContext *ctx, arg_vror_vi *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vror_vv(DisasContext *ctx, arg_vror_vv *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vror_vx(DisasContext *ctx, arg_vror_vx *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vrsub_vi(DisasContext *ctx, arg_vrsub_vi *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vrsub_vx(DisasContext *ctx, arg_vrsub_vx *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vs1r_v(DisasContext *ctx, arg_vs1r_v *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vs2r_v(DisasContext *ctx, arg_vs2r_v *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vs4r_v(DisasContext *ctx, arg_vs4r_v *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vs8r_v(DisasContext *ctx, arg_vs8r_v *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vsaddu_vi(DisasContext *ctx, arg_vsaddu_vi *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vsaddu_vv(DisasContext *ctx, arg_vsaddu_vv *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vsaddu_vx(DisasContext *ctx, arg_vsaddu_vx *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vsadd_vi(DisasContext *ctx, arg_vsadd_vi *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vsadd_vv(DisasContext *ctx, arg_vsadd_vv *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vsadd_vx(DisasContext *ctx, arg_vsadd_vx *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vsbc_vvm(DisasContext *ctx, arg_vsbc_vvm *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vsbc_vxm(DisasContext *ctx, arg_vsbc_vxm *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vse16_v(DisasContext *ctx, arg_vse16_v *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vse32_v(DisasContext *ctx, arg_vse32_v *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vse64_v(DisasContext *ctx, arg_vse64_v *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vse8_v(DisasContext *ctx, arg_vse8_v *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vsetivli(DisasContext *ctx, arg_vsetivli *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vsetvl(DisasContext *ctx, arg_vsetvl *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vsetvli(DisasContext *ctx, arg_vsetvli *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vsext_vf2(DisasContext *ctx, arg_vsext_vf2 *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vsext_vf4(DisasContext *ctx, arg_vsext_vf4 *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vsext_vf8(DisasContext *ctx, arg_vsext_vf8 *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vsha2ch_vv(DisasContext *ctx, arg_vsha2ch_vv *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vsha2cl_vv(DisasContext *ctx, arg_vsha2cl_vv *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vsha2ms_vv(DisasContext *ctx, arg_vsha2ms_vv *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vslide1down_vx(DisasContext *ctx, arg_vslide1down_vx *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vslide1up_vx(DisasContext *ctx, arg_vslide1up_vx *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vslidedown_vi(DisasContext *ctx, arg_vslidedown_vi *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vslidedown_vx(DisasContext *ctx, arg_vslidedown_vx *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vslideup_vi(DisasContext *ctx, arg_vslideup_vi *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vslideup_vx(DisasContext *ctx, arg_vslideup_vx *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vsll_vi(DisasContext *ctx, arg_vsll_vi *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vsll_vv(DisasContext *ctx, arg_vsll_vv *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vsll_vx(DisasContext *ctx, arg_vsll_vx *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vsm3c_vi(DisasContext *ctx, arg_vsm3c_vi *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vsm3me_vv(DisasContext *ctx, arg_vsm3me_vv *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vsm4k_vi(DisasContext *ctx, arg_vsm4k_vi *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vsm4r_vs(DisasContext *ctx, arg_vsm4r_vs *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vsm4r_vv(DisasContext *ctx, arg_vsm4r_vv *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vsmul_vv(DisasContext *ctx, arg_vsmul_vv *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vsmul_vx(DisasContext *ctx, arg_vsmul_vx *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vsm_v(DisasContext *ctx, arg_vsm_v *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vsra_vi(DisasContext *ctx, arg_vsra_vi *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vsra_vv(DisasContext *ctx, arg_vsra_vv *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vsra_vx(DisasContext *ctx, arg_vsra_vx *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vsrl_vi(DisasContext *ctx, arg_vsrl_vi *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vsrl_vv(DisasContext *ctx, arg_vsrl_vv *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vsrl_vx(DisasContext *ctx, arg_vsrl_vx *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vsse16_v(DisasContext *ctx, arg_vsse16_v *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vsse32_v(DisasContext *ctx, arg_vsse32_v *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vsse64_v(DisasContext *ctx, arg_vsse64_v *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vsse8_v(DisasContext *ctx, arg_vsse8_v *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vssra_vi(DisasContext *ctx, arg_vssra_vi *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vssra_vv(DisasContext *ctx, arg_vssra_vv *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vssra_vx(DisasContext *ctx, arg_vssra_vx *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vssrl_vi(DisasContext *ctx, arg_vssrl_vi *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vssrl_vv(DisasContext *ctx, arg_vssrl_vv *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vssrl_vx(DisasContext *ctx, arg_vssrl_vx *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vssubu_vv(DisasContext *ctx, arg_vssubu_vv *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vssubu_vx(DisasContext *ctx, arg_vssubu_vx *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vssub_vv(DisasContext *ctx, arg_vssub_vv *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vssub_vx(DisasContext *ctx, arg_vssub_vx *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vsub_vv(DisasContext *ctx, arg_vsub_vv *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vsub_vx(DisasContext *ctx, arg_vsub_vx *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vsxei16_v(DisasContext *ctx, arg_vsxei16_v *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vsxei32_v(DisasContext *ctx, arg_vsxei32_v *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vsxei64_v(DisasContext *ctx, arg_vsxei64_v *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vsxei8_v(DisasContext *ctx, arg_vsxei8_v *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vwaddu_vv(DisasContext *ctx, arg_vwaddu_vv *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vwaddu_vx(DisasContext *ctx, arg_vwaddu_vx *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vwaddu_wv(DisasContext *ctx, arg_vwaddu_wv *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vwaddu_wx(DisasContext *ctx, arg_vwaddu_wx *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vwadd_vv(DisasContext *ctx, arg_vwadd_vv *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vwadd_vx(DisasContext *ctx, arg_vwadd_vx *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vwadd_wv(DisasContext *ctx, arg_vwadd_wv *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vwadd_wx(DisasContext *ctx, arg_vwadd_wx *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vwmaccsu_vv(DisasContext *ctx, arg_vwmaccsu_vv *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vwmaccsu_vx(DisasContext *ctx, arg_vwmaccsu_vx *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vwmaccus_vx(DisasContext *ctx, arg_vwmaccus_vx *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vwmaccu_vv(DisasContext *ctx, arg_vwmaccu_vv *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vwmaccu_vx(DisasContext *ctx, arg_vwmaccu_vx *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vwmacc_vv(DisasContext *ctx, arg_vwmacc_vv *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vwmacc_vx(DisasContext *ctx, arg_vwmacc_vx *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vwmulsu_vv(DisasContext *ctx, arg_vwmulsu_vv *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vwmulsu_vx(DisasContext *ctx, arg_vwmulsu_vx *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vwmulu_vv(DisasContext *ctx, arg_vwmulu_vv *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vwmulu_vx(DisasContext *ctx, arg_vwmulu_vx *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vwmul_vv(DisasContext *ctx, arg_vwmul_vv *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vwmul_vx(DisasContext *ctx, arg_vwmul_vx *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vwredsumu_vs(DisasContext *ctx, arg_vwredsumu_vs *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vwredsum_vs(DisasContext *ctx, arg_vwredsum_vs *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vwsll_vi(DisasContext *ctx, arg_vwsll_vi *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vwsll_vv(DisasContext *ctx, arg_vwsll_vv *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vwsll_vx(DisasContext *ctx, arg_vwsll_vx *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vwsubu_vv(DisasContext *ctx, arg_vwsubu_vv *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vwsubu_vx(DisasContext *ctx, arg_vwsubu_vx *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vwsubu_wv(DisasContext *ctx, arg_vwsubu_wv *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vwsubu_wx(DisasContext *ctx, arg_vwsubu_wx *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vwsub_vv(DisasContext *ctx, arg_vwsub_vv *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vwsub_vx(DisasContext *ctx, arg_vwsub_vx *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vwsub_wv(DisasContext *ctx, arg_vwsub_wv *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vwsub_wx(DisasContext *ctx, arg_vwsub_wx *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vxor_vi(DisasContext *ctx, arg_vxor_vi *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vxor_vv(DisasContext *ctx, arg_vxor_vv *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vxor_vx(DisasContext *ctx, arg_vxor_vx *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vzext_vf2(DisasContext *ctx, arg_vzext_vf2 *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vzext_vf4(DisasContext *ctx, arg_vzext_vf4 *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_vzext_vf8(DisasContext *ctx, arg_vzext_vf8 *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_wrs_nto(DisasContext *ctx, arg_wrs_nto *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_wrs_sto(DisasContext *ctx, arg_wrs_sto *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_xperm4(DisasContext *ctx, arg_xperm4 *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_xperm8(DisasContext *ctx, arg_xperm8 *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_zext_h_32(DisasContext *ctx, arg_zext_h_32 *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_zext_h_64(DisasContext *ctx, arg_zext_h_64 *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_zip(DisasContext *ctx, arg_zip *a) {__NOT_IMPLEMENTED_EXIT__}


static bool trans_c64_illegal(DisasContext *ctx, arg_c64_illegal *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_c_fld(DisasContext *ctx, arg_c_fld *a) {
    return trans_fld(ctx, a);
}
static bool trans_c_flw(DisasContext *ctx, arg_c_flw *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_c_fsd(DisasContext *ctx, arg_c_fsd *a) {
    return trans_fsd(ctx, a);
}
static bool trans_c_fsw(DisasContext *ctx, arg_c_fsw *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_c_lbu(DisasContext *ctx, arg_c_lbu *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_c_lh(DisasContext *ctx, arg_c_lh *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_c_lhu(DisasContext *ctx, arg_c_lhu *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_cm_jalt(DisasContext *ctx, arg_cm_jalt *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_cm_mva01s(DisasContext *ctx, arg_cm_mva01s *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_cm_mvsa01(DisasContext *ctx, arg_cm_mvsa01 *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_c_mop_n(DisasContext *ctx, arg_c_mop_n *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_cm_pop(DisasContext *ctx, arg_cm_pop *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_cm_popret(DisasContext *ctx, arg_cm_popret *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_cm_popretz(DisasContext *ctx, arg_cm_popretz *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_cm_push(DisasContext *ctx, arg_cm_push *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_c_mul(DisasContext *ctx, arg_c_mul *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_c_not(DisasContext *ctx, arg_c_not *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_c_sb(DisasContext *ctx, arg_c_sb *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_c_sext_b(DisasContext *ctx, arg_c_sext_b *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_c_sext_h(DisasContext *ctx, arg_c_sext_h *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_c_sh(DisasContext *ctx, arg_c_sh *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_c_zext_b(DisasContext *ctx, arg_c_zext_b *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_c_zext_h(DisasContext *ctx, arg_c_zext_h *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_c_zext_w(DisasContext *ctx, arg_c_zext_w *a) {__NOT_IMPLEMENTED_EXIT__}
static bool trans_illegal(DisasContext *ctx, arg_illegal *a) {__NOT_IMPLEMENTED_EXIT__}

bool interpreter(DisasContext *ctx, uint32_t insn, bool rvc) {
    if (rvc) {
        return decode_insn16(ctx, insn);
    } else {
        return decode_insn32(ctx, insn);
    }
}

}
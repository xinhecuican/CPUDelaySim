#ifndef RISCV_ARCHSTATE_H
#define RISCV_ARCHSTATE_H
#include "common/common.h"
#include <cstddef>
#include "arch/arch.h"
extern "C" {
#include "fpu/softfloat.h"
}
namespace cds::arch::riscv {

#define MAX_RISCV_PMPS (16)

typedef struct {
    uint64_t pmp[MAX_RISCV_PMPS / 8];
    uint64_t  addr[MAX_RISCV_PMPS];
    uint64_t num_rules;
} pmp_table_t;

struct ArchState {
    uint64_t gpr[32];
    uint64_t fpr[32];
    uint8_t frm;
    float_status fp_status;
    uint64_t load_res;
    uint64_t load_val;
    uint64_t badaddr;
    uint64_t priv_ver;
    uint64_t misa;
    uint64_t priv;
    uint64_t menvcfg;
    uint64_t senvcfg;
    uint64_t mhartid;
    uint32_t mvendorid;
    uint64_t marchid;
    uint64_t mimpid;
    uint64_t mstatus;
    uint64_t mip;
    uint64_t miclaim;
    uint64_t mie;
    uint64_t mideleg;
    uint64_t sie;
    uint64_t satp;
    uint64_t stval;
    uint64_t medeleg;
    uint64_t stvec;
    uint64_t sepc;
    uint64_t scause;
    uint64_t mtvec;
    uint64_t mepc;
    uint64_t mcause;
    uint64_t mtval;
    uint64_t scounteren;
    uint64_t mcounteren;
    uint64_t mcountinhibit;
    uint64_t sscratch;
    uint64_t mscratch;
    uint64_t stimecmp;
    pmp_table_t pmp_table;
};

struct ArchEnv {
    ArchState* state;
    DecodeInfo* info;
    Arch* arch;
    uint64_t pc;
    uint64_t paddr;
};

#define DisasContext ArchEnv

typedef enum {
    EXT_NONE,
    EXT_SIGN,
    EXT_ZERO,
} DisasExtend;

typedef union PageTableEntry {
  struct {
    uint32_t v   : 1;
    uint32_t r   : 1;
    uint32_t w   : 1;
    uint32_t x   : 1;
    uint32_t u   : 1;
    uint32_t g   : 1;
    uint32_t a   : 1;
    uint32_t d   : 1;
    uint32_t rsw : 2;
    uint64_t ppn :44;
    uint32_t pad : 7;
    uint32_t pbmt: 2;
    uint32_t n   : 1;
  };
  uint64_t val;
} PTE;

#define STATUS_MPRV(mstatus) ((mstatus & 0x20000) != 0)
#define STATUS_SUM(mstatus) ((mstatus & 0x40000) != 0)
#define STATUS_MPP(mstatus) ((mstatus >> 11) & 0x3)
#define STATUS_MXR(mstatus) ((mstatus & 0x80000) != 0)
#define MODE_U 0x0
#define MODE_S 0x1
#define MODE_H 0x2
#define MODE_M 0x3

#define EXC_NONE 31
#define EXCI_USI 0 // user soft interrupt
#define EXCI_SSI 1 // supervisor soft interrupt
#define EXCI_MSI 3 // machine soft interrupt
#define EXCI_UTIMER 4
#define EXCI_STIMER 5
#define EXCI_MTIMER 7
#define EXCI_UEXT 8
#define EXCI_SEXT 9
#define EXCI_MEXT 11
#define EXCI_COUNTEROV 13
#define EXC_IAM 0 // inst address misaligned
#define EXC_IAF 1 // inst access fault
#define EXC_II 2 // illegal inst
#define EXC_BP 3 // breakpoint
#define EXC_LAM 4 // load address misaligned
#define EXC_LAF 5 // load address fault
#define EXC_SAM 6 // store address misaligned
#define EXC_SAF 7 // store address fault
#define EXC_ECU 8 // environment call from U-mode
#define EXC_ECS 9 // environment call from S-mode
#define EXC_ECM 11 // environment call from M-mode
#define EXC_IPF 12 // inst page fault
#define EXC_LPF 13 // load page fault
#define EXC_SPF 15 // store page fault
#define EXC_DT 16 // double trap
#define EXC_SC 18 // software check
#define EXC_HE 19 // hardware error
#define EXC_SRET 20 // user defined.sret 
#define EXC_MRET 21 // user defined.mret
#define EXC_EC 22 // user defined.environment call



constexpr size_t ARCH_GPR = offsetof(ArchState, gpr);
constexpr size_t ARCH_FPR = offsetof(ArchState, fpr);
constexpr size_t ARCH_FRM = offsetof(ArchState, frm);
constexpr size_t ARCH_FP_STATUS = offsetof(ArchState, fp_status);
constexpr size_t ARCH_FFLAGS = offsetof(ArchState, fp_status.float_exception_flags);
constexpr size_t ARCH_LOAD_RES = offsetof(ArchState, load_res);
constexpr size_t ARCH_LOAD_VAL = offsetof(ArchState, load_val);
constexpr size_t ARCH_BADADDR = offsetof(ArchState, badaddr);
constexpr size_t ARCH_PRIV_VER = offsetof(ArchState, priv_ver);
constexpr size_t ARCH_MISA = offsetof(ArchState, misa);
constexpr size_t ARCH_PRIV = offsetof(ArchState, priv);
constexpr size_t ARCH_MENVCFG = offsetof(ArchState, menvcfg);
constexpr size_t ARCH_SENVCFG = offsetof(ArchState, senvcfg);
constexpr size_t ARCH_MHARTID = offsetof(ArchState, mhartid);
constexpr size_t ARCH_MSTATUS = offsetof(ArchState, mstatus);
constexpr size_t ARCH_MIP = offsetof(ArchState, mip);
constexpr size_t ARCH_MICLAIM = offsetof(ArchState, miclaim);
constexpr size_t ARCH_MVENDORID = offsetof(ArchState, mvendorid);
constexpr size_t ARCH_MARCHID = offsetof(ArchState, marchid);
constexpr size_t ARCH_MIMPID = offsetof(ArchState, mimpid);
constexpr size_t ARCH_MIE = offsetof(ArchState, mie);
constexpr size_t ARCH_MIDELEG = offsetof(ArchState, mideleg);
constexpr size_t ARCH_SIE = offsetof(ArchState, sie);
constexpr size_t ARCH_SATP = offsetof(ArchState, satp);
constexpr size_t ARCH_STVAL = offsetof(ArchState, stval);
constexpr size_t ARCH_MEDELEG = offsetof(ArchState, medeleg);
constexpr size_t ARCH_STVEC = offsetof(ArchState, stvec);
constexpr size_t ARCH_SEPC = offsetof(ArchState, sepc);
constexpr size_t ARCH_SCAUSE = offsetof(ArchState, scause);
constexpr size_t ARCH_MTVEC = offsetof(ArchState, mtvec);
constexpr size_t ARCH_MEPC = offsetof(ArchState, mepc);
constexpr size_t ARCH_MCAUSE = offsetof(ArchState, mcause);
constexpr size_t ARCH_MTVAL = offsetof(ArchState, mtval);
constexpr size_t ARCH_SCOUNTEREN = offsetof(ArchState, scounteren);
constexpr size_t ARCH_MCOUNTEREN = offsetof(ArchState, mcounteren);
constexpr size_t ARCH_MCOUNTINHIBIT = offsetof(ArchState, mcountinhibit);
constexpr size_t ARCH_SSCRATCH = offsetof(ArchState, sscratch);
constexpr size_t ARCH_MSCRATCH = offsetof(ArchState, mscratch);
constexpr size_t ARCH_STIMECMP = offsetof(ArchState, stimecmp);
constexpr size_t ARCH_PMPCFG = offsetof(ArchState, pmp_table.pmp);
constexpr size_t ARCH_PMPADDR = offsetof(ArchState, pmp_table.addr);



} // namespace cds::arch::riscv

#endif
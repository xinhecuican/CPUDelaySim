#ifndef CSRDEFINES_H
#define CSRDEFINES_H
#include "arch/riscv/archstate.h"

namespace cds::arch::riscv {

    
/* Floating point round mode */
#define FSR_RD_SHIFT        5
#define FSR_RD              (0x7 << FSR_RD_SHIFT)

/* Floating point accrued exception flags */
#define FPEXC_NX            0x01
#define FPEXC_UF            0x02
#define FPEXC_OF            0x04
#define FPEXC_DZ            0x08
#define FPEXC_NV            0x10

/* Floating point status register bits */
#define FSR_AEXC_SHIFT      0
#define FSR_NVA             (FPEXC_NV << FSR_AEXC_SHIFT)
#define FSR_OFA             (FPEXC_OF << FSR_AEXC_SHIFT)
#define FSR_UFA             (FPEXC_UF << FSR_AEXC_SHIFT)
#define FSR_DZA             (FPEXC_DZ << FSR_AEXC_SHIFT)
#define FSR_NXA             (FPEXC_NX << FSR_AEXC_SHIFT)
#define FSR_AEXC            (FSR_NVA | FSR_OFA | FSR_UFA | FSR_DZA | FSR_NXA)

/* Control and Status Registers */

/* zicfiss user ssp csr */
#define CSR_SSP             0x011

/* User Trap Setup */
#define CSR_USTATUS         0x000
#define CSR_UIE             0x004
#define CSR_UTVEC           0x005

/* User Trap Handling */
#define CSR_USCRATCH        0x040
#define CSR_UEPC            0x041
#define CSR_UCAUSE          0x042
#define CSR_UTVAL           0x043
#define CSR_UIP             0x044

/* User Floating-Point CSRs */
#define CSR_FFLAGS          0x001
#define CSR_FRM             0x002
#define CSR_FCSR            0x003


/* User Timers and Counters */
#define CSR_CYCLE           0xc00
#define CSR_TIME            0xc01
#define CSR_INSTRET         0xc02
#define CSR_HPMCOUNTER3     0xc03
#define CSR_HPMCOUNTER4     0xc04
#define CSR_HPMCOUNTER5     0xc05
#define CSR_HPMCOUNTER6     0xc06
#define CSR_HPMCOUNTER7     0xc07
#define CSR_HPMCOUNTER8     0xc08
#define CSR_HPMCOUNTER9     0xc09
#define CSR_HPMCOUNTER10    0xc0a
#define CSR_HPMCOUNTER11    0xc0b
#define CSR_HPMCOUNTER12    0xc0c
#define CSR_HPMCOUNTER13    0xc0d
#define CSR_HPMCOUNTER14    0xc0e
#define CSR_HPMCOUNTER15    0xc0f
#define CSR_HPMCOUNTER16    0xc10
#define CSR_HPMCOUNTER17    0xc11
#define CSR_HPMCOUNTER18    0xc12
#define CSR_HPMCOUNTER19    0xc13
#define CSR_HPMCOUNTER20    0xc14
#define CSR_HPMCOUNTER21    0xc15
#define CSR_HPMCOUNTER22    0xc16
#define CSR_HPMCOUNTER23    0xc17
#define CSR_HPMCOUNTER24    0xc18
#define CSR_HPMCOUNTER25    0xc19
#define CSR_HPMCOUNTER26    0xc1a
#define CSR_HPMCOUNTER27    0xc1b
#define CSR_HPMCOUNTER28    0xc1c
#define CSR_HPMCOUNTER29    0xc1d
#define CSR_HPMCOUNTER30    0xc1e
#define CSR_HPMCOUNTER31    0xc1f
#define CSR_CYCLEH          0xc80
#define CSR_TIMEH           0xc81
#define CSR_INSTRETH        0xc82
#define CSR_HPMCOUNTER3H    0xc83
#define CSR_HPMCOUNTER4H    0xc84
#define CSR_HPMCOUNTER5H    0xc85
#define CSR_HPMCOUNTER6H    0xc86
#define CSR_HPMCOUNTER7H    0xc87
#define CSR_HPMCOUNTER8H    0xc88
#define CSR_HPMCOUNTER9H    0xc89
#define CSR_HPMCOUNTER10H   0xc8a
#define CSR_HPMCOUNTER11H   0xc8b
#define CSR_HPMCOUNTER12H   0xc8c
#define CSR_HPMCOUNTER13H   0xc8d
#define CSR_HPMCOUNTER14H   0xc8e
#define CSR_HPMCOUNTER15H   0xc8f
#define CSR_HPMCOUNTER16H   0xc90
#define CSR_HPMCOUNTER17H   0xc91
#define CSR_HPMCOUNTER18H   0xc92
#define CSR_HPMCOUNTER19H   0xc93
#define CSR_HPMCOUNTER20H   0xc94
#define CSR_HPMCOUNTER21H   0xc95
#define CSR_HPMCOUNTER22H   0xc96
#define CSR_HPMCOUNTER23H   0xc97
#define CSR_HPMCOUNTER24H   0xc98
#define CSR_HPMCOUNTER25H   0xc99
#define CSR_HPMCOUNTER26H   0xc9a
#define CSR_HPMCOUNTER27H   0xc9b
#define CSR_HPMCOUNTER28H   0xc9c
#define CSR_HPMCOUNTER29H   0xc9d
#define CSR_HPMCOUNTER30H   0xc9e
#define CSR_HPMCOUNTER31H   0xc9f

/* Machine Timers and Counters */
#define CSR_MCYCLE          0xb00
#define CSR_MINSTRET        0xb02
#define CSR_MCYCLEH         0xb80
#define CSR_MINSTRETH       0xb82

/* Machine Information Registers */
#define CSR_MVENDORID       0xf11
#define CSR_MARCHID         0xf12
#define CSR_MIMPID          0xf13
#define CSR_MHARTID         0xf14
#define CSR_MCONFIGPTR      0xf15

/* Machine Trap Setup */
#define CSR_MSTATUS         0x300
#define CSR_MISA            0x301
#define CSR_MEDELEG         0x302
#define CSR_MIDELEG         0x303
#define CSR_MIE             0x304
#define CSR_MTVEC           0x305
#define CSR_MCOUNTEREN      0x306

/* 32-bit only */
#define CSR_MSTATUSH        0x310
#define CSR_MEDELEGH        0x312
#define CSR_HEDELEGH        0x612

/* Machine Trap Handling */
#define CSR_MSCRATCH        0x340
#define CSR_MEPC            0x341
#define CSR_MCAUSE          0x342
#define CSR_MTVAL           0x343
#define CSR_MIP             0x344

/* Machine-Level Window to Indirectly Accessed Registers (AIA) */
#define CSR_MISELECT        0x350
#define CSR_MIREG           0x351

/* Machine-Level Interrupts (AIA) */
#define CSR_MTOPEI          0x35c
#define CSR_MTOPI           0xfb0

/* Virtual Interrupts for Supervisor Level (AIA) */
#define CSR_MVIEN           0x308
#define CSR_MVIP            0x309

/* Machine-Level High-Half CSRs (AIA) */
#define CSR_MIDELEGH        0x313
#define CSR_MIEH            0x314
#define CSR_MVIENH          0x318
#define CSR_MVIPH           0x319
#define CSR_MIPH            0x354

/* Supervisor Trap Setup */
#define CSR_SSTATUS         0x100
#define CSR_SIE             0x104
#define CSR_STVEC           0x105
#define CSR_SCOUNTEREN      0x106

/* Supervisor Configuration CSRs */
#define CSR_SENVCFG         0x10A

/* Supervisor state CSRs */
#define CSR_SSTATEEN0       0x10C
#define CSR_SSTATEEN1       0x10D
#define CSR_SSTATEEN2       0x10E
#define CSR_SSTATEEN3       0x10F

/* Supervisor Trap Handling */
#define CSR_SSCRATCH        0x140
#define CSR_SEPC            0x141
#define CSR_SCAUSE          0x142
#define CSR_STVAL           0x143
#define CSR_SIP             0x144

/* Sstc supervisor CSRs */
#define CSR_STIMECMP        0x14D
#define CSR_STIMECMPH       0x15D

/* Supervisor Protection and Translation */
#define CSR_SPTBR           0x180
#define CSR_SATP            0x180

/* Supervisor-Level Window to Indirectly Accessed Registers (AIA) */
#define CSR_SISELECT        0x150
#define CSR_SIREG           0x151

/* Supervisor-Level Interrupts (AIA) */
#define CSR_STOPEI          0x15c
#define CSR_STOPI           0xdb0

/* Supervisor-Level High-Half CSRs (AIA) */
#define CSR_SIEH            0x114
#define CSR_SIPH            0x154


/* Machine Configuration CSRs */
#define CSR_MENVCFG         0x30A
#define CSR_MENVCFGH        0x31A

/* Machine state CSRs */
#define CSR_MSTATEEN0       0x30C
#define CSR_MSTATEEN0H      0x31C
#define CSR_MSTATEEN1       0x30D
#define CSR_MSTATEEN1H      0x31D
#define CSR_MSTATEEN2       0x30E
#define CSR_MSTATEEN2H      0x31E
#define CSR_MSTATEEN3       0x30F
#define CSR_MSTATEEN3H      0x31F


/* Enhanced Physical Memory Protection (ePMP) */
#define CSR_MSECCFG         0x747
#define CSR_MSECCFGH        0x757
/* Physical Memory Protection */
#define CSR_PMPCFG0         0x3a0
#define CSR_PMPCFG1         0x3a1
#define CSR_PMPCFG2         0x3a2
#define CSR_PMPCFG3         0x3a3
#define CSR_PMPADDR0        0x3b0
#define CSR_PMPADDR1        0x3b1
#define CSR_PMPADDR2        0x3b2
#define CSR_PMPADDR3        0x3b3
#define CSR_PMPADDR4        0x3b4
#define CSR_PMPADDR5        0x3b5
#define CSR_PMPADDR6        0x3b6
#define CSR_PMPADDR7        0x3b7
#define CSR_PMPADDR8        0x3b8
#define CSR_PMPADDR9        0x3b9
#define CSR_PMPADDR10       0x3ba
#define CSR_PMPADDR11       0x3bb
#define CSR_PMPADDR12       0x3bc
#define CSR_PMPADDR13       0x3bd
#define CSR_PMPADDR14       0x3be
#define CSR_PMPADDR15       0x3bf


/* Performance Counters */
#define CSR_MHPMCOUNTER3    0xb03
#define CSR_MHPMCOUNTER4    0xb04
#define CSR_MHPMCOUNTER5    0xb05
#define CSR_MHPMCOUNTER6    0xb06
#define CSR_MHPMCOUNTER7    0xb07
#define CSR_MHPMCOUNTER8    0xb08
#define CSR_MHPMCOUNTER9    0xb09
#define CSR_MHPMCOUNTER10   0xb0a
#define CSR_MHPMCOUNTER11   0xb0b
#define CSR_MHPMCOUNTER12   0xb0c
#define CSR_MHPMCOUNTER13   0xb0d
#define CSR_MHPMCOUNTER14   0xb0e
#define CSR_MHPMCOUNTER15   0xb0f
#define CSR_MHPMCOUNTER16   0xb10
#define CSR_MHPMCOUNTER17   0xb11
#define CSR_MHPMCOUNTER18   0xb12
#define CSR_MHPMCOUNTER19   0xb13
#define CSR_MHPMCOUNTER20   0xb14
#define CSR_MHPMCOUNTER21   0xb15
#define CSR_MHPMCOUNTER22   0xb16
#define CSR_MHPMCOUNTER23   0xb17
#define CSR_MHPMCOUNTER24   0xb18
#define CSR_MHPMCOUNTER25   0xb19
#define CSR_MHPMCOUNTER26   0xb1a
#define CSR_MHPMCOUNTER27   0xb1b
#define CSR_MHPMCOUNTER28   0xb1c
#define CSR_MHPMCOUNTER29   0xb1d
#define CSR_MHPMCOUNTER30   0xb1e
#define CSR_MHPMCOUNTER31   0xb1f

/* Machine counter-inhibit register */
#define CSR_MCOUNTINHIBIT   0x320

/* Machine counter configuration registers */
#define CSR_MCYCLECFG       0x321
#define CSR_MINSTRETCFG     0x322

#define CSR_MHPMEVENT3      0x323
#define CSR_MHPMEVENT4      0x324
#define CSR_MHPMEVENT5      0x325
#define CSR_MHPMEVENT6      0x326
#define CSR_MHPMEVENT7      0x327
#define CSR_MHPMEVENT8      0x328
#define CSR_MHPMEVENT9      0x329
#define CSR_MHPMEVENT10     0x32a
#define CSR_MHPMEVENT11     0x32b
#define CSR_MHPMEVENT12     0x32c
#define CSR_MHPMEVENT13     0x32d
#define CSR_MHPMEVENT14     0x32e
#define CSR_MHPMEVENT15     0x32f
#define CSR_MHPMEVENT16     0x330
#define CSR_MHPMEVENT17     0x331
#define CSR_MHPMEVENT18     0x332
#define CSR_MHPMEVENT19     0x333
#define CSR_MHPMEVENT20     0x334
#define CSR_MHPMEVENT21     0x335
#define CSR_MHPMEVENT22     0x336
#define CSR_MHPMEVENT23     0x337
#define CSR_MHPMEVENT24     0x338
#define CSR_MHPMEVENT25     0x339
#define CSR_MHPMEVENT26     0x33a
#define CSR_MHPMEVENT27     0x33b
#define CSR_MHPMEVENT28     0x33c
#define CSR_MHPMEVENT29     0x33d
#define CSR_MHPMEVENT30     0x33e
#define CSR_MHPMEVENT31     0x33f

#define CSR_MCYCLECFGH      0x721
#define CSR_MINSTRETCFGH    0x722

#define CSR_MHPMEVENT3H     0x723
#define CSR_MHPMEVENT4H     0x724
#define CSR_MHPMEVENT5H     0x725
#define CSR_MHPMEVENT6H     0x726
#define CSR_MHPMEVENT7H     0x727
#define CSR_MHPMEVENT8H     0x728
#define CSR_MHPMEVENT9H     0x729
#define CSR_MHPMEVENT10H    0x72a
#define CSR_MHPMEVENT11H    0x72b
#define CSR_MHPMEVENT12H    0x72c
#define CSR_MHPMEVENT13H    0x72d
#define CSR_MHPMEVENT14H    0x72e
#define CSR_MHPMEVENT15H    0x72f
#define CSR_MHPMEVENT16H    0x730
#define CSR_MHPMEVENT17H    0x731
#define CSR_MHPMEVENT18H    0x732
#define CSR_MHPMEVENT19H    0x733
#define CSR_MHPMEVENT20H    0x734
#define CSR_MHPMEVENT21H    0x735
#define CSR_MHPMEVENT22H    0x736
#define CSR_MHPMEVENT23H    0x737
#define CSR_MHPMEVENT24H    0x738
#define CSR_MHPMEVENT25H    0x739
#define CSR_MHPMEVENT26H    0x73a
#define CSR_MHPMEVENT27H    0x73b
#define CSR_MHPMEVENT28H    0x73c
#define CSR_MHPMEVENT29H    0x73d
#define CSR_MHPMEVENT30H    0x73e
#define CSR_MHPMEVENT31H    0x73f

#define CSR_MHPMCOUNTER3H   0xb83
#define CSR_MHPMCOUNTER4H   0xb84
#define CSR_MHPMCOUNTER5H   0xb85
#define CSR_MHPMCOUNTER6H   0xb86
#define CSR_MHPMCOUNTER7H   0xb87
#define CSR_MHPMCOUNTER8H   0xb88
#define CSR_MHPMCOUNTER9H   0xb89
#define CSR_MHPMCOUNTER10H  0xb8a
#define CSR_MHPMCOUNTER11H  0xb8b
#define CSR_MHPMCOUNTER12H  0xb8c
#define CSR_MHPMCOUNTER13H  0xb8d
#define CSR_MHPMCOUNTER14H  0xb8e
#define CSR_MHPMCOUNTER15H  0xb8f
#define CSR_MHPMCOUNTER16H  0xb90
#define CSR_MHPMCOUNTER17H  0xb91
#define CSR_MHPMCOUNTER18H  0xb92
#define CSR_MHPMCOUNTER19H  0xb93
#define CSR_MHPMCOUNTER20H  0xb94
#define CSR_MHPMCOUNTER21H  0xb95
#define CSR_MHPMCOUNTER22H  0xb96
#define CSR_MHPMCOUNTER23H  0xb97
#define CSR_MHPMCOUNTER24H  0xb98
#define CSR_MHPMCOUNTER25H  0xb99
#define CSR_MHPMCOUNTER26H  0xb9a
#define CSR_MHPMCOUNTER27H  0xb9b
#define CSR_MHPMCOUNTER28H  0xb9c
#define CSR_MHPMCOUNTER29H  0xb9d
#define CSR_MHPMCOUNTER30H  0xb9e
#define CSR_MHPMCOUNTER31H  0xb9f


/* mstatus CSR bits */
#define MSTATUS_UIE         0x00000001
#define MSTATUS_SIE         0x00000002
#define MSTATUS_MIE         0x00000008
#define MSTATUS_UPIE        0x00000010
#define MSTATUS_SPIE        0x00000020
#define MSTATUS_UBE         0x00000040
#define MSTATUS_MPIE        0x00000080
#define MSTATUS_SPP         0x00000100
#define MSTATUS_VS          0x00000600
#define MSTATUS_MPP         0x00001800
#define MSTATUS_FS          0x00006000
#define MSTATUS_XS          0x00018000
#define MSTATUS_MPRV        0x00020000
#define MSTATUS_SUM         0x00040000 /* since: priv-1.10 */
#define MSTATUS_MXR         0x00080000
#define MSTATUS_TVM         0x00100000 /* since: priv-1.10 */
#define MSTATUS_TW          0x00200000 /* since: priv-1.10 */
#define MSTATUS_TSR         0x00400000 /* since: priv-1.10 */
#define MSTATUS_SPELP       0x00800000 /* zicfilp */
#define MSTATUS_MPELP       0x020000000000 /* zicfilp */
#define MSTATUS_GVA         0x4000000000ULL
#define MSTATUS_MPV         0x8000000000ULL

#define MSTATUS64_UXL       0x0000000300000000ULL
#define MSTATUS64_SXL       0x0000000C00000000ULL

#define MSTATUS_MPP_SHIFT   11
#define MSTATUS_SPP_SHIFT   8
#define MSTATUS_MPRV_SHIFT  17ULL

#define MSTATUS32_SD        31
#define MSTATUS64_SD        63

#define MSTATUS_MASK (MSTATUS_SIE | MSTATUS_SPIE | MSTATUS_MIE | MSTATUS_MPIE | \
                     MSTATUS_SPP | MSTATUS_MPRV | MSTATUS_SUM | \
                     MSTATUS_MPP | MSTATUS_MXR | MSTATUS_TVM | MSTATUS_TSR | \
                     MSTATUS_TW | MSTATUS_FS)
#define SSTATUS_MASK  (MSTATUS_SIE | MSTATUS_SPIE | MSTATUS_UIE | MSTATUS_UPIE | \
                     MSTATUS_SPP | MSTATUS_FS | MSTATUS_XS | MSTATUS_SUM | \
                     MSTATUS_MXR)
/* mip masks */
#define MIP_SSIP                           (1 << EXCI_SSI)
#define MIP_MSIP                           (1 << EXCI_MSI)
#define MIP_STIP                           (1 << EXCI_STIMER)
#define MIP_MTIP                           (1 << EXCI_MTIMER)
#define MIP_SEIP                           (1 << EXCI_SEXT)
#define MIP_MEIP                           (1 << EXCI_MEXT)



/* MIE masks */
#define MIE_SEIE                           (1 << EXCI_SEXT)
#define MIE_UEIE                           (1 << EXCI_MEXT)
#define MIE_STIE                           (1 << EXCI_STIMER)
#define MIE_UTIE                           (1 << EXCI_UTIMER)
#define MIE_SSIE                           (1 << EXCI_SSI)
#define MIE_USIE                           (1 << EXCI_USI)

/* Machine constants */
#define M_MODE_INTERRUPTS  ((uint64_t)(MIP_MSIP | MIP_MTIP | MIP_MEIP))
#define S_MODE_INTERRUPTS  ((uint64_t)(MIP_SSIP | MIP_STIP | MIP_SEIP))
#define LOCAL_INTERRUPTS   (~0x1FFFULL)



#define DELEGABLE_EXCPS ((1ULL << (EXC_IAM)) | \
                         (1ULL << (EXC_IAF)) | \
                         (1ULL << (EXC_II)) | \
                         (1ULL << (EXC_BP)) | \
                         (1ULL << (EXC_LAM)) | \
                         (1ULL << (EXC_LAF)) | \
                         (1ULL << (EXC_SAM)) | \
                         (1ULL << (EXC_SAF)) | \
                         (1ULL << (EXC_ECU)) | \
                         (1ULL << (EXC_ECS)) | \
                         (1ULL << (EXC_ECM)) | \
                         (1ULL << (EXC_IPF)) | \
                         (1ULL << (EXC_LPF)) | \
                         (1ULL << (EXC_SPF)) | \
                         (1ULL << (EXC_DT)) | \
                         (1ULL << (EXC_HE)))

/* Execution environment configuration bits */
#define MENVCFG_FIOM                       BIT(0)
#define MENVCFG_LPE                        BIT(2) /* zicfilp */
#define MENVCFG_SSE                        BIT(3) /* zicfiss */
#define MENVCFG_CBIE                       (3UL << 4)
#define MENVCFG_CBCFE                      BIT(6)
#define MENVCFG_CBZE                       BIT(7)
#define MENVCFG_ADUE                       (1ULL << 61)
#define MENVCFG_PBMTE                      (1ULL << 62)
#define MENVCFG_STCE                       (1ULL << 63)

#define MENVCFG_MASK (MENVCFG_FIOM | MENVCFG_CBIE | MENVCFG_CBCFE | MENVCFG_CBZE)


#define COUNTEREN_CY         (1 << 0)
#define COUNTEREN_TM         (1 << 1)
#define COUNTEREN_IR         (1 << 2)
#define COUNTEREN_HPM3       (1 << 3)

#define FSR_AEXC_SHIFT      0
#define FSR_NVA             (FPEXC_NV << FSR_AEXC_SHIFT)
#define FSR_OFA             (FPEXC_OF << FSR_AEXC_SHIFT)
#define FSR_UFA             (FPEXC_UF << FSR_AEXC_SHIFT)
#define FSR_DZA             (FPEXC_DZ << FSR_AEXC_SHIFT)
#define FSR_NXA             (FPEXC_NX << FSR_AEXC_SHIFT)
#define FSR_AEXC            (FSR_NVA | FSR_OFA | FSR_UFA | FSR_DZA | FSR_NXA)

#define FSR_RD_SHIFT        5
#define FSR_RD              (0x7 << FSR_RD_SHIFT)

/* RV64 satp CSR field masks */
#define SATP64_MODE         0xF000000000000000ULL
#define SATP64_ASID         0x0FFFF00000000000ULL
#define SATP64_PPN          0x00000FFFFFFFFFFFULL

enum {
    PRIV_VERSION_1_10_0 = 0,
    PRIV_VERSION_1_11_0,
    PRIV_VERSION_1_12_0,
    PRIV_VERSION_1_13_0,

    PRIV_VERSION_LATEST = PRIV_VERSION_1_13_0,
};

// 修复typedef声明以避免编译器错误
typedef void (*riscv_csr_predicate_fn)(DisasContext*, int);
typedef void (*riscv_csr_read_fn)(DisasContext*, int, uint64_t*);
typedef void (*riscv_csr_write_fn)(DisasContext*, int, uint64_t);
typedef void (*riscv_csr_op_fn)(DisasContext*, int, uint64_t*, uint64_t, uint64_t);


struct riscv_csr_operations{
    const char *name;
    riscv_csr_predicate_fn predicate;
    riscv_csr_read_fn read;
    riscv_csr_write_fn write;
    riscv_csr_op_fn op;
    /* The default priv spec version should be PRIV_VERSION_1_10_0 (i.e 0) */
    uint32_t min_priv_ver;
};

/* CSR function table constants */
enum {
    CSR_TABLE_SIZE = 0x1000
};

}

#endif
#ifndef COMMON_EMU_PROXY_H
#define COMMON_EMU_PROXY_H
#include "common/common.h"
#include <unistd.h>
#include <dlfcn.h>

typedef struct {
  uint64_t gpr[32];
  uint64_t fpr[32];
} arch_reg_state_t;

typedef struct {
  uint64_t priviledgeMode;
  uint64_t mstatus;
  uint64_t sstatus;
  uint64_t mepc;
  uint64_t sepc;
  uint64_t mtval;
  uint64_t stval;
  uint64_t mtvec;
  uint64_t stvec;
  uint64_t mcause;
  uint64_t scause;
  uint64_t satp;
  uint64_t mip;
  uint64_t mie;
  uint64_t mscratch;
  uint64_t sscratch;
  uint64_t mideleg;
  uint64_t medeleg;
  uint64_t this_pc;

  uint64_t fcsr;
} arch_csr_state_t;

typedef struct {
  arch_reg_state_t regs;
  arch_csr_state_t csrs;
} arch_core_state_t;

struct ExecutionGuide {
  // force raise exception
  bool force_raise_exception;
  uint64_t exception_num;
  uint64_t mtval;
  uint64_t stval;
  // force set jump target
  bool force_set_jump_target;
  uint64_t jump_target;
};

struct SyncState {
  uint64_t lrscValid;
  uint64_t lrscAddr;
};

enum { REF_TO_DUT, DUT_TO_REF };

class NemuProxy {
public:
    static NemuProxy& getInstance() {
        static NemuProxy instance;  // C++11保证线程安全
        return instance;
    }
  // public callable functions
  void (*memcpy)(uint64_t nemu_addr, void *dut_buf, size_t n, bool direction);
  void (*regcpy)(void *dut, bool direction);
  void (*mpfcpy)(void *dut, bool direction);
  void (*csrcpy)(void *dut, bool direction);
  void (*uarchstatus_cpy)(void *dut, bool direction);
  int (*store_commit)(uint64_t *saddr, uint64_t *sdata, uint8_t *smask);
  void (*exec)(uint64_t n);
  uint64_t (*guided_exec)(void *disambiguate_para);
  uint64_t (*update_config)(void *config);
  void (*raise_intr)(uint64_t no);
  void (*isa_reg_display)();
  void (*query)(void *result_buffer, uint64_t type);

  NemuProxy();

  uint64_t pc = 0x80000000;
private:
};

#endif
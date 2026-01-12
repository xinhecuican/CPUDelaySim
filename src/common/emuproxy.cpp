#include "common/emuproxy.h"


uint8_t* goldenMem = NULL;
const char *difftest_ref_so = "/home/lizilin/projects/DOoO-riscv/utils/NEMU/build/riscv64-nemu-interpreter-so";

#define check_and_assert(func)                                \
  do {                                                        \
    if (!func) {                                              \
      printf("ERROR: %s\n", dlerror());  \
      assert(func);                                           \
    }                                                         \
  } while (0);

NemuProxy::NemuProxy() {
#ifdef DIFFTEST
  if (difftest_ref_so == NULL) {
    printf("--diff is not given, "
        "try to use $(NEMU_HOME)/build/riscv64-nemu-interpreter-so by default\n");
    const char *nemu_home = getenv("NEMU_HOME");
    if (nemu_home == NULL) {
      printf("FATAL: $(NEMU_HOME) is not defined!\n");
      exit(1);
    }
    const char *so = "/build/riscv64-nemu-interpreter-so";
    char *buf = (char *)malloc(strlen(nemu_home) + strlen(so) + 1);
    strcpy(buf, nemu_home);
    strcat(buf, so);
    difftest_ref_so = buf;
  }

  printf("NemuProxy using %s\n", difftest_ref_so);

  void *handle = dlmopen(LM_ID_NEWLM, difftest_ref_so, RTLD_LAZY | RTLD_DEEPBIND);
  if(!handle){
    printf("%s\n", dlerror());
    assert(0);
  }

  this->memcpy = (void (*)(uint64_t, void *, size_t, bool))dlsym(handle, "difftest_memcpy");
  check_and_assert(this->memcpy);

  regcpy = (void (*)(void *, bool))dlsym(handle, "difftest_regcpy");
  check_and_assert(regcpy);

  mpfcpy = (void (*)(void *, bool))dlsym(handle, "difftest_mpfcpy");
  check_and_assert(mpfcpy);

  csrcpy = (void (*)(void *, bool))dlsym(handle, "difftest_csrcpy");
  check_and_assert(csrcpy);

  uarchstatus_cpy = (void (*)(void *, bool))dlsym(handle, "difftest_uarchstatus_cpy");
  check_and_assert(uarchstatus_cpy);

  exec = (void (*)(uint64_t))dlsym(handle, "difftest_exec");
  check_and_assert(exec);

  guided_exec = (uint64_t (*)(void *))dlsym(handle, "difftest_guided_exec");
  check_and_assert(guided_exec);

  update_config = (uint64_t (*)(void *))dlsym(handle, "update_dynamic_config");
  check_and_assert(update_config);

  store_commit = (int (*)(uint64_t*, uint64_t*, uint8_t*))dlsym(handle, "difftest_store_commit");
  check_and_assert(store_commit);

  raise_intr = (void (*)(uint64_t))dlsym(handle, "difftest_raise_intr");
  check_and_assert(raise_intr);

  isa_reg_display = (void (*)(void))dlsym(handle, "isa_reg_display");
  check_and_assert(isa_reg_display);

  query = (void (*)(void*, uint64_t))dlsym(handle, "difftest_query_ref");

  auto nemu_difftest_set_mhartid = (void (*)(int))dlsym(handle, "difftest_set_mhartid");
  auto nemu_misc_put_gmaddr = (void (*)(void*))dlsym(handle, "difftest_put_gmaddr");

  auto nemu_init = (void (*)(void))dlsym(handle, "difftest_init");
  check_and_assert(nemu_init);

  nemu_init();

#endif
}
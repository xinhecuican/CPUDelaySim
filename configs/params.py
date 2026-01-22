
class CPU:
    cxx_header = "cpu/cpu.h"
    fetch_width = 4
    retire_size = 256

class AtomicCPU(CPU):
    cxx_header = "cpu/atomiccpu.h"

class CacheCPU(CPU):
    cxx_header = "cpu/cachecpu.h"

class PipelineCPU(CPU):
    cxx_header = "cpu/pipelinecpu.h"
    mult_delay = 2
    div_delay = 14
    fadd_delay = 2
    fmul_delay = 2
    fma_delay = 4
    fdiv_delay = 19
    fsqrt_delay = 19
    fmisc_complex_delay = 1
    retire_size = 10

class Predictor:
    cxx_header = "pred/predictor.h"
    retire_size = CPU.retire_size

class GHR:
    cxx_header = "pred/history/ghr.h"
    ghr_size = 64

class HistoryManager:
    cxx_header = "pred/history/history.h"

class GShareBP:
    cxx_header = "pred/bp/gshare.h"
    delay = 0
    table_size = 1024
    bit_size = 2
    offset = 1

class BTB:
    cxx_header = "pred/bp/btb.h"
    table_size = 256
    tag_size = 20
    offset = 1


class Cache:
    cxx_header = "cache/cache.h"
    line_size = 64
    set_size = 64
    way = 8
    delay = 1
    level = 1
    replace_method = "lru"

class ICache(Cache):
    cxx_header = "cache/icache.h"

class DCache(Cache):
    cxx_header = "cache/dcache.h"

class Memory(Cache):
    cxx_header = "cache/memory.h"
    size = 0x40000000
    dram_config = "thirdparty/DRAMsim3/configs/xiangshan_DDR4_8Gb_x8_3200_2ch.ini"
    dram_queue_size = 32

class CacheManager:
    cxx_header = "cache/cachemanager.h"
    icache_id = 0
    dcache_id = 1

class Uart:
    cxx_header = "device/uart.h"
    base_addr = 0x10000000
    baudrate = 115200
    irq_number = 1
    delay = 1

class SimpleUart:
    cxx_header = "device/simpleuart.h"
    base_addr = 0x10000000

class BasicIrqHandler:
    cxx_header = "device/basicirqhandler.h"
    base_addr = 0xc000000
    range = 0x4000000

class Clint:
    cxx_header = "arch/riscv/clint.h"
    base_addr = 0x2000000

class CPU:
    cxx_header = "cpu/cpu.h"
    fetch_width = 4
    retire_size = 256

class AtomicCPU(CPU):
    cxx_header = "cpu/atomiccpu.h"

# class Frontend:
#     inst_buffer_size = 16
#     fetch_width = CPU.fetch_width
#     retire_size = CPU.retire_size

# class DRFrontend(Frontend):
#     cacheid = 0
#     ibuf_size = 32
#     block_size = 8

# class Predictor:
#     stage = 0

# class FsqPredictor(Predictor):
#     stream_size = 32
#     block_size = 8

# class GHR:
#     ghr_size = 64

# class BTB:
#     tag_size = 20
#     target_size = 20
#     offset = 2
#     stage = 0

# class BasicBTB(BTB):
#     table_size = 1024

# class BP:
#     stage = 0

# class BimodalBP(BP):
#     table_size = 1024
#     bits = 2

# class Backend:
#     fetch_width = CPU.fetch_width
#     retire_size = CPU.retire_size

# class PRFRename:
#     preg_size = 128
#     vreg_size = 32


class Cache:
    cxx_header = "cache/cache.h"
    line_size = 64
    set_size = 64
    way = 8
    delay = 1
    level = 0
    id = 0
    replace_method = "lru"

class ICache(Cache):
    cxx_header = "cache/icache.h"

class DCache(Cache):
    cxx_header = "cache/dcache.h"

class Memory(Cache):
    cxx_header = "cache/memory.h"
    size = 0x40000000
    dram_config = "thirdparty/DRAMsim3/configs/xiangshan_DDR4_8Gb_x8_3200_2ch.ini"
    dram_outpath = "output/"
    dram_queue_size = 32

class CacheManager:
    cxx_header = "cache/cachemanager.h"

class Uart:
    cxx_header = "device/uart.h"
    base_addr = 0x10000000
    baudrate = 115200
    irq_number = 10
    delay = 1
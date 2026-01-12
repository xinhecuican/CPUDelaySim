#include "cache/memory.h"
#include <sys/mman.h>
#include "common/log.h"
#include "config.h"
#include "common/emuproxy.h"
#include "common/log.h"

Memory::Memory(const std::string& filename) {
    this->filename = filename;
}

Memory::~Memory() {
    if (ram != NULL) {
        munmap(ram, size);
    }
#ifdef DRAMSIM
    while (!dram_idle_queue.empty()) {
        CoDRAMRequest* req = dram_idle_queue.front();
        dram_idle_queue.pop();
        delete (DRAMMeta*)req->meta;
        delete req;
    }
#endif

    while (!device_idle_queue.empty()) {
        DeviceReq* req = device_idle_queue.front();
        device_idle_queue.pop();
        delete req;
    }
    for (auto device : devices) {
        delete device;
    }
    delete result;
}

void Memory::afterLoad() {
    result = new CacheTagv;
    result->valid = true;
    result->shared = true;
    result->dirty = false;
    ram = (uint8_t *)mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);
    if (ram == (uint8_t*)MAP_FAILED) {
        Log::error("could not mmap memory size {}", size);
        exit(1);
    }

#ifdef LOG_MEM
    Log::init("mem", Config::getLogFilePath("mem.log"));
#endif

#ifdef DRAMSIM
    for (int i = 0; i < dram_queue_size; i++) {
        DRAMMeta* meta = new DRAMMeta;
        dram_idle_queue.push(new CoDRAMRequest(0, 0, meta));
    }
    dram_outpath = Config::getLogFilePath("");
    dram = new ComplexCoDRAMsim3(dram_config, dram_outpath);
#endif

    for (auto device : devices) {
        device->afterLoad();
        device_idle_queue.push(new DeviceReq);
    }

    FILE *fp = fopen(filename.c_str(), "rb");
    if (fp == NULL) {
      printf("Can not open '%s'\n", filename.c_str());
      assert(0);
    }

    fseek(fp, 0, SEEK_END);
    filesize = ftell(fp);
    if (filesize > size) {
      filesize = size;
    }

    fseek(fp, 0, SEEK_SET);
    int ret = fread(ram, filesize, 1, fp);
#ifdef DIFFTEST
    NemuProxy::getInstance().memcpy(0x80000000, ram, filesize, DUT_TO_REF);
#endif
    assert(ret == 1);
    fclose(fp);
}

bool Memory::lookup(int id, snoop_req_t req, uint64_t addr, uint32_t size) {
    addr &= 0xffffffffff;
    if (req <= READ_END) {
        return memoryRead(id, addr, size);
    } else {
        return memoryWrite(id, addr, size);
    }
}

void Memory::tick() {
#ifdef DRAMSIM
    dram->tick();
    CoDRAMResponse* rresp = dram->check_read_response();
    if (rresp != NULL) {
        DRAMMeta* meta = (DRAMMeta*)rresp->req->meta;
        callbacks[0](meta->id, result);
        dram_idle_queue.push(const_cast<CoDRAMRequest*>(rresp->req));
    }
    CoDRAMResponse* wresp = dram->check_write_response();
    if (wresp != NULL) {
        DRAMMeta* meta = (DRAMMeta*)wresp->req->meta;
        callbacks[0](meta->id, result);
        dram_idle_queue.push(const_cast<CoDRAMRequest*>(wresp->req));
    }
#endif
    for (auto device : devices) {
        device->tick();
    }
}

bool Memory::memoryRead(int id, uint64_t addr, uint32_t size) {
    for (Device* device: devices) {
        if (unlikely(device->inRange(addr))) {
            if (device_idle_queue.empty()) {
                return false;
            }
            DeviceReq* req = device_idle_queue.front();
            req->id = id;
            req->addr = addr;   
            req->size = size;
            req->is_write = false;
            bool success = device->addRequest(req);
            if (success) {
                device_idle_queue.pop();
            }
            return success;
        }
    }
#ifdef DRAMSIM
    if (unlikely(dram_idle_queue.empty())) {
        return false;
    }
    CoDRAMRequest* req = dram_idle_queue.front();
    dram_idle_queue.pop();
    ((DRAMMeta*)req->meta)->id = id;
    ((DRAMMeta*)req->meta)->addr = addr;
    ((DRAMMeta*)req->meta)->size = size;
    req->address = addr;
    req->is_write = false;
    dram->add_request(req);
    return true;
#else
    callbacks[0](id, result);
    return true;
#endif
}

bool Memory::memoryWrite(int id, uint64_t addr, uint32_t size) {
    for (Device* device: devices) {
        if (unlikely(device->inRange(addr))) {
            if (device_idle_queue.empty()) {
                return false;
            }
            DeviceReq* req = device_idle_queue.front();
            req->id = id;
            req->addr = addr;   
            req->size = size;
            req->is_write = true;
            bool success = device->addRequest(req);
            if (success) {
                device_idle_queue.pop();
            }
            return success;
        }
    }
#ifdef DRAMSIM
    if (unlikely(dram_idle_queue.empty())) {
        return false;
    }
    CoDRAMRequest* req = dram_idle_queue.front();
    dram_idle_queue.pop();
    ((DRAMMeta*)req->meta)->id = id;
    ((DRAMMeta*)req->meta)->addr = addr;
    ((DRAMMeta*)req->meta)->size = size;
    req->address = addr;
    req->is_write = true;
    dram->add_request(req);
    return true;
#else
    callbacks[0](id, result);
    return true;
#endif
}

void Memory::paddrRead(uint64_t paddr, uint32_t size, uint8_t* data, bool& mmio) {
    mmio = false;
    for (Device* device: devices) {
        if (unlikely(device->inRange(paddr))) {
            mmio = true;
            device->read(paddr, size, data);
#ifdef DIFFTEST
            NemuProxy::getInstance().memcpy(paddr, data, 1, DUT_TO_REF);
#endif
#ifdef LOG_MEM
            Log::trace("mem", "mmio read 0x{:x} {} {:x}", paddr, size, *data);
#endif
            return;
        }
    }
    memcpy(data, ram + paddr - 0x80000000, size);
#ifdef LOG_MEM
    uint64_t val = *((uint64_t*)(ram + paddr - 0x80000000));
    Log::trace("mem", "read 0x{:x} {} {:x}", paddr, size, val);
#endif
}

void Memory::paddrWrite(uint64_t paddr, uint32_t size, uint8_t* data, bool& mmio) {
    mmio = false;
    for (Device* device: devices) {
        if (unlikely(device->inRange(paddr))) {
            mmio = true;
            device->write(paddr, size, data);
#ifdef LOG_MEM
            Log::trace("mem", "mmio write 0x{:x} {} {:x}", paddr, size, *data);
#endif
            return;
        }
    }
    memcpy(ram + paddr - 0x80000000, data, size);
#ifdef LOG_MEM
    uint64_t val = *((uint64_t*)(ram + paddr - 0x80000000));
    Log::trace("mem", "write 0x{:x} {} {:x}", paddr, size, val);
#endif
}

void Memory::setDevices(std::vector<Device*> devices) {
    this->devices = devices;
}

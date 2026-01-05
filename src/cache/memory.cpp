#include "cache/memory.h"
#include <sys/mman.h>

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
        spdlog::error("could not mmap memory size {}", size);
        exit(1);
    }

#ifdef DRAMSIM
    for (int i = 0; i < dram_queue_size; i++) {
        DRAMMeta* meta = new DRAMMeta;
        dram_idle_queue.push(new CoDRAMRequest(0, 0, meta));
    }
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
            return;
        }
    }
    memcpy(data, ram + paddr - 0x80000000, size);
}

void Memory::paddrWrite(uint64_t paddr, uint32_t size, uint8_t* data, bool& mmio) {
    mmio = false;
    for (Device* device: devices) {
        if (unlikely(device->inRange(paddr))) {
            mmio = true;
            device->write(paddr, size, data);
            return;
        }
    }
    memcpy(ram + paddr - 0x80000000, data, size);
}

void Memory::setDevices(std::vector<Device*> devices) {
    this->devices = devices;
}

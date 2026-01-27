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
#ifdef RAMULATOR
    ramulator_frontend->finalize();
    ramulator_memory->finalize();
    
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
        ExitHandler::exit(1);
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
#ifdef RAMULATOR
    YAML::Node config = Ramulator::Config::parse_config_file(dram_config, {});
    ramulator_frontend = Ramulator::Factory::create_frontend(config);
    ramulator_memory = Ramulator::Factory::create_memory_system(config);
    ramulator_frontend->connect_memory_system(ramulator_memory);
    ramulator_memory->connect_frontend(ramulator_frontend);
    for (int i = 0; i < dram_queue_size; i++) {
        DRAMMeta* meta = new DRAMMeta;
        dram_read_queue.push(meta);
        DRAMMeta* meta_write = new DRAMMeta;
    }
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

bool Memory::lookup(int callback_id, CacheReq* req) {
    req->addr &= 0xffffffffff;
    if (req->req == READ_SHARED) {
        return memoryRead(callback_id, req->id, req->addr, req->size);
    } else {
        return memoryWrite(callback_id, req->id, req->addr, req->size);
    }
}

void Memory::tick() {
#ifdef DRAMSIM
    dram->tick();
    CoDRAMResponse* rresp = dram->check_read_response();
    if (rresp != NULL) {
        DRAMMeta* meta = (DRAMMeta*)rresp->req->meta;
        callbacks[meta->callback_id](meta->id, result);
        dram_idle_queue.push(const_cast<CoDRAMRequest*>(rresp->req));
    }
    CoDRAMResponse* wresp = dram->check_write_response();
    if (wresp != NULL) {
        DRAMMeta* meta = (DRAMMeta*)wresp->req->meta;
        callbacks[meta->callback_id](meta->id, result);
        dram_idle_queue.push(const_cast<CoDRAMRequest*>(wresp->req));
    }
#endif
#ifdef RAMULATOR
    ramulator_memory->tick();
    if (write_valid) {
        write_valid = false;
        callbacks[write_callback_id](write_ids, result);
    }
#endif
    for (auto device : devices) {
        device->tick();
        DeviceReq* resp = device->checkResponse();
        if (resp != nullptr) {
            callbacks[resp->callback_id](resp->id, result);
            device_idle_queue.push(resp);
        }
    }
}

bool Memory::memoryRead(int callback_id, uint16_t* id, uint64_t addr, uint32_t size) {
    for (Device* device: devices) {
        if (unlikely(device->inRange(addr))) {
            if (device_idle_queue.empty()) {
                return false;
            }
            DeviceReq* req = device_idle_queue.front();
            req->callback_id = callback_id;
            *(uint64_t*)req->id = *(uint64_t*)id;
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
    
    ((DRAMMeta*)req->meta)->callback_id = callback_id;
    *(uint64_t*)((DRAMMeta*)req->meta)->id = *(uint64_t*)id;
    ((DRAMMeta*)req->meta)->addr = addr;
    ((DRAMMeta*)req->meta)->size = size;
    req->address = addr;
    req->is_write = false;
    dram->add_request(req);
    return true;
#elif RAMULATOR
    Ramulator::Request req(addr, Ramulator::Request::Type::Read);
    req.source_id = 0;
    req.callback = [this](Ramulator::Request& req) {
        DRAMMeta* meta = this->dram_read_queue.pop();
        this->callbacks[meta->callback_id](meta->id, this->result);
    };
    bool success = ramulator_memory->send(req);
    if (success) {
        DRAMMeta* meta = dram_read_queue.next();
        meta->callback_id = callback_id;
        *(uint64_t*)meta->id = *(uint64_t*)id;
    }
    return success;
#else
    callbacks[callback_id](id, result);
    return true;
#endif
}

bool Memory::memoryWrite(int callback_id, uint16_t* id, uint64_t addr, uint32_t size) {
    for (Device* device: devices) {
        if (unlikely(device->inRange(addr))) {
            if (device_idle_queue.empty()) {
                return false;
            }
            DeviceReq* req = device_idle_queue.front();
            req->callback_id = callback_id;
            *(uint64_t*)req->id = *(uint64_t*)id;
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
    
    ((DRAMMeta*)req->meta)->callback_id = callback_id;
    *(uint64_t*)((DRAMMeta*)req->meta)->id = *(uint64_t*)id;
    ((DRAMMeta*)req->meta)->addr = addr;
    ((DRAMMeta*)req->meta)->size = size;
    req->address = addr;
    req->is_write = true;
    dram->add_request(req);
    return true;
#elif RAMULATOR
    Ramulator::Request req(addr, Ramulator::Request::Type::Write);
    req.source_id = 1;
    bool success = ramulator_memory->send(req);
    if (success) {
        write_ids = id;
        write_valid = true;
        write_callback_id = callback_id;
    }
    return success;
#else
    callbacks[callback_id](id, result);
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

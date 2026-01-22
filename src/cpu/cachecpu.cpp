#include "cpu/cachecpu.h"
#include "cache/cachemanager.h"

CacheCPU::~CacheCPU() {
    delete req;
    delete info;
}

void CacheCPU::afterLoad() {
    info = new DecodeInfo();
    pc = Base::arch->getStartPC();
    inst_count = 0;
    Base::arch->setInstret(&inst_count);
    icache = CacheManager::getInstance().getICache();
    icache->setCallback(std::bind(&CacheCPU::icacheCallback, this, std::placeholders::_1, std::placeholders::_2));
    req = new CacheReq();
    req->req = READ_SHARED;
    req->size = 4;
}

void CacheCPU::exec() {
    if (inst_valid) {
        Base::arch->decode(pc, paddr, info);
        pc = Base::arch->updateEnv();
        if (info->type == IFENCE) {
            arch->flushCache(0, 0, 0);
        }
        inst_valid = false; 
        inst_count++;
    }

    uint64_t exception;
    Base::arch->translateAddr(pc, FETCH_TYPE::IFETCH, paddr, exception);
    if (exception) {
        inst_valid = true;
        Base::arch->handleException(exception, pc, info);
    } else {
        req->addr = paddr;
        icache->lookup(0, req);
    }
    Base::upTick();
}

void CacheCPU::icacheCallback(uint16_t* id, CacheTagv* tag) {
    inst_valid = true;
}
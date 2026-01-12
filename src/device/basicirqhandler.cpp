#include "device/basicirqhandler.h"

void BasicIrqHandler::afterLoad() {
    end_addr = base_addr + range;
    ram = new uint8_t[range];
}

void BasicIrqHandler::tick() {

}

void BasicIrqHandler::read(uint64_t addr, int size, uint8_t* data) {
    if (addr < base_addr || addr + size > end_addr) {
        return;
    }
    memcpy(data, ram + addr - base_addr, size);
}

void BasicIrqHandler::write(uint64_t addr, int size, uint8_t* data) {
    if (addr < base_addr || addr + size > end_addr) {
        return;
    }
    memcpy(ram + addr - base_addr, data, size);
}

bool BasicIrqHandler::inRange(uint64_t addr) {
    return addr >= base_addr && addr < end_addr;
}

bool BasicIrqHandler::addRequest(DeviceReq* req) {
    if (this->req != nullptr) {
        return false;
    }
    this->req = req;
    return true;
}

DeviceReq* BasicIrqHandler::checkResponse() {
    if (this->req == nullptr) {
        return nullptr;
    }
    DeviceReq* req = this->req;
    this->req = nullptr;
    return req;
}

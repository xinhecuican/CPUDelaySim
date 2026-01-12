#include "device/simpleuart.h"

void SimpleUart::tick() {

}

bool SimpleUart::addRequest(DeviceReq* req) {
    this->req = req;
    req_valid = true;
    return true;
}

DeviceReq* SimpleUart::checkResponse() {
    if (req_valid) {
        req_valid = false;
        return req;
    }
    return nullptr;
}

bool SimpleUart::inRange(uint64_t addr) {
    return addr >= base_addr && addr < base_addr + 0x1000;
}

void SimpleUart::read(uint64_t addr, int size, uint8_t* data) {
    uart.do_read(addr - base_addr, size, (char*)data);
}

void SimpleUart::write(uint64_t addr, int size, uint8_t* data) {
    uart.do_write(addr - base_addr, size, (char*)data);
}
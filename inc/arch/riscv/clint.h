#ifndef DEVICE_CLINT_H
#define DEVICE_CLINT_H
#include "device/device.h"

class Clint : public Device {
public:
    void load() override;
    void afterLoad() override;
    void tick() override;
    bool addRequest(DeviceReq* req) override;
    DeviceReq* checkResponse() override;
    bool inRange(uint64_t addr) override;
    void read(uint64_t addr, int size, uint8_t* data) override;
    void write(uint64_t addr, int size, uint8_t* data) override;

private:
    uint64_t base_addr;
    
    uint64_t end_addr;
    uint64_t msip;
    uint64_t mtime;
    uint64_t mtimecmp;
    DeviceReq* req = nullptr;
    bool irqValid = false;
};

#endif
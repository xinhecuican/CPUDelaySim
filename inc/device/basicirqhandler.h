#ifndef DEVICE_BASICIRQHANDLER_H
#define DEVICE_BASICIRQHANDLER_H
#include "device/irqhandler.h"

class BasicIrqHandler : public IrqHandler {
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
    uint64_t range;

    uint64_t end_addr;
    uint8_t* ram;
    DeviceReq* req;
};

#endif
#ifndef DEVICE_H
#define DEVICE_H
#include "common/base.h"

struct DeviceReq {
    uint64_t addr;
    uint32_t size;
    int id;
    bool is_write;
};

class IrqHandler;

class Device : public Base {
public:
    virtual ~Device() = default;
    virtual void tick()=0;
    virtual bool addRequest(DeviceReq* req) = 0;
    virtual DeviceReq* checkResponse() = 0;
    virtual bool inRange(uint64_t addr) = 0;
    virtual void read(uint64_t addr, int size, uint8_t* data) = 0;
    virtual void write(uint64_t addr, int size, uint8_t* data) = 0;
    virtual void setIrqHandler(IrqHandler* irq_handler) {this->irq_handler = irq_handler;}
protected:
    IrqHandler* irq_handler = nullptr;
};

#endif
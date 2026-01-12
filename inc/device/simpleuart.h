#ifndef DEVICE_SIMPLE_UART_H
#define DEVICE_SIMPLE_UART_H
#include "device/device.h"
#include "device/uart8250.h"
class SimpleUart : public Device {
public:
    void load() override;
    void tick() override;
    bool addRequest(DeviceReq* req) override;
    DeviceReq* checkResponse() override;
    bool inRange(uint64_t addr) override;
    void read(uint64_t addr, int size, uint8_t* data) override;
    void write(uint64_t addr, int size, uint8_t* data) override;
private:
    uint64_t base_addr;

    uart8250 uart;
    DeviceReq* req;
    bool req_valid = false;
};

#endif
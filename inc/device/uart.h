#ifndef DEVICE_UART_H
#define DEVICE_UART_H
#include "device/device.h"
#include "device/serial.h"

class Uart : public Device {
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
    uint32_t baudrate;
    uint32_t irq_number;
    int delay;

    SerialState* serial;
    qemu_irq irq;
    DeviceReq* req;
    bool req_valid = false;
    bool resp_valid = false;
    int current_delay = 0;
    uint64_t end_addr;
};

#endif
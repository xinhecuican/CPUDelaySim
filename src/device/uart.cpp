#include "device/uart.h"
#include "device/irqhandler.h"

void cpu_irq_handler(void *opaque, int n, int level) {
    Uart* uart = (Uart*)opaque;
    uart->setIrq(level);
}

void Uart::afterLoad() {
    irq = qemu_allocate_irq(cpu_irq_handler, this, irq_number);
    serial = simple_serial_init(base_addr, irq, baudrate);
    end_addr = base_addr + 0x1000;
}

bool Uart::inRange(uint64_t addr) {
    return addr >= base_addr && addr < end_addr;
}

bool Uart::addRequest(DeviceReq* req) {
    if (req_valid || resp_valid) {
        return false;
    }
    this->req = req;
    this->req_valid = true;
    current_delay = delay;
    return true;
}

void Uart::tick() {
    if (req_valid) {
        if (current_delay == 0) {
            req_valid = false;
            resp_valid = true;
        } else {
            current_delay--;
        }
    }
}

DeviceReq* Uart::checkResponse() {
    if (!resp_valid) {
        return nullptr;
    }
    resp_valid = false;
    return req;
}

void Uart::read(uint64_t addr, int size, uint8_t* data) {
    uint8_t res = serial_ioport_read(serial, addr, size);
    *data = res;
}

void Uart::write(uint64_t addr, int size, uint8_t* data) {
    uint64_t di = *data;
    serial_ioport_write(serial, addr, di, size);
}

void Uart::setIrq(bool valid) {
    irq_handler->setIrqState(irq_number, valid);
}
#ifndef DEVICE_IRQHANDLER_H
#define DEVICE_IRQHANDLER_H
#include "device/device.h"
#include <functional>
#include <vector>

class IrqHandler : public Device {
public:
    typedef std::function<void(uint64_t)> irq_listener_t;
    virtual void addIrqListener(irq_listener_t listener) {
        irq_listeners.push_back(listener);
    }
    virtual void setIrqState(uint64_t irqnumber, bool state) {
        uint64_t irqshift = 1 << irqnumber;
        if (state) {
            irq |= irqshift;
        } else {
            irq &= ~irqshift;
        }
        for (auto listener : irq_listeners) {
            listener(irq);
        }
    }
protected:
    uint64_t irq = 0;
    std::vector<irq_listener_t> irq_listeners;
};




#endif
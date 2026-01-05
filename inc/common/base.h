#ifndef COMMON_BASE_H_
#define COMMON_BASE_H_

#include "common/common.h"
#include "arch/arch.h"

class Base {
public:
    virtual ~Base() = default;
    /**
     * @brief load configuration, generate by parse.py
     */
    virtual void load() {}
    /**
     * @brief do some post processing after load configuration
     * 
     */
    virtual void afterLoad() {}
    static uint64_t getTick() { return *tick; }
    static void upTick() { (*tick)++; }
    static void setTick(uint64_t* tick) { Base::tick = tick; }
    static void setArch(Arch* arch) { Base::arch = arch; }

protected:
    static Arch* arch;

private:
    static uint64_t* tick;
};

#endif // COMMON_BASE_H_

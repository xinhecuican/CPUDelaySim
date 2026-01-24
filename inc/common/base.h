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
    virtual void finalize() {}
    static inline uint64_t getTick() { return *tick; }
    static inline void upTick() { (*tick)++; }
    static inline void setTick(uint64_t* tick) { Base::tick = tick; }
    static inline void setArch(Arch* arch) { Base::arch = arch; }
    static inline Arch* getArch() { return arch; }

protected:
    static Arch* arch;

private:
    static uint64_t* tick;
};

#endif // COMMON_BASE_H_

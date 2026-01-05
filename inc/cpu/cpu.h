#ifndef CPU_H
#define CPU_H
#include "common/base.h"
#include "arch/arch.h"

class CPU : public Base {
public:
    virtual ~CPU() = default;
    virtual void load();
    virtual void exec() = 0;

protected:
    int fetch_width;
    int retire_size;
};

#endif
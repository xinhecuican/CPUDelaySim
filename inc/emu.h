#ifndef EMU_H
#define EMU_H

#include "cpu/cpu.h"
#include "config.h"

class EMU {
public:
    EMU();
    ~EMU();
    void init(const std::string& config);
    void run();
    void stop();
    void finalize();

protected:
    CPU* cpu;
    Arch* arch;
    Config config;
    uint64_t tick = 0;
    bool stopped = false;
};

#endif
#ifndef LLC_H
#define LLC_H
#include "cache.h"
#ifdef DRAMSIM
#include "cosimulation.h"
#endif
#include "device/device.h"

class Memory : public Cache {
public:
    Memory(const std::string& filename);
    ~Memory();
    void load() override;
    void afterLoad() override;
    void tick();
    bool lookup(int id, snoop_req_t req, uint64_t addr, uint32_t size) override;
    bool memoryRead(int id, uint64_t addr, uint32_t size);
    bool memoryWrite(int id, uint64_t addr, uint32_t size);
    void paddrRead(uint64_t paddr, uint32_t size, uint8_t* data, bool& mmio);
    void paddrWrite(uint64_t paddr, uint32_t size, uint8_t* data, bool& mmio);
    void setDevices(std::vector<Device*> devices);

private:
    /**
     * @ingroup config
     *
     * @brief 内存大小
     */
    uint64_t size;

    std::string filename;
    
    uint8_t* ram;
    uint64_t filesize;

    CacheTagv* result;
    /**
     * @ingroup config
     *
     * @brief DRAMSim3 配置文件路径
     */
    std::string dram_config;
    /**
     * @ingroup config
     *
     * @brief DRAMSim3 输出文件路径
     */ 
    std::string dram_outpath;
    /**
     * @ingroup config
     *
     * @brief DRAM 队列大小
     */
    int dram_queue_size;
    ComplexCoDRAMsim3* dram;
    std::queue<CoDRAMRequest*> dram_idle_queue;

    std::vector<Device*> devices;
    std::queue<DeviceReq*> device_idle_queue;
};

#endif

#ifndef LLC_H
#define LLC_H
#include "cache/cache.h"
#ifdef DRAMSIM
#include "cosimulation.h"
#endif
#ifdef RAMULATOR
#include "base/base.h"
#include "base/request.h"
#include "base/config.h"
#include "frontend/frontend.h"
#include "memory_system/memory_system.h"
#endif
#include "device/device.h"
#include "common/linklist.h"

class Memory : public Cache {
public:
    Memory(const std::string &filename);
    ~Memory();
    void load() override;
    void afterLoad() override;
    void tick() override;
    bool lookup(int callback_id, CacheReq* req) override;
    bool memoryRead(int callback_id, uint16_t* id, uint64_t addr, uint32_t size);
    bool memoryWrite(int callback_id, uint16_t* id, uint64_t addr, uint32_t size);
    void paddrRead(uint64_t paddr, uint32_t size, uint8_t *data, bool &mmio);
    void paddrWrite(uint64_t paddr, uint32_t size, uint8_t *data, bool &mmio);
    void setDevices(std::vector<Device *> devices);



private:
    /**
     * @ingroup config
     *
     * @brief 内存大小
     */
    uint64_t size;

    std::string filename;

    uint8_t *ram;
    uint64_t filesize;

    CacheTagv *result;
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

    std::vector<Device *> devices;
    std::queue<DeviceReq *> device_idle_queue;
#ifdef DRAMSIM
    ComplexCoDRAMsim3 *dram;
    std::queue<CoDRAMRequest *> dram_idle_queue;
#endif
#ifdef RAMULATOR
    Ramulator::IFrontEnd* ramulator_frontend;
    Ramulator::IMemorySystem* ramulator_memory;
    LinkList<DRAMMeta> dram_read_queue;
    uint16_t* write_ids;
    int write_callback_id;
    bool write_valid = false;
#endif
};

#endif

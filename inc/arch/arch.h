#ifndef ARCH_ARCH_H
#define ARCH_ARCH_H
#include "common/common.h"

enum FETCH_TYPE {
    IFETCH = 1,
    LFETCH = 2,
    SFETCH = 4,
};

class Arch {
public:
    virtual ~Arch() = default;
    virtual bool getStream(uint64_t pc, uint8_t pred, bool btbv, BTBEntry* btb_entry, FetchStream* stream) = 0;
    /**
     * @brief translate virtual address to physical address
     * 
     * @param vaddr virtual address
     * @param ifetch ifetch or load
     * @param paddr physical address
     * @param exception exception flag
     */
    virtual void translateAddr(uint64_t vaddr, FETCH_TYPE type, uint64_t& paddr, bool& exception) = 0;
    /**
     * @brief read from physical address and decode instruction
     * 
     * @param paddr physical address
     * @param info decode info
     * @return inst size in byte
     */
    virtual int decode(uint64_t paddr, DecodeInfo* info)=0;
    /**
     * @brief read from physical address
     * 
     * @param paddr physical address
     * @param size read size in byte
     * @param type read type
     * @param data read data
     * @return false paddr read exception
     */
    virtual bool paddrRead(uint64_t paddr, int size, FETCH_TYPE type, uint8_t* data)=0;
    /**
     * @brief write to physical address
     * 
     * @param paddr physical address
     * @param size write size in byte
     * @param type write type
     * @param data write data
     * @return false paddr write exception
     */
    virtual bool paddrWrite(uint64_t paddr, int size, FETCH_TYPE type, uint8_t* data)=0;
    virtual void afterLoad(){}
    /**
     * @brief update environment after @ref decode
     * 
     * @return uint64_t target pc
     */
    virtual uint64_t updateEnv(){return 0;}
    void setTick(uint64_t* tick) { this->tick = tick; }
    uint64_t getTick() { return *tick; }
    void setInstret(uint64_t* instret) { this->instret = instret; }
    uint64_t getInstret() { return *instret; }

    virtual uint64_t getStartPC() = 0;
protected:
    uint64_t* tick;
    uint64_t* instret;
};

#endif
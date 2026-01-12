#ifndef ARCH_ARCH_H
#define ARCH_ARCH_H
#include "common/common.h"
#include <algorithm>
#include <cstdint>
#include <fstream>
#include <map>
#include <sstream>
#include <string>

enum FETCH_TYPE {
    IFETCH = 1,
    LFETCH = 2,
    SFETCH = 4,
};

class Arch {
public:
    virtual ~Arch() {}
    virtual bool getStream(uint64_t pc, uint8_t pred, bool btbv, BTBEntry* btb_entry, FetchStream* stream) = 0;
    /**
     * @brief translate virtual address to physical address
     * 
     * @param vaddr virtual address
     * @param ifetch ifetch or load
     * @param paddr physical address
     * @param exception exception flag
     */
    virtual void translateAddr(uint64_t vaddr, FETCH_TYPE type, uint64_t& paddr, uint64_t& exception) = 0;
    /**
     * @brief read from physical address and decode instruction
     * 
     * @param vaddr virtual address
     * @param paddr physical address
     * @param info decode info
     * @return inst size in byte
     */
    virtual int decode(uint64_t vaddr, uint64_t paddr, DecodeInfo* info)=0;

    /**
     * @brief handle IPF
     * 
     * @param exception exception code
     * @param paddr physical address causing exception
     * @param info decode info
     */
    virtual void handleException(uint64_t exception, uint64_t paddr, DecodeInfo* info) {}

    /**
     * @brief interrupt request listener
     */
    virtual void irqListener(uint64_t irq) = 0;

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

    virtual void printState() {}


    virtual void initConfig(const std::string& config_path) {
        std::ifstream file(config_path);
        if (!file.is_open()) {
            spdlog::error("Arch::initConfig: failed to open config file {}", config_path);
            return;
        }

        std::string line;
        while (std::getline(file, line)) {
            // Trim line
            line.erase(line.begin(), std::find_if(line.begin(), line.end(),
                                                [](unsigned char ch) { return !std::isspace(ch); }));
            line.erase(std::find_if(line.rbegin(), line.rend(),
                                    [](unsigned char ch) { return !std::isspace(ch); })
                        .base(),
                    line.end());

            // Find '='
            size_t eq_pos = line.find('=');
            if (eq_pos == std::string::npos)
                continue;

            std::string left = line.substr(0, eq_pos);
            std::string right = line.substr(eq_pos + 1);

            // Trim left and right
            left.erase(left.begin(), std::find_if(left.begin(), left.end(),
                                                [](unsigned char ch) { return !std::isspace(ch); }));
            left.erase(std::find_if(left.rbegin(), left.rend(),
                                    [](unsigned char ch) { return !std::isspace(ch); })
                        .base(),
                    left.end());
            right.erase(right.begin(), std::find_if(right.begin(), right.end(), [](unsigned char ch) {
                            return !std::isspace(ch);
                        }));
            right.erase(std::find_if(right.rbegin(), right.rend(),
                                    [](unsigned char ch) { return !std::isspace(ch); })
                            .base(),
                        right.end());

            // Parse left
            std::string name;
            int offset_index = 0;
            size_t bracket_start = left.find('[');
            size_t bracket_end = left.find(']');
            if (bracket_start != std::string::npos && bracket_end != std::string::npos &&
                bracket_end > bracket_start) {
                name = left.substr(0, bracket_start);
                std::string offset_str =
                    left.substr(bracket_start + 1, bracket_end - bracket_start - 1);
                offset_index = std::stoi(offset_str);
            } else {
                name = left;
                offset_index = 0;
            }

            size_t actual_offset = static_cast<size_t>(offset_index) * 8;

            uint64_t value = std::stoull(right, nullptr, 0); // Support hex, etc.

            archConfigs.push_back({static_cast<uint32_t>(actual_offset), left, value});
        }
    }

protected:
    uint64_t* tick;
    uint64_t* instret;
    struct ArchConfig {
        uint32_t offset;
        std::string name;
        uint64_t value;
    };
    std::vector<ArchConfig> archConfigs;
};

#endif
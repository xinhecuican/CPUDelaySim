#ifndef COMMON_STATS_H
#define COMMON_STATS_H
#include <any>
#include <vector>
#include <string>

class Stats {
public:
    static void registerStat(std::any value, const std::string& name, const std::string& description);
    static void registerRatio(std::any divend, std::any divisor, const std::string& name, const std::string& description);
    static std::any getStat(const std::string& name);
    static void writeback();

private:
    static Stats& instance() {
        static Stats stats;
        return stats;
    }
    struct stat_t {
        std::any value;
        std::string name;
        std::string description;
    };
    struct ratio_t {
        std::any divend;
        std::any divisor;
        std::string name;
        std::string description;
    };
    std::vector<stat_t> stats;
    std::vector<ratio_t> ratios;
};

#endif
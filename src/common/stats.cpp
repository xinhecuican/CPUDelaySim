#include "common/stats.h"
#include "common/log.h"

void Stats::registerStat(std::any value, const std::string& name, const std::string& description) {
    instance().stats.push_back({value, name, description});
}

void Stats::registerRatio(std::any divend, std::any divisor, const std::string& name, const std::string& description) {
    instance().ratios.push_back({divend, divisor, name, description});
}

void Stats::writeback() {
    // 清空stat.log文件并重新创建日志器
    auto stat_logger = spdlog::get("stat");
    if (stat_logger) {
        // 获取sink并尝试转换为basic_file_sink_mt
        auto& sinks = stat_logger->sinks();
        if (!sinks.empty()) {
            auto file_sink = std::dynamic_pointer_cast<spdlog::sinks::basic_file_sink_mt>(sinks[0]);
            if (file_sink) {
                auto log_path = file_sink->filename();
                spdlog::drop("stat");
                stat_logger = spdlog::basic_logger_mt<spdlog::async_factory>("stat", log_path, true);
                stat_logger->set_pattern("%v");
                stat_logger->set_level(spdlog::level::trace);
            }
        }
    }
    
    for (auto& stat : instance().stats) {
        if (stat.value.type() == typeid(int*)) {
            Log::stat("{:<20} {:>10} # {}", stat.name, *std::any_cast<int*>(stat.value), stat.description);
        }
        if (stat.value.type() == typeid(uint64_t*)) {
            Log::stat("{:<20} {:>10} # {}", stat.name, *std::any_cast<uint64_t*>(stat.value), stat.description);
        } else if (stat.value.type() == typeid(double*)) {
            Log::stat("{:<20} {:>10} # {}", stat.name, *std::any_cast<double*>(stat.value), stat.description);
        }
    }
    for (auto& ratio : instance().ratios) {
        double result = 0.0;
        bool valid = true;
        
        if (ratio.divend.type() == typeid(int*) && ratio.divisor.type() == typeid(int*)) {
            int divend = *std::any_cast<int*>(ratio.divend);
            int divisor = *std::any_cast<int*>(ratio.divisor);
            if (divisor != 0) {
                result = static_cast<double>(divend) / static_cast<double>(divisor);
            } else {
                valid = false;
            }
        } else if (ratio.divend.type() == typeid(int*) && ratio.divisor.type() == typeid(uint64_t*)) {
            int divend = *std::any_cast<int*>(ratio.divend);
            uint64_t divisor = *std::any_cast<uint64_t*>(ratio.divisor);
            if (divisor != 0) {
                result = static_cast<double>(divend) / static_cast<double>(divisor);
            } else {
                valid = false;
            }
        } else if (ratio.divend.type() == typeid(uint64_t*) && ratio.divisor.type() == typeid(int*)) {
            uint64_t divend = *std::any_cast<uint64_t*>(ratio.divend);
            int divisor = *std::any_cast<int*>(ratio.divisor);
            if (divisor != 0) {
                result = static_cast<double>(divend) / static_cast<double>(divisor);
            } else {
                valid = false;
            }
        } else if (ratio.divend.type() == typeid(uint64_t*) && ratio.divisor.type() == typeid(uint64_t*)) {
            uint64_t divend = *std::any_cast<uint64_t*>(ratio.divend);
            uint64_t divisor = *std::any_cast<uint64_t*>(ratio.divisor);
            if (divisor != 0) {
                result = static_cast<double>(divend) / static_cast<double>(divisor);
            } else {
                valid = false;
            }
        } else {
            valid = false;
        }
        
        if (valid) {
            Log::stat("{:<20} {:>10.5f} # {}", ratio.name, result, ratio.description);
        } else {
            Log::stat("{:<20} {:>10} # {} (invalid division)", ratio.name, "NaN", ratio.description);
        }
    }
}

std::any Stats::getStat(const std::string& name) {
    for (auto& stat : instance().stats) {
        if (stat.name == name) {
            return stat.value;
        }
    }
    return std::any();
}

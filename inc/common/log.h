#ifndef COMMON_LOG_H
#define COMMON_LOG_H

#include <spdlog/spdlog.h>
#include <spdlog/async.h>
#include <spdlog/sinks/basic_file_sink.h>
#include "common/base.h"
#include <iostream>

class Log {
public:
    static void init(const std::string& name, const std::string& log_file) {
        try {
        auto logger = spdlog::basic_logger_mt<spdlog::async_factory>(name, log_file);
        logger->set_pattern("%v");
        logger->set_level(spdlog::level::trace);
        } catch (const spdlog::spdlog_ex& ex) {
            std::cout << "Log initialization failed: " << ex.what() << std::endl;
        }
    }
    template <typename... Args>
    static inline void trace(const std::string& name, spdlog::format_string_t<Args...> fmt, Args &&...args) {
        spdlog::get(name)->trace("[{0}] {1}", Base::getTick(), fmt::format(fmt, std::forward<Args>(args)...));
        spdlog::get(name)->flush();
    }
};


#endif
#ifndef COMMON_LOG_H
#define COMMON_LOG_H

#include <spdlog/spdlog.h>
#include <spdlog/async.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
#include "common/base.h"
#include <iostream>
#include <filesystem>
#include "arch/arch.h"
namespace fs = std::filesystem;

class Log {
public:
    static uint64_t start_tick;
    static uint64_t end_tick;
    static void initStdio(bool serial_stdio, bool log_stdio, const std::string& log_path) {
        spdlog::init_thread_pool(65536, 2);
        std::shared_ptr<spdlog::logger> std_logger;
        if (log_stdio) {
            std_logger = spdlog::stdout_color_mt("stdio");
        } else {
            std_logger = spdlog::basic_logger_mt<spdlog::async_factory>("stdio", fs::path(log_path) / "stdio.log");
        }
        std_logger->set_pattern("%^[%l]%$ %v");
        std_logger->set_level(spdlog::level::trace);

        std::shared_ptr<spdlog::logger> serial_logger = std_logger;
        if (serial_stdio) {
            serial_logger = spdlog::stdout_color_mt("serial");
        } else {
            serial_logger = spdlog::basic_logger_mt<spdlog::async_factory>("serial", fs::path(log_path) / "serial.log");
        }
        serial_logger->set_pattern("%v");
        serial_logger->set_level(spdlog::level::trace);

        std::shared_ptr<spdlog::logger> stat_logger = spdlog::basic_logger_mt<spdlog::async_factory>("stat", fs::path(log_path) / "stat.log", true);
        stat_logger->set_pattern("%v");
        stat_logger->set_level(spdlog::level::trace);

        spdlog::set_default_logger(std_logger);

    }
    static void init(const std::string& name, const std::string& log_file, bool is_stdout = false) {
        try {
            if (is_stdout) {
                auto logger = spdlog::stdout_color_mt(name);
                logger->set_pattern("%v");
                logger->set_level(spdlog::level::trace);
            } else {
                auto logger = spdlog::basic_logger_mt<spdlog::async_factory>(name, log_file);
                logger->set_pattern("%v");
                logger->set_level(spdlog::level::trace);
            }
        } catch (const spdlog::spdlog_ex& ex) {
            std::cout << "Log initialization failed: " << ex.what() << std::endl;
        }
    }
    template <typename... Args>
    static inline void trace(const std::string& name, spdlog::format_string_t<Args...> fmt, Args &&...args) {
        if (Base::getTick() >= start_tick && Base::getTick() <= end_tick)
            spdlog::get(name)->trace("[{0}] {1}", Base::getTick(), fmt::format(fmt, std::forward<Args>(args)...));
    }

    template <typename... Args>
    static inline void info(spdlog::format_string_t<Args...> fmt, Args &&...args) {
        spdlog::get("stdio")->info("[{0}] {1}", Base::getTick(), fmt::format(fmt, std::forward<Args>(args)...));
    }

    template <typename... Args>
    static inline void warn(spdlog::format_string_t<Args...> fmt, Args &&...args) {
        spdlog::get("stdio")->warn("[{0}] {1}", Base::getTick(), fmt::format(fmt, std::forward<Args>(args)...));
    }

    template <typename... Args>
    static inline void error(spdlog::format_string_t<Args...> fmt, Args &&...args) {
        Base::getArch()->printState();
        spdlog::get("stdio")->error("[{0}] {1}", Base::getTick(), fmt::format(fmt, std::forward<Args>(args)...));
    }

    static inline void serial(char c) {
        spdlog::get("serial")->trace("{}", c);
    }

    template <typename... Args>
    static inline void stat(spdlog::format_string_t<Args...> fmt, Args &&...args) {
        spdlog::get("stat")->info(fmt, std::forward<Args>(args)...);
    }

    static inline void stat(const std::string& name, int value) {
        spdlog::get("stat")->info("{}: {}", name, value);
    }

    static inline void stat(const std::string& name, uint64_t value) {
        spdlog::get("stat")->info("{}: {}", name, value);
    }

    static inline void stat(const std::string& name, double value) {
        spdlog::get("stat")->info("{}: {}", name, value);
    }

};


#endif
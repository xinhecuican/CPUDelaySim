#ifndef CONFIG_H
#define CONFIG_H

#include <string>
#include <fstream>
#include <iostream>
#include <map>
#include <algorithm>
#include <cctype>
#include <unistd.h>
#include <cstdlib>
#include <filesystem>
#include <chrono>
#include "common/exithandler.h"
namespace fs = std::filesystem;

// Custom trim function to remove whitespace from both ends of a string
inline std::string trim(const std::string &s, const char *whitespace = " \t\n\r\f\v") {
    const size_t start = s.find_first_not_of(whitespace);
    if (start == std::string::npos) {
        return "";
    }
    const size_t end = s.find_last_not_of(whitespace);
    return s.substr(start, end - start + 1);
}

class Config {
public:
    std::string memory_path;
    std::string log_path;
    std::string arch_path;
    uint64_t end_tick = -1;
    uint64_t inst_count = -1;
    bool serial_stdio = true;
    bool log_stdio = true;
    uint64_t log_start_tick = 0;
    uint64_t log_end_tick = -1;

    static std::string _log_path;

    void setup(const std::string& config_path) {
        std::fstream config_file(config_path, std::ios::in);
        std::map<std::string, std::string> config_map;
        if (!config_file.is_open()) {
            std::cerr << "Error: Failed to open config file " << config_path << std::endl;
            ExitHandler::exit(1);
        }

        std::string line;
        while (std::getline(config_file, line)) {
            auto pos = line.find("=");
            if (pos != std::string::npos) {
                    const char *w = " \t\n\r\f\v";
                    std::string key = trim(line.substr(0, pos), w);
                    std::string value = trim(line.substr(pos + 1), w);
                    config_map[key] = value;
            }
        }

        if (config_map.find("memory_path") != config_map.end()) {
            memory_path = config_map["memory_path"];
        }
        if (config_map.find("end_tick") != config_map.end()) {
            end_tick = std::stoull(config_map["end_tick"]);
        }
        if (config_map.find("inst_count") != config_map.end()) {
            inst_count = std::stoull(config_map["inst_count"]);
        }
        if (config_map.find("log_path") != config_map.end()) {
            log_path = config_map["log_path"];
            std::string log_parent = log_path;
            if (!fs::exists(fs::path(log_path))) {
                fs::create_directories(fs::path(log_path));
            }
            log_path = (fs::path(log_path) / fs::path(std::to_string(std::chrono::system_clock::now().time_since_epoch().count()))).string();
            if (!fs::exists(fs::path(log_path))) {
                fs::create_directories(fs::path(log_path));
            }

            // Create a symbolic link from log_parent to log_path
            std::string latest_link = log_parent + "/latest";
            fs::remove_all(latest_link);
            fs::create_symlink(fs::canonical(fs::path(log_path)), latest_link);

            _log_path = log_path;
        }
        if (config_map.find("arch_path") != config_map.end()) {
            arch_path = config_map["arch_path"];
        }
        if (config_map.find("serial_stdio") != config_map.end()) {
            serial_stdio = std::stoi(config_map["serial_stdio"]) != 0;
        }
        if (config_map.find("log_stdio") != config_map.end()) {
            log_stdio = std::stoi(config_map["log_stdio"]) != 0;
        }
        if (config_map.find("log_start_tick") != config_map.end()) {
            log_start_tick = std::stoull(config_map["log_start_tick"]);
        }
        if (config_map.find("log_end_tick") != config_map.end()) {
            log_end_tick = std::stoull(config_map["log_end_tick"]);
        }
    }

    static std::string getLogFilePath(const std::string& filename) {
        return (fs::path(_log_path) / fs::path(filename)).string();
    }
};

#endif
#include <iostream>
#include "common/common.h"
#include "emu.h"
#include <csignal>
#include "common/dbhandler.h"

/**
 * @defgroup config 配置变量
 * @brief 可以修改配置的成员变量
 */
void clearGlobal() {
    Stats::writeback();
    DBHandler::closeDB();
    spdlog::shutdown();
}

void signal_handler(int signal) {
    clearGlobal();
    // 恢复默认处理并重新触发信号
    std::signal(signal, SIG_DFL);
    std::raise(signal);
}

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cout << "Usage: " << argv[0] << " <config_file>" << std::endl;
        return 1;
    }

    std::signal(SIGSEGV, signal_handler);  // 段错误
    std::signal(SIGABRT, signal_handler);  // 异常终止
    std::signal(SIGFPE, signal_handler);   // 算术错误
    std::signal(SIGILL, signal_handler);   // 非法指令
    std::signal(SIGINT, signal_handler);    // Ctrl+C
    ExitHandler::registerExitHandler([](int code) {
        clearGlobal();
    });

    std::string config_file = argv[1];
    EMU emu;
    emu.init(config_file);
    emu.run();
    clearGlobal();
    return 0;
}

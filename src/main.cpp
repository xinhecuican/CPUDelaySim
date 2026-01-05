#include <iostream>
#include "common/common.h"
#include "emu.h"

/**
 * @defgroup config 配置变量
 * @brief 可以修改配置的成员变量
 */

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cout << "Usage: " << argv[0] << " <config_file>" << std::endl;
        return 1;
    }
    std::string config_file = argv[1];
    EMU emu;
    emu.init(config_file);
    emu.run();
    return 0;
}

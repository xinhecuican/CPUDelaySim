#ifndef COMMON_EXITHANDLER_H_
#define COMMON_EXITHANDLER_H_
#include <functional>
#include <vector>

class ExitHandler {
public:
    static void exit(int code);
    static void registerExitHandler(std::function<void(int)> handler);

private:
    static std::vector<std::function<void(int)>> handlers;
};
#endif
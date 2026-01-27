#include "common/exithandler.h"

std::vector<std::function<void(int)>> ExitHandler::handlers;

void ExitHandler::exit(int code) {
    for (auto& handler : handlers) {
        handler(code);
    }
    exit(code);
}

void ExitHandler::registerExitHandler(std::function<void(int)> handler) {
    handlers.push_back(handler);
}

#include <csignal>
#include <iostream>

#include "ClientManager.h"
#include "Logger.h"
#include "config.h"

auto fileLogger = AsyncLogger(std::make_unique<FileLogger>(MAIN_LOG_FILE));

ClientManager clientManager;

void signalHandler(int signal) {
    fileLogger.log("Received signal " + std::to_string(signal) + ", shutting down...", LogLevel::INFO);
    clientManager.stop();
    exit(signal);
}

int main() {
    fileLogger.log("Starting server...", LogLevel::INFO);

    // Register signal handlers
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
#ifndef _WIN32
    signal(SIGQUIT, signalHandler);
#endif

    constexpr int PORT = 8080;
    clientManager.start(PORT);

    while (true) {
        // anything
        ClientManager::sleep(10000);
    }

    return 0;
}
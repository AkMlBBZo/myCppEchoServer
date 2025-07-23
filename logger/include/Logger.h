#ifndef LOGGER
#define LOGGER

#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

#include "GoChan.h"

enum class LogLevel { ERR, WARNING, MESSAGE, INFO, UNKNOWN, CUSTOM };

class Logger {
protected:
    std::string getMessage(const std::string& msg, LogLevel level, const std::string& prefix) {
        switch (level) {
        case LogLevel::ERR:
            return "[ERROR] " + msg;
        case LogLevel::WARNING:
            return "[WARNING] " + msg;
        case LogLevel::MESSAGE:
            return "[MESSAGE] " + msg;
        case LogLevel::INFO:
            return "[INFO] " + msg;
        case LogLevel::CUSTOM:
            return prefix + " " + msg;
        default:
            return "[UNKNOWN] " + msg;
        }
    }

public:
    virtual ~Logger()                                                              = default;
    virtual int log(const std::string&, LogLevel level, const std::string& prefix) = 0;
};

class FileLogger: public Logger {
private:
    std::string filename;

public:
    explicit FileLogger(const std::string& filename) : filename(filename) {}
    int log(const std::string& msg, LogLevel level = LogLevel::MESSAGE, const std::string& prefix = "") final {
        std::ofstream out(filename, std::ios::app);
        if (!out.is_open())
            return -1;

        out << getMessage(msg, level, prefix) << std::endl;
        return 0;
    }
};

class ConsoleLogger: public Logger {
public:
    int log(const std::string& msg, LogLevel level = LogLevel::MESSAGE, const std::string& prefix = "") final {
        std::cout << getMessage(msg, level, prefix) << msg << std::endl;
        return 0;
    }
};

class AsyncLogger: public Logger {
private:
    struct LogData {
        std::string msg;
        LogLevel level;
        std::string pref;
    };
    std::unique_ptr<Logger> wrappedLogger;
    std::unique_ptr<GoChan<LogData>> logChan;
    std::thread workThread;
    bool running = true;

    void worker() {
        while (running) {
            auto msg = logChan->receive();
            if (!msg)
                break;
            wrappedLogger->log(msg->msg, msg->level, msg->pref);
        }
        while (auto msg = logChan->receive()) {
            wrappedLogger->log(msg->msg, msg->level, msg->pref);
        }
    }

public:
    explicit AsyncLogger(std::unique_ptr<Logger> wrappedLogger)
        : wrappedLogger(std::move(wrappedLogger)), logChan(std::make_unique<GoChan<LogData>>(10)) {
        workThread = std::thread(&AsyncLogger::worker, this);
    }

    int log(const std::string& msg, LogLevel level = LogLevel::MESSAGE, const std::string& prefix = "") final {
        if (!logChan->send({msg, level, prefix}))
            return wrappedLogger->log(msg, level, prefix);
        return 0;
    }

    ~AsyncLogger() {
        running = false;
        logChan->close();
        if (workThread.joinable())
            workThread.join();
    }
};

#endif  // LOGGER
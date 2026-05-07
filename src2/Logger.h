// src2/Logger.h
#ifndef LOGGER_H
#define LOGGER_H

#include <string>
#include <iostream>
#include <fstream>
#include <mutex>
// #include <format>
#include <fmt/ranges.h>
#include <vector>
#include <fmt/core.h>
#include <fmt/format.h>
// #include "ScopedTimer.h"
enum class LogLevel {
    INFO,
    WARNING,
    ERROR,
    DEBUG,
    STATS
};

class Logger {
public:
    Logger();
    ~Logger();

    // Redirect logs to a file (optional)
    void setOutputFile(const std::string& filename);

    void closeOutputFile();

    // Core logging function
    void log(LogLevel level, const std::string& message);
    void log(LogLevel level, const std::string& prefix, const std::vector<int>& vec);

private:
    std::ofstream logFile;
    std::mutex logMutex; // Ensures thread safety if you use threads later
    std::string levelToString(LogLevel level);
};

// Declare the global logger instance
extern Logger globalLogger;
extern Logger statisticsLogger;

#endif // LOGGER_H

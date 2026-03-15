// src2/Logger.h
#ifndef LOGGER_H
#define LOGGER_H

#include <string>
#include <iostream>
#include <fstream>
#include <mutex>
#include <format>
#include <fmt/ranges.h>
#include <vector>

enum class LogLevel {
    INFO,
    WARNING,
    ERROR,
    DEBUG
};

class Logger {
public:
    Logger();
    ~Logger();

    // Redirect logs to a file (optional)
    void setOutputFile(const std::string& filename);

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

#endif // LOGGER_H

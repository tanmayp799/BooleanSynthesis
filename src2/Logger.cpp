#include "Logger.h"

// Global logger instance definition
Logger globalLogger;

Logger::Logger() {
}

Logger::~Logger() {
    if (logFile.is_open()) {
        logFile.close();
    }
}

void Logger::setOutputFile(const std::string& filename) {
    std::lock_guard<std::mutex> lock(logMutex);
    if (logFile.is_open()) {
        logFile.close();
    }
    logFile.open(filename, std::ios::out | std::ios::app);
}

void Logger::log(LogLevel level, const std::string& message) {
    std::lock_guard<std::mutex> lock(logMutex);
    
    std::string levelStr = levelToString(level);
    
    // Output to console
    if (level == LogLevel::ERROR) {
        std::cerr << "[" << levelStr << "] " << message << std::endl;
    } else {
        std::cout << "[" << levelStr << "] " << message << std::endl;
    }

    // Output to file
    if (logFile.is_open()) {
        logFile << "[" << levelStr << "] " << message << std::endl;
    }
}

std::string Logger::levelToString(LogLevel level) {
    switch (level) {
        case LogLevel::INFO: return "INFO";
        case LogLevel::WARNING: return "WARNING";
        case LogLevel::ERROR: return "ERROR";
        case LogLevel::DEBUG: return "DEBUG";
        default: return "UNKNOWN";
    }
}
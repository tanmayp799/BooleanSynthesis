#ifndef SCOPED_TIMER_H
#define SCOPED_TIMER_H

#include <chrono>
#include <string>
#include "Logger.h"

extern std::string g_argv2;

class ScopedTimer {
public:
    ScopedTimer(std::string name, int id, LogLevel level = LogLevel::INFO)
        : m_name(std::move(name)), m_level(level), m_start(std::chrono::steady_clock::now()), m_id(id) {}

    ~ScopedTimer() {
        auto end = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - m_start).count();
        if(m_level==LogLevel::STATS){
            statisticsLogger.log(m_level, fmt::format("{},{},{}", g_argv2, m_id, duration));
        } else {
            globalLogger.log(m_level, fmt::format("{} took {} ms", m_name, duration));
        }
    }

private:
    std::string m_name;
    LogLevel m_level;
    std::chrono::steady_clock::time_point m_start;
    int m_id;
};

// Helper macros to generate unique variable names for the timer instance
#define SCOPED_TIMER_CONCAT_IMPL(x, y) x##y
#define SCOPED_TIMER_CONCAT(x, y) SCOPED_TIMER_CONCAT_IMPL(x, y)
#define MEASURE_TIME(...) ScopedTimer SCOPED_TIMER_CONCAT(timer_, __LINE__)(__VA_ARGS__)

#endif // SCOPED_TIMER_H
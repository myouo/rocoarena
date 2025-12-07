#pragma once

#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <utility>

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_RESET   "\x1b[0m"

// 线程安全的轻量日志工具，便于在其他模块统一输出格式和颜色
class Logger {
public:
    enum class Level {
        Debug = 0,
        Info,
        Warn,
        Error
    };

    static void setLevel(Level level) { minLevel_ = level; }
    static Level level() { return minLevel_; }

    static void enableColor(bool enable) { useColor_ = enable; }
    static void showTimestamp(bool enable) { showTimestamp_ = enable; }

    template <typename... Args>
    static void debug(const std::string& module, Args&&... args) {
        log(Level::Debug, module, std::forward<Args>(args)...);
    }

    template <typename... Args>
    static void info(const std::string& module, Args&&... args) {
        log(Level::Info, module, std::forward<Args>(args)...);
    }

    template <typename... Args>
    static void warn(const std::string& module, Args&&... args) {
        log(Level::Warn, module, std::forward<Args>(args)...);
    }

    template <typename... Args>
    static void error(const std::string& module, Args&&... args) {
        log(Level::Error, module, std::forward<Args>(args)...);
    }

    template <typename... Args>
    static void log(Level level, const std::string& module, Args&&... args) {
        if(level < minLevel_) return;

        std::ostringstream oss;
        (oss << ... << std::forward<Args>(args));
        write(level, module, oss.str());
    }

private:
    static const char* levelName(Level level);
    static const char* levelColor(Level level);
    static void write(Level level, const std::string& module, const std::string& msg);
    static std::string nowString();

    inline static Level minLevel_ = Level::Info;
    inline static bool useColor_ = true;
    inline static bool showTimestamp_ = true;
    inline static std::mutex mutex_;
};

inline const char* Logger::levelName(Level level) {
    switch(level) {
        case Level::Debug: return "DEBUG";
        case Level::Info:  return "INFO";
        case Level::Warn:  return "WARN";
        case Level::Error: return "ERROR";
    }
    return "INFO";
}

inline const char* Logger::levelColor(Level level) {
    switch(level) {
        case Level::Debug: return ANSI_COLOR_BLUE;
        case Level::Info:  return ANSI_COLOR_GREEN;
        case Level::Warn:  return ANSI_COLOR_YELLOW;
        case Level::Error: return ANSI_COLOR_RED;
    }
    return ANSI_COLOR_RESET;
}

inline std::string Logger::nowString() {
    using namespace std::chrono;
    auto now = system_clock::now();
    std::time_t t = system_clock::to_time_t(now);
    std::tm tm{};
#if defined(_WIN32)
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tm, "%H:%M:%S");
    return oss.str();
}

inline void Logger::write(Level level, const std::string& module, const std::string& msg) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto& stream = (level == Level::Warn || level == Level::Error) ? std::cerr : std::cout;
    if(useColor_) stream << levelColor(level);

    stream << '[';
    if(showTimestamp_) {
        stream << nowString() << ' ';
    }
    stream << levelName(level) << ']';

    if(!module.empty()) {
        stream << '[' << module << ']';
    }

    stream << ' ' << msg;

    if(useColor_) stream << ANSI_COLOR_RESET;
    stream << std::endl;
}

#define LOG_DEBUG(module, ...) Logger::log(Logger::Level::Debug, module, __VA_ARGS__)
#define LOG_INFO(module, ...)  Logger::log(Logger::Level::Info,  module, __VA_ARGS__)
#define LOG_WARN(module, ...)  Logger::log(Logger::Level::Warn,  module, __VA_ARGS__)
#define LOG_ERROR(module, ...) Logger::log(Logger::Level::Error, module, __VA_ARGS__)

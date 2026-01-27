// C:\FarfadetsCorp\AgentSmith\include\logging\log_macros.h

#pragma once

#include <string>

namespace smith::logging {

enum class LogLevel : int {
    Verbose = 0,
    Debug = 1,
    Info = 2,
    Warning = 3,
    Error = 4,
    Fatal = 5
};

namespace Category {
    constexpr const char* Core = "Core";
    constexpr const char* Config = "Config";
    constexpr const char* Agent = "Agent";
    constexpr const char* Terminal = "Terminal";
    constexpr const char* UI = "UI";
    constexpr const char* Network = "Network";
    constexpr const char* Platform = "Platform";
    constexpr const char* WebView = "WebView";
}

const char* LogLevelToString(LogLevel level);
LogLevel StringToLogLevel(const std::string& str);

} // namespace smith::logging

// Forward declaration for macro
namespace smith::logging { class Logger; }

#define SMITH_LOG(Level, Category, Format, ...) \
    ::smith::logging::Logger::Instance().Log( \
        ::smith::logging::LogLevel::Level, \
        Category, \
        __FILE__, __LINE__, __FUNCTION__, \
        Format, ##__VA_ARGS__)

#define SMITH_VERBOSE(Cat, Fmt, ...) SMITH_LOG(Verbose, Cat, Fmt, ##__VA_ARGS__)
#define SMITH_DEBUG(Cat, Fmt, ...)   SMITH_LOG(Debug, Cat, Fmt, ##__VA_ARGS__)
#define SMITH_INFO(Cat, Fmt, ...)    SMITH_LOG(Info, Cat, Fmt, ##__VA_ARGS__)
#define SMITH_WARN(Cat, Fmt, ...)    SMITH_LOG(Warning, Cat, Fmt, ##__VA_ARGS__)
#define SMITH_ERROR(Cat, Fmt, ...)   SMITH_LOG(Error, Cat, Fmt, ##__VA_ARGS__)
#define SMITH_FATAL(Cat, Fmt, ...)   SMITH_LOG(Fatal, Cat, Fmt, ##__VA_ARGS__)

#define SMITH_ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            SMITH_FATAL("Assert", "Assertion failed: {} at {}:{}", \
                        message, __FILE__, __LINE__); \
            std::abort(); \
        } \
    } while (false)

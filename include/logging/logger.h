// C:\FarfadetsCorp\AgentSmith\include\logging\logger.h

#pragma once

#include "log_macros.h"
#include <memory>
#include <vector>
#include <mutex>
#include <deque>
#include <chrono>
#include <thread>
#include <functional>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/fmt.h>

namespace smith::logging {

struct LogEntry {
    LogLevel level;
    std::string category;
    std::string message;
    std::string file;
    int line;
    std::string function;
    std::chrono::system_clock::time_point timestamp;
    std::thread::id threadId;
};

class ILogSink {
public:
    virtual ~ILogSink() = default;
    virtual void Write(const LogEntry& entry) = 0;
    virtual void Flush() {}
    virtual const char* GetName() const = 0;
};

class Logger {
public:
    static Logger& Instance();

    template<typename... Args>
    void Log(LogLevel level, const char* category,
             const char* file, int line, const char* function,
             const char* format, Args&&... args) {
        if (level < m_minLevel) return;
        if (!IsCategoryEnabled(category)) return;

        std::string message;
        try {
            message = fmt::format(fmt::runtime(format), std::forward<Args>(args)...);
        } catch (const fmt::format_error& e) {
            message = std::string("FORMAT ERROR: ") + format;
        }

        LogEntry entry{
            level, category, std::move(message),
            ExtractFileName(file), line, function,
            std::chrono::system_clock::now(),
            std::this_thread::get_id()
        };

        Dispatch(entry);
    }

    void AddSink(std::shared_ptr<ILogSink> sink);
    void RemoveSink(const std::string& name);
    void ClearSinks();

    void SetMinLevel(LogLevel level) { m_minLevel = level; }
    LogLevel GetMinLevel() const { return m_minLevel; }

    void SetCategoryFilter(const std::string& filter);
    bool IsCategoryEnabled(const std::string& category) const;

    void Flush();

private:
    Logger();
    ~Logger();
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    void Dispatch(const LogEntry& entry);
    static std::string ExtractFileName(const char* path);

    std::vector<std::shared_ptr<ILogSink>> m_sinks;
    mutable std::mutex m_mutex;
    LogLevel m_minLevel = LogLevel::Info;
    std::vector<std::string> m_enabledCategories;
    bool m_filterCategories = false;
};

} // namespace smith::logging

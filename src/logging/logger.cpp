// C:\FarfadetsCorp\AgentSmith\src\logging\logger.cpp

#include "logging/logger.h"
#include <algorithm>

namespace smith::logging {

const char* LogLevelToString(LogLevel level) {
    switch (level) {
        case LogLevel::Verbose: return "VERBOSE";
        case LogLevel::Debug:   return "DEBUG";
        case LogLevel::Info:    return "INFO";
        case LogLevel::Warning: return "WARN";
        case LogLevel::Error:   return "ERROR";
        case LogLevel::Fatal:   return "FATAL";
        default:                return "UNKNOWN";
    }
}

LogLevel StringToLogLevel(const std::string& str) {
    if (str == "Verbose" || str == "VERBOSE") return LogLevel::Verbose;
    if (str == "Debug" || str == "DEBUG") return LogLevel::Debug;
    if (str == "Info" || str == "INFO") return LogLevel::Info;
    if (str == "Warning" || str == "WARN") return LogLevel::Warning;
    if (str == "Error" || str == "ERROR") return LogLevel::Error;
    if (str == "Fatal" || str == "FATAL") return LogLevel::Fatal;
    return LogLevel::Info;
}

Logger& Logger::Instance() {
    static Logger instance;
    return instance;
}

Logger::Logger() = default;
Logger::~Logger() = default;

void Logger::AddSink(std::shared_ptr<ILogSink> sink) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_sinks.push_back(std::move(sink));
}

void Logger::RemoveSink(const std::string& name) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_sinks.erase(
        std::remove_if(m_sinks.begin(), m_sinks.end(),
            [&name](const auto& sink) { return sink->GetName() == name; }),
        m_sinks.end()
    );
}

void Logger::ClearSinks() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_sinks.clear();
}

void Logger::SetCategoryFilter(const std::string& filter) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_enabledCategories.clear();
    m_filterCategories = !filter.empty();

    if (!filter.empty()) {
        size_t start = 0;
        size_t end;
        while ((end = filter.find(',', start)) != std::string::npos) {
            std::string cat = filter.substr(start, end - start);
            if (!cat.empty()) m_enabledCategories.push_back(cat);
            start = end + 1;
        }
        std::string last = filter.substr(start);
        if (!last.empty()) m_enabledCategories.push_back(last);
    }
}

bool Logger::IsCategoryEnabled(const std::string& category) const {
    if (!m_filterCategories) return true;

    std::lock_guard<std::mutex> lock(m_mutex);
    return std::find(m_enabledCategories.begin(), m_enabledCategories.end(), category)
           != m_enabledCategories.end();
}

void Logger::Dispatch(const LogEntry& entry) {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto& sink : m_sinks) {
        sink->Write(entry);
    }
}

void Logger::Flush() {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto& sink : m_sinks) {
        sink->Flush();
    }
}

std::string Logger::ExtractFileName(const char* path) {
    std::string fullPath(path);
    size_t pos = fullPath.find_last_of("/\\");
    return (pos != std::string::npos) ? fullPath.substr(pos + 1) : fullPath;
}

} // namespace smith::logging

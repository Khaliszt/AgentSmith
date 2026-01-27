// C:\FarfadetsCorp\AgentSmith\src\logging\log_sinks.cpp

#include "logging/log_sink.h"
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <iostream>
#include <iomanip>
#include <ctime>

namespace smith::logging {

// === FileSink ===

FileSink::FileSink(const std::string& path, size_t maxSize, int maxFiles) {
    auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
        path, maxSize, maxFiles);
    m_logger = std::make_shared<spdlog::logger>("file_logger", file_sink);
    m_logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] %v");
    m_logger->flush_on(spdlog::level::warn);
}

void FileSink::Write(const LogEntry& entry) {
    std::string formatted = fmt::format("[{}] [{}] {}",
        entry.category, entry.function, entry.message);

    switch (entry.level) {
        case LogLevel::Verbose: m_logger->trace(formatted); break;
        case LogLevel::Debug:   m_logger->debug(formatted); break;
        case LogLevel::Info:    m_logger->info(formatted); break;
        case LogLevel::Warning: m_logger->warn(formatted); break;
        case LogLevel::Error:   m_logger->error(formatted); break;
        case LogLevel::Fatal:   m_logger->critical(formatted); break;
    }
}

void FileSink::Flush() {
    m_logger->flush();
}

// === ConsoleSink ===

ConsoleSink::ConsoleSink(bool useColors) : m_useColors(useColors) {}

void ConsoleSink::Write(const LogEntry& entry) {
    auto time_t = std::chrono::system_clock::to_time_t(entry.timestamp);
    std::tm tm;
#ifdef _WIN32
    localtime_s(&tm, &time_t);
#else
    localtime_r(&time_t, &tm);
#endif

    std::cout << std::put_time(&tm, "%H:%M:%S") << " "
              << "[" << LogLevelToString(entry.level) << "] "
              << "[" << entry.category << "] "
              << entry.message << std::endl;
}

// === ImGuiSink ===

ImGuiSink::ImGuiSink(size_t maxEntries) : m_maxEntries(maxEntries) {}

void ImGuiSink::Write(const LogEntry& entry) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_entries.push_back(entry);
    while (m_entries.size() > m_maxEntries) {
        m_entries.pop_front();
    }
}

std::deque<LogEntry> ImGuiSink::GetEntries() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_entries;
}

void ImGuiSink::Clear() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_entries.clear();
}

} // namespace smith::logging

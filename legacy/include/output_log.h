#pragma once

#include <string>
#include <vector>
#include <mutex>
#include <chrono>
#include <fstream>

namespace AgentSmith {

//=============================================================================
// Output Log
//
// ImGui-based output log that replaces console output.
// Thread-safe singleton for logging from any part of the application.
// Also logs to file for post-mortem debugging.
//=============================================================================

enum class LogLevel {
    Debug,
    Info,
    Warning,
    Error
};

struct LogEntry {
    std::chrono::system_clock::time_point timestamp;
    LogLevel level;
    std::string message;
    std::string source;  // Optional: which component logged this
};

class OutputLog {
public:
    // Singleton access
    static OutputLog& Instance();

    // Prevent copying
    OutputLog(const OutputLog&) = delete;
    OutputLog& operator=(const OutputLog&) = delete;

    // Logging functions (thread-safe)
    void Debug(const std::string& message, const std::string& source = "");
    void Info(const std::string& message, const std::string& source = "");
    void Warning(const std::string& message, const std::string& source = "");
    void Error(const std::string& message, const std::string& source = "");
    void Log(LogLevel level, const std::string& message, const std::string& source = "");

    // Render the log window
    void Render(bool* p_open = nullptr);

    // Clear all entries
    void Clear();

    // Get entry count
    size_t GetEntryCount() const;

    // File logging
    void SetFileLogging(bool enable, const std::string& filename = "agentsmith.log");
    bool IsFileLoggingEnabled() const { return m_fileLoggingEnabled; }
    std::string GetLogFilePath() const { return m_logFilePath; }

    // Settings
    void SetMaxEntries(size_t max) { m_maxEntries = max; }
    void SetAutoScroll(bool autoScroll) { m_autoScroll = autoScroll; }
    bool GetAutoScroll() const { return m_autoScroll; }

    // Filter settings
    void SetShowDebug(bool show) { m_showDebug = show; }
    void SetShowInfo(bool show) { m_showInfo = show; }
    void SetShowWarning(bool show) { m_showWarning = show; }
    void SetShowError(bool show) { m_showError = show; }

private:
    OutputLog();
    ~OutputLog();

    void AddEntry(LogLevel level, const std::string& message, const std::string& source);
    void WriteToFile(const LogEntry& entry);
    const char* GetLevelName(LogLevel level) const;
    unsigned int GetLevelColor(LogLevel level) const;

    std::vector<LogEntry> m_entries;
    mutable std::mutex m_mutex;

    size_t m_maxEntries = 1000;
    bool m_autoScroll = true;
    bool m_scrollToBottom = false;

    // Filters
    bool m_showDebug = true;
    bool m_showInfo = true;
    bool m_showWarning = true;
    bool m_showError = true;

    // Filter text
    char m_filterText[256] = "";

    // File logging
    bool m_fileLoggingEnabled = false;
    std::string m_logFilePath;
    std::ofstream m_logFile;
};

// Convenience macros
#define LOG_DEBUG(msg) AgentSmith::OutputLog::Instance().Debug(msg)
#define LOG_INFO(msg) AgentSmith::OutputLog::Instance().Info(msg)
#define LOG_WARNING(msg) AgentSmith::OutputLog::Instance().Warning(msg)
#define LOG_ERROR(msg) AgentSmith::OutputLog::Instance().Error(msg)

#define LOG_DEBUG_SRC(msg, src) AgentSmith::OutputLog::Instance().Debug(msg, src)
#define LOG_INFO_SRC(msg, src) AgentSmith::OutputLog::Instance().Info(msg, src)
#define LOG_WARNING_SRC(msg, src) AgentSmith::OutputLog::Instance().Warning(msg, src)
#define LOG_ERROR_SRC(msg, src) AgentSmith::OutputLog::Instance().Error(msg, src)

} // namespace AgentSmith

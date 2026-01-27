// C:\FarfadetsCorp\AgentSmith\include\logging\log_sink.h

#pragma once

#include "logger.h"
#include <deque>
#include <spdlog/sinks/rotating_file_sink.h>

namespace smith::logging {

class FileSink : public ILogSink {
public:
    FileSink(const std::string& path, size_t maxSize = 10*1024*1024, int maxFiles = 3);
    void Write(const LogEntry& entry) override;
    void Flush() override;
    const char* GetName() const override { return "File"; }
private:
    std::shared_ptr<spdlog::logger> m_logger;
};

class ConsoleSink : public ILogSink {
public:
    ConsoleSink(bool useColors = true);
    void Write(const LogEntry& entry) override;
    const char* GetName() const override { return "Console"; }
private:
    bool m_useColors;
};

class ImGuiSink : public ILogSink {
public:
    ImGuiSink(size_t maxEntries = 1000);
    void Write(const LogEntry& entry) override;
    const char* GetName() const override { return "ImGui"; }

    std::deque<LogEntry> GetEntries() const;
    void Clear();

private:
    std::deque<LogEntry> m_entries;
    size_t m_maxEntries;
    mutable std::mutex m_mutex;
};

} // namespace smith::logging

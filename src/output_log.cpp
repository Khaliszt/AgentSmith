#include "output_log.h"
#include <imgui.h>
#include <ctime>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <filesystem>

namespace AgentSmith {

OutputLog& OutputLog::Instance() {
    static OutputLog instance;
    return instance;
}

OutputLog::OutputLog() {
    // Reserve some initial capacity
    m_entries.reserve(100);

    // Enable file logging by default
    SetFileLogging(true);
}

OutputLog::~OutputLog() {
    if (m_logFile.is_open()) {
        m_logFile.close();
    }
}

void OutputLog::SetFileLogging(bool enable, const std::string& filename) {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_logFile.is_open()) {
        m_logFile.close();
    }

    m_fileLoggingEnabled = enable;

    if (enable) {
        // Get executable directory
        try {
            m_logFilePath = filename;
            m_logFile.open(m_logFilePath, std::ios::out | std::ios::app);

            if (m_logFile.is_open()) {
                // Write session header
                auto now = std::chrono::system_clock::now();
                auto time_t_val = std::chrono::system_clock::to_time_t(now);
                std::tm tm_val;
#ifdef _WIN32
                localtime_s(&tm_val, &time_t_val);
#else
                localtime_r(&time_t_val, &tm_val);
#endif
                char timeStr[64];
                std::strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", &tm_val);

                m_logFile << "\n========================================\n";
                m_logFile << "AgentSmith Session Started: " << timeStr << "\n";
                m_logFile << "========================================\n";
                m_logFile.flush();
            }
        } catch (...) {
            m_fileLoggingEnabled = false;
        }
    }
}

void OutputLog::Debug(const std::string& message, const std::string& source) {
    AddEntry(LogLevel::Debug, message, source);
}

void OutputLog::Info(const std::string& message, const std::string& source) {
    AddEntry(LogLevel::Info, message, source);
}

void OutputLog::Warning(const std::string& message, const std::string& source) {
    AddEntry(LogLevel::Warning, message, source);
}

void OutputLog::Error(const std::string& message, const std::string& source) {
    AddEntry(LogLevel::Error, message, source);
}

void OutputLog::Log(LogLevel level, const std::string& message, const std::string& source) {
    AddEntry(level, message, source);
}

void OutputLog::AddEntry(LogLevel level, const std::string& message, const std::string& source) {
    std::lock_guard<std::mutex> lock(m_mutex);

    LogEntry entry;
    entry.timestamp = std::chrono::system_clock::now();
    entry.level = level;
    entry.message = message;
    entry.source = source;

    // Write to file first (before move)
    if (m_fileLoggingEnabled && m_logFile.is_open()) {
        WriteToFile(entry);
    }

    m_entries.push_back(std::move(entry));

    // Trim if over max
    if (m_entries.size() > m_maxEntries) {
        m_entries.erase(m_entries.begin(), m_entries.begin() + (m_entries.size() - m_maxEntries));
    }

    m_scrollToBottom = m_autoScroll;
}

void OutputLog::WriteToFile(const LogEntry& entry) {
    // Format timestamp
    auto time_t_val = std::chrono::system_clock::to_time_t(entry.timestamp);
    std::tm tm_val;
#ifdef _WIN32
    localtime_s(&tm_val, &time_t_val);
#else
    localtime_r(&time_t_val, &tm_val);
#endif

    char timeStr[32];
    std::strftime(timeStr, sizeof(timeStr), "%H:%M:%S", &tm_val);

    // Get milliseconds
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        entry.timestamp.time_since_epoch()) % 1000;

    // Write to file
    if (entry.source.empty()) {
        m_logFile << "[" << timeStr << "." << std::setfill('0') << std::setw(3) << ms.count()
                  << "] [" << GetLevelName(entry.level) << "] "
                  << entry.message << "\n";
    } else {
        m_logFile << "[" << timeStr << "." << std::setfill('0') << std::setw(3) << ms.count()
                  << "] [" << GetLevelName(entry.level) << "] [" << entry.source << "] "
                  << entry.message << "\n";
    }
    m_logFile.flush();
}

void OutputLog::Clear() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_entries.clear();
}

size_t OutputLog::GetEntryCount() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_entries.size();
}

const char* OutputLog::GetLevelName(LogLevel level) const {
    switch (level) {
        case LogLevel::Debug:   return "DEBUG";
        case LogLevel::Info:    return "INFO";
        case LogLevel::Warning: return "WARN";
        case LogLevel::Error:   return "ERROR";
        default:                return "???";
    }
}

unsigned int OutputLog::GetLevelColor(LogLevel level) const {
    switch (level) {
        case LogLevel::Debug:   return IM_COL32(150, 150, 150, 255);  // Gray
        case LogLevel::Info:    return IM_COL32(200, 200, 200, 255);  // Light gray
        case LogLevel::Warning: return IM_COL32(255, 200, 100, 255);  // Yellow/Orange
        case LogLevel::Error:   return IM_COL32(255, 100, 100, 255);  // Red
        default:                return IM_COL32(255, 255, 255, 255);
    }
}

void OutputLog::Render(bool* p_open) {
    ImGui::SetNextWindowSize(ImVec2(600, 300), ImGuiCond_FirstUseEver);

    if (!ImGui::Begin("Output Log", p_open, ImGuiWindowFlags_MenuBar)) {
        ImGui::End();
        return;
    }

    // Menu bar
    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("Options")) {
            ImGui::MenuItem("Auto-scroll", nullptr, &m_autoScroll);
            ImGui::Separator();
            if (ImGui::MenuItem("Clear")) {
                Clear();
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Filter")) {
            ImGui::MenuItem("Debug", nullptr, &m_showDebug);
            ImGui::MenuItem("Info", nullptr, &m_showInfo);
            ImGui::MenuItem("Warning", nullptr, &m_showWarning);
            ImGui::MenuItem("Error", nullptr, &m_showError);
            ImGui::EndMenu();
        }

        ImGui::EndMenuBar();
    }

    // Filter buttons (quick access)
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 4));

    if (ImGui::SmallButton("Clear")) {
        Clear();
    }
    ImGui::SameLine();

    ImGui::PushStyleColor(ImGuiCol_Button, m_showDebug ? IM_COL32(60, 60, 60, 255) : IM_COL32(40, 40, 40, 255));
    if (ImGui::SmallButton("DBG")) m_showDebug = !m_showDebug;
    ImGui::PopStyleColor();
    ImGui::SameLine();

    ImGui::PushStyleColor(ImGuiCol_Button, m_showInfo ? IM_COL32(60, 60, 80, 255) : IM_COL32(40, 40, 40, 255));
    if (ImGui::SmallButton("INF")) m_showInfo = !m_showInfo;
    ImGui::PopStyleColor();
    ImGui::SameLine();

    ImGui::PushStyleColor(ImGuiCol_Button, m_showWarning ? IM_COL32(80, 70, 40, 255) : IM_COL32(40, 40, 40, 255));
    if (ImGui::SmallButton("WRN")) m_showWarning = !m_showWarning;
    ImGui::PopStyleColor();
    ImGui::SameLine();

    ImGui::PushStyleColor(ImGuiCol_Button, m_showError ? IM_COL32(80, 40, 40, 255) : IM_COL32(40, 40, 40, 255));
    if (ImGui::SmallButton("ERR")) m_showError = !m_showError;
    ImGui::PopStyleColor();
    ImGui::SameLine();

    // Text filter
    ImGui::SetNextItemWidth(200);
    ImGui::InputTextWithHint("##Filter", "Filter...", m_filterText, sizeof(m_filterText));

    ImGui::PopStyleVar();

    ImGui::Separator();

    // Log content
    ImGui::BeginChild("LogScrollRegion", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

    std::lock_guard<std::mutex> lock(m_mutex);

    std::string filterStr(m_filterText);
    std::transform(filterStr.begin(), filterStr.end(), filterStr.begin(), ::tolower);

    for (const auto& entry : m_entries) {
        // Filter by level
        bool show = false;
        switch (entry.level) {
            case LogLevel::Debug:   show = m_showDebug; break;
            case LogLevel::Info:    show = m_showInfo; break;
            case LogLevel::Warning: show = m_showWarning; break;
            case LogLevel::Error:   show = m_showError; break;
        }

        if (!show) continue;

        // Filter by text
        if (!filterStr.empty()) {
            std::string msgLower = entry.message;
            std::transform(msgLower.begin(), msgLower.end(), msgLower.begin(), ::tolower);

            std::string srcLower = entry.source;
            std::transform(srcLower.begin(), srcLower.end(), srcLower.begin(), ::tolower);

            if (msgLower.find(filterStr) == std::string::npos &&
                srcLower.find(filterStr) == std::string::npos) {
                continue;
            }
        }

        // Format timestamp
        auto time_t_val = std::chrono::system_clock::to_time_t(entry.timestamp);
        std::tm tm_val;
#ifdef _WIN32
        localtime_s(&tm_val, &time_t_val);
#else
        localtime_r(&time_t_val, &tm_val);
#endif

        char timeStr[32];
        std::strftime(timeStr, sizeof(timeStr), "%H:%M:%S", &tm_val);

        // Get milliseconds
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            entry.timestamp.time_since_epoch()) % 1000;

        // Build display string
        ImGui::PushStyleColor(ImGuiCol_Text, GetLevelColor(entry.level));

        if (entry.source.empty()) {
            ImGui::Text("[%s.%03d] [%s] %s",
                timeStr,
                static_cast<int>(ms.count()),
                GetLevelName(entry.level),
                entry.message.c_str());
        } else {
            ImGui::Text("[%s.%03d] [%s] [%s] %s",
                timeStr,
                static_cast<int>(ms.count()),
                GetLevelName(entry.level),
                entry.source.c_str(),
                entry.message.c_str());
        }

        ImGui::PopStyleColor();
    }

    // Auto-scroll
    if (m_scrollToBottom) {
        ImGui::SetScrollHereY(1.0f);
        m_scrollToBottom = false;
    }

    ImGui::EndChild();
    ImGui::End();
}

} // namespace AgentSmith

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <chrono>
#include <functional>
#include <atomic>
#include <mutex>

namespace AgentSmith {

//=============================================================================
// Agent Status & Types
//=============================================================================

enum class AgentStatus {
    Idle,           // Agent is ready but not running
    Running,        // Agent is actively processing
    Waiting,        // Agent is waiting for user input
    Error,          // Agent encountered an error
    Stopped         // Agent has been stopped
};

enum class AgentType {
    ClaudeCode,     // Anthropic Claude Code CLI
    Aider,          // Aider coding assistant
    Cursor,         // Cursor AI
    Custom          // User-defined agent
};

//=============================================================================
// Git Information
//=============================================================================

struct GitInfo {
    std::string branch;
    std::string repo_name;
    std::string last_commit_hash;
    std::string last_commit_message;
    int uncommitted_changes = 0;
    int ahead = 0;
    int behind = 0;
    bool is_git_repo = false;
    
    std::chrono::system_clock::time_point last_updated;
};

//=============================================================================
// File Activity (for future visualizer)
//=============================================================================

struct FileActivity {
    std::string filepath;
    std::string action;  // "created", "modified", "deleted", "read"
    std::chrono::system_clock::time_point timestamp;
};

//=============================================================================
// Agent Definition
//=============================================================================

struct Agent {
    // Identity
    std::string id;
    std::string name;
    AgentType type = AgentType::ClaudeCode;
    
    // Working directory
    std::string working_directory;
    
    // Command to launch
    std::string command;
    std::vector<std::string> args;
    
    // Status
    AgentStatus status = AgentStatus::Idle;
    std::string status_message;
    
    // Process info
    int pid = -1;
    
    // Git info for this agent's working directory
    GitInfo git_info;
    
    // Terminal embedding
    void* native_terminal_handle = nullptr;  // Platform-specific handle
    bool terminal_embedded = false;
    
    // Statistics
    std::chrono::system_clock::time_point started_at;
    std::chrono::system_clock::time_point last_activity;
    int tokens_used = 0;
    int messages_sent = 0;

    // Session tracking (for embedded terminal)
    std::chrono::system_clock::time_point session_start;
    int tokens_remaining = -1;  // -1 = unknown
    std::string project_name;
    bool needs_attention = false;

    // Per-agent settings (configurable via right-click)
    bool auto_accept_edits = false;  // Default OFF for safety

    // File activity tracking (for future visualizer)
    std::vector<FileActivity> recent_files;

    // Position in grid (for layout persistence)
    int grid_row = 0;
    int grid_col = 0;
    
    Agent() = default;
    Agent(const std::string& n, const std::string& wd, AgentType t = AgentType::ClaudeCode)
        : name(n), working_directory(wd), type(t) {
        id = std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
    }
};

//=============================================================================
// Grid Layout Configuration
//=============================================================================

struct GridConfig {
    int rows = 2;
    int cols = 2;
    float panel_padding = 4.0f;
    float info_overlay_height = 28.0f;
    bool show_git_info = true;
    bool show_status_indicator = true;
    bool show_file_activity = false;  // Future feature
};

//=============================================================================
// Application Configuration
//=============================================================================

struct AppConfig {
    // Window settings
    int window_width = 1920;
    int window_height = 1080;
    bool fullscreen = false;
    bool dark_mode = true;
    
    // Grid settings
    GridConfig grid;
    
    // Default agent settings
    std::string default_terminal_command;  // e.g., "wt.exe" on Windows, "gnome-terminal" on Linux
    std::string default_shell;             // e.g., "powershell.exe", "bash"
    
    // Theme colors
    struct {
        float accent_r = 0.85f;
        float accent_g = 0.45f;
        float accent_b = 0.25f;
    } theme;
    
    // Saved agents
    std::vector<Agent> agents;
};

//=============================================================================
// Status Colors Helper
//=============================================================================

inline void GetStatusColor(AgentStatus status, float& r, float& g, float& b) {
    switch (status) {
        case AgentStatus::Running:
            r = 0.3f; g = 0.85f; b = 0.4f;  // Green
            break;
        case AgentStatus::Waiting:
            r = 0.9f; g = 0.75f; b = 0.2f;  // Yellow
            break;
        case AgentStatus::Error:
            r = 0.9f; g = 0.3f; b = 0.3f;   // Red
            break;
        case AgentStatus::Stopped:
            r = 0.5f; g = 0.5f; b = 0.5f;   // Gray
            break;
        case AgentStatus::Idle:
        default:
            r = 0.4f; g = 0.6f; b = 0.9f;   // Blue
            break;
    }
}

inline const char* GetStatusText(AgentStatus status) {
    switch (status) {
        case AgentStatus::Running: return "Running";
        case AgentStatus::Waiting: return "Waiting";
        case AgentStatus::Error:   return "Error";
        case AgentStatus::Stopped: return "Stopped";
        case AgentStatus::Idle:
        default:                   return "Idle";
    }
}

inline const char* GetAgentTypeText(AgentType type) {
    switch (type) {
        case AgentType::ClaudeCode: return "Claude Code";
        case AgentType::Aider:      return "Aider";
        case AgentType::Cursor:     return "Cursor";
        case AgentType::Custom:
        default:                    return "Custom";
    }
}

} // namespace AgentSmith

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
// Color Representation
//=============================================================================

struct FColor {
    float r = 1.0f;
    float g = 1.0f;
    float b = 1.0f;

    FColor() = default;
    FColor(float r_, float g_, float b_) : r(r_), g(g_), b(b_) {}
};

// Helper to convert hex color string "#RRGGBB" to uint32_t (ABGR format for ImGui)
inline uint32_t HexToColor(const char* hex) {
    if (!hex || hex[0] != '#' || strlen(hex) < 7) return 0xFFFFFFFF;
    unsigned int r, g, b;
    sscanf(hex + 1, "%02x%02x%02x", &r, &g, &b);
    return 0xFF000000 | (b << 16) | (g << 8) | r;  // ABGR format
}

//=============================================================================
// Terminal Theme
//=============================================================================

struct TerminalTheme {
    std::string name;
    uint32_t background;
    uint32_t foreground;
    uint32_t cursorColor;
    uint32_t selectionBackground;
    uint32_t ansiColors[16];  // 0-7: normal, 8-15: bright

    TerminalTheme() = default;

    // Construct from hex color strings (PowerShell/Windows Terminal format)
    TerminalTheme(const std::string& themeName,
                  const char* bg, const char* fg, const char* cursor, const char* selection,
                  const char* black, const char* red, const char* green, const char* yellow,
                  const char* blue, const char* purple, const char* cyan, const char* white,
                  const char* brightBlack, const char* brightRed, const char* brightGreen, const char* brightYellow,
                  const char* brightBlue, const char* brightPurple, const char* brightCyan, const char* brightWhite)
        : name(themeName)
        , background(HexToColor(bg))
        , foreground(HexToColor(fg))
        , cursorColor(HexToColor(cursor))
        , selectionBackground(HexToColor(selection))
    {
        ansiColors[0] = HexToColor(black);
        ansiColors[1] = HexToColor(red);
        ansiColors[2] = HexToColor(green);
        ansiColors[3] = HexToColor(yellow);
        ansiColors[4] = HexToColor(blue);
        ansiColors[5] = HexToColor(purple);
        ansiColors[6] = HexToColor(cyan);
        ansiColors[7] = HexToColor(white);
        ansiColors[8] = HexToColor(brightBlack);
        ansiColors[9] = HexToColor(brightRed);
        ansiColors[10] = HexToColor(brightGreen);
        ansiColors[11] = HexToColor(brightYellow);
        ansiColors[12] = HexToColor(brightBlue);
        ansiColors[13] = HexToColor(brightPurple);
        ansiColors[14] = HexToColor(brightCyan);
        ansiColors[15] = HexToColor(brightWhite);
    }
};

//=============================================================================
// Predefined Terminal Themes
//=============================================================================

namespace TerminalThemes {

inline TerminalTheme Breeze() {
    return TerminalTheme("Breeze",
        "#31363b", "#eff0f1", "#eff0f1", "#eff0f1",
        "#31363b", "#ed1515", "#11d116", "#f67400",
        "#1d99f3", "#9b59b6", "#1abc9c", "#eff0f1",
        "#7f8c8d", "#c0392b", "#1cdc9a", "#fdbc4b",
        "#3daee9", "#8e44ad", "#16a085", "#fcfcfc");
}

inline TerminalTheme CatppuccinMacchiato() {
    return TerminalTheme("Catppuccin Macchiato",
        "#24273a", "#cad3f5", "#f4dbd6", "#5b6078",
        "#494d64", "#ed8796", "#a6da95", "#eed49f",
        "#8aadf4", "#f5bde6", "#8bd5ca", "#b8c0e0",
        "#5b6078", "#ed8796", "#a6da95", "#eed49f",
        "#8aadf4", "#f5bde6", "#8bd5ca", "#a5adcb");
}

inline TerminalTheme Chalk() {
    return TerminalTheme("Chalk",
        "#2b2d2e", "#d2d8d9", "#708284", "#e4e8ed",
        "#7d8b8f", "#b23a52", "#789b6a", "#b9ac4a",
        "#2a7fac", "#bd4f5a", "#44a799", "#d2d8d9",
        "#888888", "#f24840", "#80c470", "#ffeb62",
        "#4196ff", "#fc5275", "#53cdbd", "#d2d8d9");
}

inline TerminalTheme Ciapre() {
    return TerminalTheme("Ciapre",
        "#191c27", "#aea47a", "#92805b", "#172539",
        "#181818", "#810009", "#48513b", "#cc8b3f",
        "#576d8c", "#724d7c", "#5c4f4b", "#aea47f",
        "#555555", "#ac3835", "#a6a75d", "#dcdf7c",
        "#3097c6", "#d33061", "#f3dbb2", "#f4f4f4");
}

inline TerminalTheme CyberCube() {
    return TerminalTheme("Cyber-Cube",
        "#161C2B", "#A4B1CD", "#C5D1EB", "#C5D1EB",
        "#141D2B", "#FF3E3E", "#9FEF00", "#FFAF00",
        "#2E6CFF", "#9F00FF", "#2DE2B2", "#D5E0FF",
        "#767676", "#FF8484", "#C5F467", "#FFCC5C",
        "#5CB2FF", "#CF8DFB", "#5CECC6", "#D5DAFF");
}

inline TerminalTheme Idea() {
    return TerminalTheme("Idea",
        "#202020", "#adadad", "#bbbbbb", "#44475a",
        "#adadad", "#fc5256", "#98b61c", "#ccb444",
        "#437ee7", "#9d74b0", "#248887", "#181818",
        "#ffffff", "#fc7072", "#98b61c", "#ffff0b",
        "#6c9ced", "#fc7eff", "#248887", "#181818");
}

inline TerminalTheme Jubi() {
    return TerminalTheme("Jubi",
        "#262b33", "#c3d3de", "#c3d3de", "#5b5184",
        "#3b3750", "#cf7b98", "#90a94b", "#6ebfc0",
        "#576ea6", "#bc4f68", "#75a7d2", "#c3d3de",
        "#a874ce", "#de90ab", "#bcdd61", "#87e9ea",
        "#8c9fcd", "#e16c87", "#b7c9ef", "#d5e5f1");
}

inline TerminalTheme MonokaiSoda() {
    return TerminalTheme("Monokai Soda",
        "#1a1a1a", "#c4c5b5", "#f6f7ec", "#343434",
        "#1a1a1a", "#f4005f", "#98e024", "#fa8419",
        "#9d65ff", "#f4005f", "#58d1eb", "#c4c5b5",
        "#625e4c", "#f4005f", "#98e024", "#e0d561",
        "#9d65ff", "#f4005f", "#58d1eb", "#f6f6ef");
}

inline TerminalTheme Slate() {
    return TerminalTheme("Slate",
        "#222222", "#35b1d2", "#87d3c4", "#0f3754",
        "#222222", "#e2a8bf", "#81d778", "#c4c9c0",
        "#264b49", "#a481d3", "#15ab9c", "#02c5e0",
        "#ffffff", "#ffcdd9", "#beffa8", "#d0ccca",
        "#7ab0d2", "#c5a7d9", "#8cdfe0", "#e0e0e0");
}

inline TerminalTheme Spacedust() {
    return TerminalTheme("Spacedust",
        "#0a1e24", "#ecf0c1", "#708284", "#0a385c",
        "#6e5346", "#e35b00", "#5cab96", "#e3cd7b",
        "#0f548b", "#e35b00", "#06afc7", "#f0f1ce",
        "#684c31", "#ff8a3a", "#aecab8", "#ffc878",
        "#67a0ce", "#ff8a3a", "#83a7b4", "#fefff1");
}

inline TerminalTheme Urple() {
    return TerminalTheme("Urple",
        "#1b1b23", "#877a9b", "#a063eb", "#a063eb",
        "#000000", "#b0425b", "#37a415", "#ad5c42",
        "#564d9b", "#6c3ca1", "#808080", "#87799c",
        "#5d3225", "#ff6388", "#29e620", "#f08161",
        "#867aed", "#a05eee", "#eaeaea", "#bfa3ff");
}

inline TerminalTheme WarmNeon() {
    return TerminalTheme("WarmNeon",
        "#404040", "#afdab6", "#30ff24", "#b0ad21",
        "#000000", "#e24346", "#39b13a", "#dae145",
        "#4261c5", "#f920fb", "#2abbd4", "#d0b8a3",
        "#fefcfc", "#e97071", "#9cc090", "#ddda7a",
        "#7b91d6", "#f674ba", "#5ed1e5", "#d8c8bb");
}

inline TerminalTheme WildCherry() {
    return TerminalTheme("WildCherry",
        "#1f1726", "#dafaff", "#dd00ff", "#002831",
        "#000507", "#d94085", "#2ab250", "#ffd16f",
        "#883cdc", "#ececec", "#c1b8b7", "#fff8de",
        "#009cc9", "#da6bac", "#f4dca5", "#eac066",
        "#308cba", "#ae636b", "#ff919d", "#e4838d");
}

// Get all available themes
inline std::vector<TerminalTheme> GetAllThemes() {
    return {
        Breeze(),
        CatppuccinMacchiato(),
        Chalk(),
        Ciapre(),
        CyberCube(),
        Idea(),
        Jubi(),
        MonokaiSoda(),
        Slate(),
        Spacedust(),
        Urple(),
        WarmNeon(),
        WildCherry()
    };
}

// Get theme by name
inline TerminalTheme GetThemeByName(const std::string& name) {
    auto themes = GetAllThemes();
    for (const auto& theme : themes) {
        if (theme.name == name) return theme;
    }
    return CatppuccinMacchiato();  // Default
}

} // namespace TerminalThemes

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

    // Terminal theme (empty = use default based on agent type)
    std::string terminal_theme;

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

    // UI Theme colors
    struct {
        FColor accent = FColor(0.85f, 0.45f, 0.25f);
    } theme;

    // Terminal theme settings
    std::string default_terminal_theme = "Catppuccin Macchiato";

    // Per-agent-type terminal themes (optional overrides)
    struct {
        std::string claude_code = "";   // Empty = use default
        std::string aider = "";
        std::string cursor = "";
        std::string chatgpt = "";
        std::string grok = "";
        std::string custom = "";
    } agent_themes;

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

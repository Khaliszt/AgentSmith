# AgentSmith v2.0 - Architecture Specification

> **Purpose:** This document provides a complete architectural blueprint for the AgentSmith v2.0 overhaul. It is designed to be consumed by a Planner agent to generate development workflows and task assignments.

---

## Table of Contents

1. [Executive Summary](#1-executive-summary)
2. [Current State Analysis](#2-current-state-analysis)
3. [Technology Stack](#3-technology-stack)
4. [Project Structure](#4-project-structure)
5. [Core Framework (AppBase)](#5-core-framework-appbase)
6. [Logging System](#6-logging-system)
7. [Configuration System](#7-configuration-system)
8. [Agent System](#8-agent-system)
9. [Terminal System](#9-terminal-system)
10. [UI System](#10-ui-system)
11. [Platform Abstraction](#11-platform-abstraction)
12. [Testing Infrastructure](#12-testing-infrastructure)
13. [Build System](#13-build-system)
14. [Implementation Phases](#14-implementation-phases)
15. [File Manifest](#15-file-manifest)
16. [Migration Guide](#16-migration-guide)
17. [Risk Assessment](#17-risk-assessment)

---

## 1. Executive Summary

### 1.1 Project Vision

AgentSmith is a **multi-agent mission control interface** for managing multiple AI coding agents (Claude Code, Grok, ChatGPT, Cursor, Custom) with embedded terminals, real-time status monitoring, and cost tracking.

### 1.2 Key Objectives

| Objective | Description |
|-----------|-------------|
| **Terminal Rendering Overhaul** | Replace custom ANSI parser with WebView2 + xterm.js for perfect terminal emulation |
| **Agent Provider Abstraction** | Create extensible interface supporting terminal-based and API-based agents |
| **Metrics & Cost Tracking** | Parse and display agent costs, token usage, rate limits |
| **Professional Logging** | Implement SMITH_LOG macro with categories and verbosity levels |
| **Testing Infrastructure** | Add GoogleTest for unit and integration testing |
| **Clean Architecture** | Adopt AppBase pattern from Dear-ImGui-App-Framework for separation of concerns |

### 1.3 Supported Agent Types

| Agent | Type | Integration | Priority |
|-------|------|-------------|----------|
| Claude Code | Terminal | ConPTY + output parsing | P0 (Primary) |
| Grok | API | xAI REST API | P1 |
| ChatGPT | API | OpenAI REST API | P1 |
| Cursor | Terminal | CLI (if available) | P2 |
| Custom | Terminal | User-defined command | P2 |

---

## 2. Current State Analysis

### 2.1 Existing Architecture

```
Current:
App (monolithic)
 ├── ConfigManager
 ├── AgentsTracker (vector<unique_ptr<Agent>>)
 ├── InputManager
 ├── OutputLog (singleton)
 └── GridLayout
      └── AgentWindow[]
           ├── ConPTYTerminal
           └── TerminalBuffer (custom ANSI parser)
```

### 2.2 Current Files to Migrate

| Current File | New Location | Changes Required |
|--------------|--------------|------------------|
| `include/app.h` | `include/core/application.h` | Inherit from AppBase |
| `include/types.h` | `include/core/types.h` + `include/agent/agent.h` | Split into modules |
| `include/config.h` | `include/config/config_manager.h` | Minor refactor |
| `include/agents_tracker.h` | `include/agent/agent_tracker.h` | Add provider integration |
| `include/agent_window.h` | `include/ui/agent_window.h` | Add ITerminal abstraction |
| `include/grid_layout.h` | `include/ui/grid_layout.h` | Namespace change |
| `include/conpty_terminal.h` | `include/terminal/conpty_terminal.h` | Implement ITerminal |
| `include/terminal_buffer.h` | `include/terminal/terminal_buffer.h` | Becomes fallback renderer |
| `include/output_log.h` | `include/ui/output_log.h` | Integrate with Logger |
| `include/git_utils.h` | `include/utils/git_utils.h` | Namespace change |
| `include/input_manager.h` | `include/core/input_manager.h` | Namespace change |

### 2.3 Known Issues to Address

1. **Terminal rendering limitations** - Custom ANSI parser doesn't handle all escape sequences
2. **No agent abstraction** - All agents treated identically regardless of type
3. **No metrics tracking** - Token usage, costs not captured
4. **No testing** - Zero test coverage
5. **Flat file structure** - Hard to navigate as codebase grows
6. **Inconsistent logging** - Mix of OutputLog calls and direct console output

---

## 3. Technology Stack

### 3.1 Core Dependencies (Existing)

| Dependency | Version | Purpose | Source |
|------------|---------|---------|--------|
| GLFW | 3.3.9+ | Window management, input | vcpkg or FetchContent |
| OpenGL | 3.3 Core | Rendering backend | System |
| Dear ImGui | 1.90.1+ | Immediate mode GUI | FetchContent |
| nlohmann/json | 3.11.3+ | JSON parsing | FetchContent (header-only) |

### 3.2 New Dependencies to Add

| Dependency | Version | Purpose | Integration Method |
|------------|---------|---------|-------------------|
| **WebView2 SDK** | 1.0.2210+ | Embedded browser for xterm.js | NuGet via CMake |
| **spdlog** | 1.12.0+ | Fast logging library | FetchContent |
| **fmt** | 10.1.0+ | String formatting (spdlog dep) | FetchContent |
| **GoogleTest** | 1.14.0+ | Unit testing framework | FetchContent |
| **cpp-httplib** | 0.14.0+ | HTTP client for API agents | FetchContent (header-only) |
| **xterm.js** | 5.3.0+ | Terminal emulator | Bundled in resources/ |
| **xterm-addon-fit** | 0.8.0+ | xterm.js auto-fit addon | Bundled in resources/ |

### 3.3 Windows-Specific Dependencies

| Dependency | Purpose | Notes |
|------------|---------|-------|
| ConPTY API | Pseudo-console for terminals | Windows 10 1809+ |
| WebView2 Runtime | Browser control | Pre-installed on Win10/11 |
| Windows Implementation Library (WIL) | COM helpers for WebView2 | Header-only |

### 3.4 Dependency Installation

```cmake
# cmake/Dependencies.cmake

include(FetchContent)

# Dear ImGui
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.90.1
)

# spdlog (includes fmt)
FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG v1.12.0
)

# GoogleTest
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0
)

# cpp-httplib (header-only)
FetchContent_Declare(
    httplib
    GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
    GIT_TAG v0.14.3
)

# nlohmann/json (header-only)
FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
)

# WebView2 (Windows only)
if(WIN32)
    # Download WebView2 NuGet package
    FetchContent_Declare(
        webview2
        URL https://www.nuget.org/api/v2/package/Microsoft.Web.WebView2/1.0.2210.55
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
endif()

FetchContent_MakeAvailable(imgui spdlog googletest httplib json)
if(WIN32)
    FetchContent_MakeAvailable(webview2)
endif()
```

---

## 4. Project Structure

### 4.1 Complete Directory Layout

```
AgentSmith/
├── CMakeLists.txt                     # Root build configuration
├── cmake/
│   ├── Dependencies.cmake             # FetchContent declarations
│   ├── CompilerFlags.cmake            # Platform-specific compiler settings
│   └── Testing.cmake                  # Test configuration
│
├── docs/
│   ├── Claude_Context.md              # Original context document
│   └── Claude_Smith_Architecture.md   # This document
│
├── include/
│   ├── smith.h                        # Umbrella header (optional)
│   │
│   ├── core/                          # Core framework
│   │   ├── app_base.h                 # Reusable ImGui app base class
│   │   ├── application.h              # AgentSmith main application
│   │   ├── types.h                    # Common type definitions
│   │   ├── event.h                    # Event types
│   │   ├── event_dispatcher.h         # Event routing
│   │   └── input_manager.h            # Keyboard/mouse handling
│   │
│   ├── logging/                       # Logging subsystem
│   │   ├── logger.h                   # Logger class with sinks
│   │   ├── log_macros.h               # SMITH_LOG macro definitions
│   │   └── log_sink.h                 # ILogSink interface
│   │
│   ├── config/                        # Configuration management
│   │   ├── config_manager.h           # Load/save configuration
│   │   └── app_config.h               # Configuration data structures
│   │
│   ├── agent/                         # Agent abstraction layer
│   │   ├── agent.h                    # Agent data structure
│   │   ├── agent_tracker.h            # Agent lifecycle management
│   │   ├── agent_provider.h           # IAgentProvider interface
│   │   ├── agent_registry.h           # Provider registration
│   │   ├── agent_utils.h              # Metrics parsing utilities
│   │   └── providers/                 # Concrete provider implementations
│   │       ├── claude_provider.h      # Claude Code (terminal)
│   │       ├── grok_provider.h        # Grok (xAI API)
│   │       ├── chatgpt_provider.h     # ChatGPT (OpenAI API)
│   │       ├── cursor_provider.h      # Cursor (terminal)
│   │       └── custom_provider.h      # Custom command (terminal)
│   │
│   ├── terminal/                      # Terminal subsystem
│   │   ├── terminal_interface.h       # ITerminal abstraction
│   │   ├── terminal_theme.h           # Theme data structures
│   │   ├── terminal_buffer.h          # ANSI parser (fallback renderer)
│   │   ├── conpty_terminal.h          # Windows ConPTY implementation
│   │   ├── webview_terminal.h         # WebView2 + xterm.js implementation
│   │   └── terminal_factory.h         # Terminal creation factory
│   │
│   ├── ui/                            # UI components
│   │   ├── imgui_layer.h              # ImGui setup and rendering
│   │   ├── menu_bar.h                 # Main menu bar
│   │   ├── grid_layout.h              # Agent grid layout manager
│   │   ├── agent_window.h             # Single agent view
│   │   ├── output_log.h               # Log viewer widget
│   │   └── dialogs/                   # Modal dialogs
│   │       ├── add_agent_dialog.h     # New agent dialog
│   │       ├── settings_dialog.h      # Application settings
│   │       └── confirmation_dialog.h  # Yes/No confirmation
│   │
│   ├── platform/                      # Platform abstraction
│   │   ├── platform.h                 # Platform detection macros
│   │   ├── clipboard.h                # Clipboard operations
│   │   ├── file_dialog.h              # Native file dialogs
│   │   └── process.h                  # Process spawning utilities
│   │
│   └── utils/                         # Utilities
│       ├── git_utils.h                # Git information retrieval
│       ├── string_utils.h             # String manipulation
│       ├── time_utils.h               # Time formatting
│       └── uuid.h                     # UUID generation
│
├── src/
│   ├── main.cpp                       # Entry point
│   │
│   ├── core/
│   │   ├── app_base.cpp               # AppBase implementation
│   │   ├── application.cpp            # Application implementation
│   │   ├── event_dispatcher.cpp
│   │   └── input_manager.cpp
│   │
│   ├── logging/
│   │   ├── logger.cpp                 # Logger implementation
│   │   └── log_sinks.cpp              # Concrete sink implementations
│   │
│   ├── config/
│   │   └── config_manager.cpp
│   │
│   ├── agent/
│   │   ├── agent.cpp                  # Agent factory methods
│   │   ├── agent_tracker.cpp
│   │   ├── agent_registry.cpp
│   │   ├── agent_utils.cpp            # Metric parsing implementations
│   │   └── providers/
│   │       ├── claude_provider.cpp
│   │       ├── grok_provider.cpp
│   │       ├── chatgpt_provider.cpp
│   │       ├── cursor_provider.cpp
│   │       └── custom_provider.cpp
│   │
│   ├── terminal/
│   │   ├── terminal_buffer.cpp
│   │   ├── conpty_terminal.cpp
│   │   ├── webview_terminal.cpp
│   │   └── terminal_factory.cpp
│   │
│   ├── ui/
│   │   ├── imgui_layer.cpp
│   │   ├── menu_bar.cpp
│   │   ├── grid_layout.cpp
│   │   ├── agent_window.cpp
│   │   ├── output_log.cpp
│   │   └── dialogs/
│   │       ├── add_agent_dialog.cpp
│   │       ├── settings_dialog.cpp
│   │       └── confirmation_dialog.cpp
│   │
│   ├── platform/
│   │   ├── windows/                   # Windows implementations
│   │   │   ├── clipboard_win.cpp
│   │   │   ├── file_dialog_win.cpp
│   │   │   └── process_win.cpp
│   │   ├── linux/                     # Linux implementations (future)
│   │   │   └── ...
│   │   └── macos/                     # macOS implementations (future)
│   │       └── ...
│   │
│   └── utils/
│       ├── git_utils.cpp
│       ├── string_utils.cpp
│       ├── time_utils.cpp
│       └── uuid.cpp
│
├── resources/
│   ├── fonts/
│   │   ├── JetBrainsMono-Regular.ttf
│   │   └── NerdFontsSymbols.ttf
│   │
│   ├── themes/
│   │   └── terminal_themes.json       # Predefined terminal color schemes
│   │
│   └── web/                           # xterm.js resources
│       ├── terminal.html              # Host HTML page
│       ├── terminal.css               # Custom styles
│       ├── terminal.js                # JavaScript bridge
│       ├── xterm.js                   # xterm.js library
│       ├── xterm.css                  # xterm.js styles
│       └── xterm-addon-fit.js         # Fit addon
│
├── tests/
│   ├── CMakeLists.txt                 # Test build configuration
│   ├── test_main.cpp                  # GoogleTest main
│   │
│   ├── unit/
│   │   ├── core/
│   │   │   └── test_event_dispatcher.cpp
│   │   ├── logging/
│   │   │   └── test_logger.cpp
│   │   ├── config/
│   │   │   └── test_config_manager.cpp
│   │   ├── agent/
│   │   │   ├── test_agent.cpp
│   │   │   ├── test_agent_tracker.cpp
│   │   │   └── test_agent_utils.cpp
│   │   ├── terminal/
│   │   │   └── test_terminal_buffer.cpp
│   │   └── utils/
│   │       ├── test_string_utils.cpp
│   │       └── test_git_utils.cpp
│   │
│   └── integration/
│       ├── test_agent_lifecycle.cpp
│       └── test_terminal_io.cpp
│
└── bin/                               # Build output directory
    ├── agent_smith.exe
    ├── config.json
    ├── agentsmith.log
    └── resources/                     # Copied at build time
        ├── fonts/
        ├── themes/
        └── web/
```

---

## 5. Core Framework (AppBase)

### 5.1 AppBase Class Definition

The `AppBase` class encapsulates all GLFW/OpenGL/ImGui boilerplate, allowing derived applications to focus on business logic.

```cpp
// include/core/app_base.h

#pragma once

#include <string>
#include <chrono>
#include <functional>

struct GLFWwindow;
struct ImVec2;

namespace smith::core {

/**
 * AppBase - Reusable Dear ImGui application framework
 *
 * Responsibilities:
 * - GLFW window creation and management
 * - OpenGL 3.3 context initialization
 * - Dear ImGui setup with docking support
 * - Main loop with delta time calculation
 * - Clean shutdown sequence
 *
 * Usage:
 *   class MyApp : public AppBase {
 *   protected:
 *       void OnStartUp() override { /* init */ }
 *       void OnImGuiRender() override { /* UI */ }
 *   };
 *
 *   int main() {
 *       MyApp app;
 *       return app.Run({.title = "My App"});
 *   }
 */
class AppBase {
public:
    /**
     * Configuration for application initialization
     */
    struct Config {
        std::string title = "Smith Application";
        int width = 1280;
        int height = 720;
        bool vsync = true;
        bool maximized = false;
        bool decorated = true;           // Window decorations (title bar, etc.)
        bool resizable = true;
        std::string imguiIniPath = "";   // Empty = default imgui.ini
        float fontSizePixels = 16.0f;
        std::string fontPath = "";       // Empty = use default font
    };

    AppBase();
    virtual ~AppBase();

    // Non-copyable, non-movable
    AppBase(const AppBase&) = delete;
    AppBase& operator=(const AppBase&) = delete;
    AppBase(AppBase&&) = delete;
    AppBase& operator=(AppBase&&) = delete;

    /**
     * Main entry point - initializes systems and runs main loop
     * @param config Application configuration
     * @return Exit code (0 = success)
     */
    int Run(const Config& config);

    /**
     * Request application exit (will complete current frame)
     */
    void RequestExit();

    /**
     * Check if exit has been requested
     */
    bool IsExitRequested() const { return m_exitRequested; }

    // === Accessors ===

    GLFWwindow* GetWindow() const { return m_window; }
    float GetDeltaTime() const { return m_deltaTime; }
    double GetTime() const;
    int GetWindowWidth() const { return m_windowWidth; }
    int GetWindowHeight() const { return m_windowHeight; }
    ImVec2 GetWindowSize() const;
    float GetDpiScale() const { return m_dpiScale; }

protected:
    // === Virtual hooks for derived classes ===

    /**
     * Called once after all systems initialized, before main loop
     * Use for: loading config, creating subsystems, initial UI state
     */
    virtual void OnStartUp() {}

    /**
     * Called every frame, before ImGui rendering
     * Use for: non-UI updates, background tasks, polling
     * @param deltaTime Time since last frame in seconds
     */
    virtual void OnUpdate(float deltaTime) { (void)deltaTime; }

    /**
     * Called every frame for ImGui rendering
     * Use for: all ImGui::Begin/End calls, UI logic
     */
    virtual void OnImGuiRender() {}

    /**
     * Called once before shutdown
     * Use for: saving state, cleanup
     */
    virtual void OnShutDown() {}

    /**
     * Called when window is resized
     * @param width New width in pixels
     * @param height New height in pixels
     */
    virtual void OnResize(int width, int height) { (void)width; (void)height; }

    /**
     * Called when files are dropped onto window
     * @param count Number of files
     * @param paths Array of file paths
     */
    virtual void OnFileDrop(int count, const char** paths) { (void)count; (void)paths; }

    /**
     * Called when window gains/loses focus
     * @param focused True if window gained focus
     */
    virtual void OnFocusChanged(bool focused) { (void)focused; }

private:
    // Initialization sequence
    bool InitializeGLFW(const Config& config);
    bool InitializeOpenGL();
    bool InitializeImGui(const Config& config);
    void LoadFonts(const Config& config);

    // Main loop
    void MainLoop();
    void BeginFrame();
    void EndFrame();

    // Shutdown sequence
    void ShutdownImGui();
    void ShutdownOpenGL();
    void ShutdownGLFW();

    // GLFW callbacks (static to match GLFW signature)
    static void GLFWErrorCallback(int error, const char* description);
    static void GLFWFramebufferSizeCallback(GLFWwindow* window, int width, int height);
    static void GLFWDropCallback(GLFWwindow* window, int count, const char** paths);
    static void GLFWWindowFocusCallback(GLFWwindow* window, int focused);

    // State
    GLFWwindow* m_window = nullptr;
    bool m_exitRequested = false;
    bool m_initialized = false;

    // Timing
    float m_deltaTime = 0.0f;
    std::chrono::high_resolution_clock::time_point m_lastFrameTime;

    // Window state
    int m_windowWidth = 0;
    int m_windowHeight = 0;
    float m_dpiScale = 1.0f;
};

} // namespace smith::core
```

### 5.2 Application Class Definition

```cpp
// include/core/application.h

#pragma once

#include "core/app_base.h"
#include <memory>

namespace smith {
    namespace config { class ConfigManager; }
    namespace agent { class AgentTracker; class AgentProviderRegistry; }
    namespace logging { class Logger; }
    namespace ui { class GridLayout; class MenuBar; class OutputLog; }
}

namespace smith::core {

/**
 * Application - AgentSmith main application singleton
 *
 * Inherits AppBase for framework functionality and adds:
 * - Configuration management
 * - Agent tracking and providers
 * - Logging infrastructure
 * - UI components (grid, menu, dialogs)
 */
class Application : public AppBase {
public:
    /**
     * Get singleton instance
     */
    static Application& Instance();

    /**
     * Convenience method to run the application
     */
    static int Main(int argc, char** argv);

    // === Subsystem Accessors ===

    config::ConfigManager& GetConfig();
    agent::AgentTracker& GetAgentTracker();
    agent::AgentProviderRegistry& GetProviderRegistry();
    logging::Logger& GetLogger();
    ui::GridLayout& GetGridLayout();

    // === Application State ===

    const std::string& GetExecutableDirectory() const { return m_executableDir; }
    const std::string& GetResourcesDirectory() const { return m_resourcesDir; }

protected:
    // AppBase overrides
    void OnStartUp() override;
    void OnUpdate(float deltaTime) override;
    void OnImGuiRender() override;
    void OnShutDown() override;
    void OnResize(int width, int height) override;
    void OnFileDrop(int count, const char** paths) override;

private:
    Application();
    ~Application();

    void InitializeLogging();
    void InitializeConfig();
    void InitializeAgentSystem();
    void InitializeUI();
    void RegisterDefaultProviders();
    void DiscoverPaths();

    // Subsystems (owned)
    std::unique_ptr<logging::Logger> m_logger;
    std::unique_ptr<config::ConfigManager> m_config;
    std::unique_ptr<agent::AgentTracker> m_agentTracker;
    std::unique_ptr<agent::AgentProviderRegistry> m_providerRegistry;
    std::unique_ptr<ui::GridLayout> m_gridLayout;
    std::unique_ptr<ui::MenuBar> m_menuBar;
    std::unique_ptr<ui::OutputLog> m_outputLog;

    // Paths
    std::string m_executableDir;
    std::string m_resourcesDir;
};

// Global accessor functions (convenience)
inline Application& GetApp() { return Application::Instance(); }
inline config::ConfigManager& GetConfig() { return Application::Instance().GetConfig(); }
inline logging::Logger& GetLogger() { return Application::Instance().GetLogger(); }

} // namespace smith::core
```

### 5.3 Entry Point

```cpp
// src/main.cpp

#include "core/application.h"

int main(int argc, char** argv) {
    return smith::core::Application::Main(argc, argv);
}

#ifdef _WIN32
#include <Windows.h>
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    (void)hInstance; (void)hPrevInstance; (void)lpCmdLine; (void)nCmdShow;
    return main(__argc, __argv);
}
#endif
```

---

## 6. Logging System

### 6.1 Log Levels and Categories

```cpp
// include/logging/log_macros.h

#pragma once

#include "logging/logger.h"

namespace smith::logging {

/**
 * Log verbosity levels (ascending severity)
 */
enum class LogLevel : int {
    Verbose = 0,   // Detailed debug information
    Log = 1,       // General information (default)
    Warning = 2,   // Potential issues
    Error = 3,     // Recoverable errors
    Fatal = 4      // Unrecoverable errors (may terminate)
};

} // namespace smith::logging

// === Primary Logging Macro ===
// Usage: SMITH_LOG(Warning, "MyCategory", "Value is %d", value);

#define SMITH_LOG(Level, Category, Format, ...) \
    ::smith::logging::Logger::Instance().Log( \
        ::smith::logging::LogLevel::Level, \
        Category, \
        __FILE__, \
        __LINE__, \
        Format, \
        ##__VA_ARGS__)

// === Convenience Macros ===

#define SMITH_VERBOSE(Category, Format, ...) SMITH_LOG(Verbose, Category, Format, ##__VA_ARGS__)
#define SMITH_INFO(Category, Format, ...)    SMITH_LOG(Log, Category, Format, ##__VA_ARGS__)
#define SMITH_WARN(Category, Format, ...)    SMITH_LOG(Warning, Category, Format, ##__VA_ARGS__)
#define SMITH_ERROR(Category, Format, ...)   SMITH_LOG(Error, Category, Format, ##__VA_ARGS__)
#define SMITH_FATAL(Category, Format, ...)   SMITH_LOG(Fatal, Category, Format, ##__VA_ARGS__)

// === Category Declaration (use in .cpp files) ===
// Usage: SMITH_DECLARE_LOG_CATEGORY(LogAgent);

#define SMITH_DECLARE_LOG_CATEGORY(CategoryName) \
    static constexpr const char* CategoryName = #CategoryName

// === Predefined Categories ===

namespace smith::logging::categories {
    constexpr const char* Core = "Core";
    constexpr const char* Config = "Config";
    constexpr const char* Agent = "Agent";
    constexpr const char* Terminal = "Terminal";
    constexpr const char* UI = "UI";
    constexpr const char* Platform = "Platform";
    constexpr const char* WebView = "WebView";
}
```

### 6.2 Logger Class

```cpp
// include/logging/logger.h

#pragma once

#include "log_macros.h"
#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <chrono>
#include <functional>

namespace smith::logging {

// Forward declarations
class ILogSink;

/**
 * Log entry structure passed to sinks
 */
struct LogEntry {
    LogLevel level;
    std::string category;
    std::string message;
    std::string file;
    int line;
    std::chrono::system_clock::time_point timestamp;
    std::thread::id threadId;
};

/**
 * Logger - Central logging facility
 *
 * Features:
 * - Multiple output sinks (file, console, ImGui)
 * - Category-based filtering
 * - Thread-safe
 * - Printf-style formatting
 */
class Logger {
public:
    static Logger& Instance();

    /**
     * Log a message (called by SMITH_LOG macro)
     */
    template<typename... Args>
    void Log(LogLevel level, const char* category, const char* file, int line,
             const char* format, Args&&... args);

    /**
     * Add a log sink
     */
    void AddSink(std::shared_ptr<ILogSink> sink);

    /**
     * Remove a specific sink
     */
    void RemoveSink(ILogSink* sink);

    /**
     * Set minimum log level (messages below this are ignored)
     */
    void SetMinLevel(LogLevel level) { m_minLevel = level; }
    LogLevel GetMinLevel() const { return m_minLevel; }

    /**
     * Set category filter (empty = all categories)
     * Comma-separated list: "Agent,Terminal,UI"
     */
    void SetCategoryFilter(const std::string& filter);

    /**
     * Check if a category passes the filter
     */
    bool IsCategoryEnabled(const std::string& category) const;

    /**
     * Flush all sinks
     */
    void Flush();

private:
    Logger() = default;
    ~Logger();

    void Dispatch(const LogEntry& entry);
    std::string FormatMessage(const char* format, ...);

    std::vector<std::shared_ptr<ILogSink>> m_sinks;
    mutable std::mutex m_mutex;
    LogLevel m_minLevel = LogLevel::Log;
    std::vector<std::string> m_enabledCategories;  // Empty = all
};

} // namespace smith::logging
```

### 6.3 Log Sinks

```cpp
// include/logging/log_sink.h

#pragma once

#include "logger.h"
#include <fstream>
#include <deque>

namespace smith::logging {

/**
 * ILogSink - Interface for log output destinations
 */
class ILogSink {
public:
    virtual ~ILogSink() = default;

    /**
     * Write a log entry
     */
    virtual void Write(const LogEntry& entry) = 0;

    /**
     * Flush any buffered output
     */
    virtual void Flush() {}

    /**
     * Get sink name (for debugging)
     */
    virtual const char* GetName() const = 0;
};

/**
 * FileSink - Writes logs to a file
 */
class FileSink : public ILogSink {
public:
    explicit FileSink(const std::string& filePath, bool append = false);
    ~FileSink() override;

    void Write(const LogEntry& entry) override;
    void Flush() override;
    const char* GetName() const override { return "FileSink"; }

private:
    std::ofstream m_file;
    std::mutex m_mutex;
};

/**
 * ConsoleSink - Writes logs to stdout/stderr
 */
class ConsoleSink : public ILogSink {
public:
    void Write(const LogEntry& entry) override;
    const char* GetName() const override { return "ConsoleSink"; }
};

/**
 * ImGuiSink - Stores logs for display in ImGui window
 */
class ImGuiSink : public ILogSink {
public:
    explicit ImGuiSink(size_t maxEntries = 1000);

    void Write(const LogEntry& entry) override;
    const char* GetName() const override { return "ImGuiSink"; }

    /**
     * Get stored entries for rendering
     */
    const std::deque<LogEntry>& GetEntries() const { return m_entries; }

    /**
     * Clear all stored entries
     */
    void Clear();

private:
    std::deque<LogEntry> m_entries;
    size_t m_maxEntries;
    mutable std::mutex m_mutex;
};

/**
 * CallbackSink - Invokes a callback for each log entry
 */
class CallbackSink : public ILogSink {
public:
    using Callback = std::function<void(const LogEntry&)>;

    explicit CallbackSink(Callback callback);

    void Write(const LogEntry& entry) override;
    const char* GetName() const override { return "CallbackSink"; }

private:
    Callback m_callback;
};

} // namespace smith::logging
```

---

## 7. Configuration System

### 7.1 Configuration Data Structures

```cpp
// include/config/app_config.h

#pragma once

#include <string>
#include <vector>
#include "agent/agent.h"
#include "terminal/terminal_theme.h"

namespace smith::config {

/**
 * Window configuration
 */
struct WindowConfig {
    int width = 1920;
    int height = 1080;
    bool maximized = false;
    bool vsync = true;
    int posX = -1;  // -1 = centered
    int posY = -1;
};

/**
 * Grid layout configuration
 */
struct GridConfig {
    int rows = 2;
    int cols = 2;
    int focusedRow = 0;
    int focusedCol = 0;
};

/**
 * Terminal configuration
 */
struct TerminalConfig {
    std::string defaultTheme = "Catppuccin Macchiato";
    std::string fontFamily = "JetBrains Mono";
    int fontSize = 14;
    bool useWebView = true;       // Use WebView2+xterm.js when available
    int scrollbackLines = 10000;
};

/**
 * Logging configuration
 */
struct LoggingConfig {
    std::string minLevel = "Log";       // Verbose, Log, Warning, Error, Fatal
    bool logToFile = true;
    std::string logFilePath = "agentsmith.log";
    bool logToConsole = false;
    std::string categoryFilter = "";     // Empty = all
};

/**
 * Agent configuration (for persistence)
 */
struct AgentConfig {
    std::string id;
    std::string name;
    std::string type;                    // "ClaudeCode", "Grok", "ChatGPT", etc.
    std::string workingDirectory;
    std::string command;
    std::vector<std::string> args;
    std::string terminalTheme;
    bool autoAcceptEdits = false;
    int gridRow = 0;
    int gridCol = 0;

    // API-specific (for Grok, ChatGPT)
    std::string apiKeyEnvVar;            // Environment variable name for API key
    std::string model;                   // e.g., "gpt-4", "grok-1"
};

/**
 * API provider configuration
 */
struct ApiProviderConfig {
    std::string name;
    std::string baseUrl;
    std::string apiKeyEnvVar;
    std::string defaultModel;
};

/**
 * Complete application configuration
 */
struct AppConfig {
    WindowConfig window;
    GridConfig grid;
    TerminalConfig terminal;
    LoggingConfig logging;
    std::vector<AgentConfig> agents;
    std::vector<ApiProviderConfig> apiProviders;

    // Runtime state (not persisted)
    bool showOutputLog = false;
    bool showImGuiDemo = false;
};

} // namespace smith::config
```

### 7.2 ConfigManager Class

```cpp
// include/config/config_manager.h

#pragma once

#include "app_config.h"
#include <string>
#include <functional>

namespace smith::config {

/**
 * ConfigManager - Handles loading and saving application configuration
 *
 * Features:
 * - JSON file persistence
 * - Default value handling
 * - Schema validation (basic)
 * - Change notification
 */
class ConfigManager {
public:
    using ChangeCallback = std::function<void(const AppConfig&)>;

    ConfigManager();
    ~ConfigManager();

    /**
     * Load configuration from file
     * @param filePath Path to config.json
     * @return True if loaded successfully (or defaults applied)
     */
    bool Load(const std::string& filePath);

    /**
     * Save configuration to file
     * @param filePath Path to config.json (empty = use loaded path)
     * @return True if saved successfully
     */
    bool Save(const std::string& filePath = "");

    /**
     * Reset to default configuration
     */
    void ResetToDefaults();

    /**
     * Get current configuration (const)
     */
    const AppConfig& Get() const { return m_config; }

    /**
     * Get mutable configuration reference
     * Call MarkDirty() after modifications
     */
    AppConfig& GetMutable() { return m_config; }

    /**
     * Mark configuration as modified (triggers save on next autosave)
     */
    void MarkDirty() { m_dirty = true; }

    /**
     * Check if configuration has unsaved changes
     */
    bool IsDirty() const { return m_dirty; }

    /**
     * Register callback for configuration changes
     */
    void OnChange(ChangeCallback callback);

    /**
     * Get the path configuration was loaded from
     */
    const std::string& GetFilePath() const { return m_filePath; }

private:
    void ApplyDefaults();
    void NotifyChange();

    AppConfig m_config;
    std::string m_filePath;
    bool m_dirty = false;
    std::vector<ChangeCallback> m_changeCallbacks;
};

} // namespace smith::config
```

---

## 8. Agent System

### 8.1 Agent Data Structure

```cpp
// include/agent/agent.h

#pragma once

#include <string>
#include <vector>
#include <chrono>
#include <optional>

namespace smith::agent {

/**
 * Agent status enumeration
 */
enum class AgentStatus {
    Idle,       // Ready but not active
    Running,    // Terminal/API active
    Waiting,    // Waiting for user input
    Thinking,   // AI processing (API call in progress)
    Error,      // Encountered an error
    Stopped     // Explicitly stopped
};

/**
 * Agent type enumeration
 */
enum class AgentType {
    ClaudeCode,  // Claude Code CLI (terminal)
    Grok,        // xAI Grok API
    ChatGPT,     // OpenAI ChatGPT API
    Cursor,      // Cursor IDE (terminal)
    Custom       // User-defined command (terminal)
};

/**
 * Git repository information
 */
struct GitInfo {
    std::string branch;
    std::string commit;           // Short hash
    int aheadCount = 0;
    int behindCount = 0;
    int uncommittedChanges = 0;
    bool isRepo = false;
    std::chrono::system_clock::time_point lastUpdated;
};

/**
 * Token usage and cost metrics
 */
struct TokenMetrics {
    int inputTokens = 0;
    int outputTokens = 0;
    int cacheReadTokens = 0;
    int cacheWriteTokens = 0;
    double totalCost = 0.0;           // USD
    std::string model;
    bool valid = false;
    std::chrono::system_clock::time_point lastUpdated;
};

/**
 * Rate limit information
 */
struct RateLimitInfo {
    int remaining = -1;               // -1 = unknown
    int limit = -1;
    std::chrono::system_clock::time_point resetsAt;
    bool valid = false;
};

/**
 * Complete agent metrics
 */
struct AgentMetrics {
    TokenMetrics tokens;
    RateLimitInfo rateLimit;

    // Session statistics
    int messageCount = 0;
    int toolCallCount = 0;
    std::chrono::seconds sessionDuration{0};
};

/**
 * Agent - Represents a single AI agent instance
 *
 * Combines identity, configuration, state, and metrics.
 */
struct Agent {
    // === Identity (immutable after creation) ===
    std::string id;                   // UUID
    std::string name;                 // Display name
    AgentType type;

    // === Configuration ===
    std::string workingDirectory;
    std::string command;              // Executable path/name
    std::vector<std::string> args;    // Command arguments
    std::string terminalTheme;        // Theme name (empty = default)
    bool autoAcceptEdits = false;

    // API-specific configuration
    std::string apiKeyEnvVar;         // Env var for API key
    std::string model;                // Model name
    std::string apiBaseUrl;           // Custom API endpoint

    // === Runtime State ===
    AgentStatus status = AgentStatus::Idle;
    std::string statusMessage;        // Detailed status text
    int processId = -1;               // OS process ID (terminal agents)
    bool needsAttention = false;      // Requires user action

    // === Session Tracking ===
    std::chrono::system_clock::time_point sessionStart;
    std::chrono::system_clock::time_point lastActivity;

    // === Metrics ===
    AgentMetrics metrics;

    // === Git Info ===
    GitInfo gitInfo;

    // === Grid Position ===
    int gridRow = 0;
    int gridCol = 0;

    // === Factory Methods ===
    static Agent Create(const std::string& name,
                       const std::string& workDir,
                       AgentType type);

    static Agent CreateFromConfig(const config::AgentConfig& config);

    // === Utilities ===
    std::string GetTypeString() const;
    static AgentType ParseType(const std::string& typeStr);
    bool RequiresTerminal() const;
    bool RequiresApiKey() const;
    std::chrono::seconds GetSessionDuration() const;
};

} // namespace smith::agent
```

### 8.2 Agent Provider Interface

```cpp
// include/agent/agent_provider.h

#pragma once

#include "agent.h"
#include <string>
#include <optional>
#include <functional>
#include <memory>

namespace smith::agent {

/**
 * IAgentProvider - Abstract interface for agent implementations
 *
 * Each agent type (Claude, Grok, ChatGPT, etc.) implements this interface
 * to provide type-specific behavior:
 * - Command/connection setup
 * - I/O processing and parsing
 * - Metrics extraction
 * - Status detection
 */
class IAgentProvider {
public:
    virtual ~IAgentProvider() = default;

    // === Provider Identity ===

    /**
     * Get human-readable provider name
     */
    virtual std::string GetName() const = 0;

    /**
     * Get agent type enum
     */
    virtual AgentType GetType() const = 0;

    /**
     * Get default command for this provider
     */
    virtual std::string GetDefaultCommand() const = 0;

    /**
     * Get default command arguments
     */
    virtual std::vector<std::string> GetDefaultArgs() const { return {}; }

    // === Capabilities ===

    /**
     * Does this provider use a terminal (ConPTY)?
     * False for API-based providers (Grok, ChatGPT)
     */
    virtual bool RequiresTerminal() const { return true; }

    /**
     * Does this provider require an API key?
     */
    virtual bool RequiresApiKey() const { return false; }

    /**
     * Get environment variable name for API key
     */
    virtual std::string GetApiKeyEnvVar() const { return ""; }

    /**
     * Get default model for API providers
     */
    virtual std::string GetDefaultModel() const { return ""; }

    // === Lifecycle Hooks ===

    /**
     * Called when provider is attached to an agent
     */
    virtual void OnAttach(Agent& agent) { (void)agent; }

    /**
     * Called when provider is detached from an agent
     */
    virtual void OnDetach(Agent& agent) { (void)agent; }

    /**
     * Called when agent starts (terminal launched or API connected)
     */
    virtual void OnStart(Agent& agent) { (void)agent; }

    /**
     * Called when agent stops
     */
    virtual void OnStop(Agent& agent) { (void)agent; }

    // === I/O Processing (Terminal Providers) ===

    /**
     * Process input before sending to terminal
     * @param input User input
     * @return Modified input, or empty to block
     */
    virtual std::string ProcessInput(const std::string& input) { return input; }

    /**
     * Process output received from terminal
     * @param output Raw terminal output
     * @return Modified output, or empty to suppress display
     */
    virtual std::string ProcessOutput(const std::string& output) { return output; }

    // === Metrics Extraction ===

    /**
     * Parse terminal output for metrics (tokens, cost, etc.)
     * @param output Terminal output to parse
     * @param metrics Metrics structure to update
     * @return True if metrics were found and updated
     */
    virtual bool ParseMetrics(const std::string& output, AgentMetrics& metrics) {
        (void)output; (void)metrics;
        return false;
    }

    /**
     * Get command to request metrics from agent
     * e.g., "/cost" for Claude Code
     */
    virtual std::string GetMetricsCommand() const { return ""; }

    /**
     * How often to auto-request metrics (0 = never)
     */
    virtual std::chrono::seconds GetMetricsRefreshInterval() const {
        return std::chrono::seconds(0);
    }

    // === Status Detection ===

    /**
     * Analyze output to detect agent status changes
     * @param output Terminal output to analyze
     * @return New status if detected, nullopt otherwise
     */
    virtual std::optional<AgentStatus> DetectStatus(const std::string& output) {
        (void)output;
        return std::nullopt;
    }

    /**
     * Check if output indicates agent needs attention
     */
    virtual bool DetectNeedsAttention(const std::string& output) {
        (void)output;
        return false;
    }

    // === API Providers (Non-Terminal) ===

    /**
     * Send a message to API-based agent
     * (Only for providers where RequiresTerminal() returns false)
     */
    virtual void SendMessage(Agent& agent, const std::string& message,
                            std::function<void(const std::string&)> onResponse) {
        (void)agent; (void)message; (void)onResponse;
    }

    /**
     * Cancel in-progress API request
     */
    virtual void CancelRequest(Agent& agent) { (void)agent; }
};

// === Type alias for provider factory ===
using ProviderFactory = std::function<std::unique_ptr<IAgentProvider>()>;

} // namespace smith::agent
```

### 8.3 Concrete Provider Examples

```cpp
// include/agent/providers/claude_provider.h

#pragma once

#include "agent/agent_provider.h"
#include <regex>

namespace smith::agent {

/**
 * ClaudeAgentProvider - Provider for Claude Code CLI
 *
 * Features:
 * - Parses /cost output for token metrics
 * - Detects thinking/response states
 * - Identifies when user input is needed
 */
class ClaudeAgentProvider : public IAgentProvider {
public:
    // Identity
    std::string GetName() const override { return "Claude Code"; }
    AgentType GetType() const override { return AgentType::ClaudeCode; }
    std::string GetDefaultCommand() const override { return "claude"; }

    // Lifecycle
    void OnStart(Agent& agent) override;

    // I/O Processing
    std::string ProcessOutput(const std::string& output) override;

    // Metrics
    bool ParseMetrics(const std::string& output, AgentMetrics& metrics) override;
    std::string GetMetricsCommand() const override { return "/cost"; }
    std::chrono::seconds GetMetricsRefreshInterval() const override {
        return std::chrono::seconds(300);  // 5 minutes
    }

    // Status Detection
    std::optional<AgentStatus> DetectStatus(const std::string& output) override;
    bool DetectNeedsAttention(const std::string& output) override;

private:
    // Parsing helpers
    bool ParseCostOutput(const std::string& output, AgentMetrics& metrics);
    bool IsThinkingIndicator(const std::string& line);
    bool IsPromptIndicator(const std::string& line);
    bool IsErrorIndicator(const std::string& line);

    // Regex patterns (compiled once)
    static const std::regex s_costPattern;
    static const std::regex s_tokenPattern;
    static const std::regex s_modelPattern;
};

} // namespace smith::agent
```

```cpp
// include/agent/providers/grok_provider.h

#pragma once

#include "agent/agent_provider.h"
#include <thread>
#include <atomic>

namespace smith::agent {

/**
 * GrokAgentProvider - Provider for xAI Grok API
 *
 * Non-terminal provider that uses HTTP API calls.
 * Requires GROK_API_KEY environment variable.
 */
class GrokAgentProvider : public IAgentProvider {
public:
    // Identity
    std::string GetName() const override { return "Grok"; }
    AgentType GetType() const override { return AgentType::Grok; }
    std::string GetDefaultCommand() const override { return ""; }  // No terminal

    // Capabilities
    bool RequiresTerminal() const override { return false; }
    bool RequiresApiKey() const override { return true; }
    std::string GetApiKeyEnvVar() const override { return "GROK_API_KEY"; }
    std::string GetDefaultModel() const override { return "grok-2"; }

    // Lifecycle
    void OnAttach(Agent& agent) override;
    void OnDetach(Agent& agent) override;

    // API Methods
    void SendMessage(Agent& agent, const std::string& message,
                    std::function<void(const std::string&)> onResponse) override;
    void CancelRequest(Agent& agent) override;

private:
    std::string BuildRequestBody(const std::string& message, const std::string& model);
    void ProcessApiResponse(const std::string& response, Agent& agent,
                           std::function<void(const std::string&)> onResponse);

    std::atomic<bool> m_cancelRequested{false};
    std::thread m_requestThread;
};

} // namespace smith::agent
```

```cpp
// include/agent/providers/chatgpt_provider.h

#pragma once

#include "agent/agent_provider.h"

namespace smith::agent {

/**
 * ChatGPTAgentProvider - Provider for OpenAI ChatGPT API
 *
 * Non-terminal provider that uses HTTP API calls.
 * Requires OPENAI_API_KEY environment variable.
 */
class ChatGPTAgentProvider : public IAgentProvider {
public:
    // Identity
    std::string GetName() const override { return "ChatGPT"; }
    AgentType GetType() const override { return AgentType::ChatGPT; }
    std::string GetDefaultCommand() const override { return ""; }

    // Capabilities
    bool RequiresTerminal() const override { return false; }
    bool RequiresApiKey() const override { return true; }
    std::string GetApiKeyEnvVar() const override { return "OPENAI_API_KEY"; }
    std::string GetDefaultModel() const override { return "gpt-4-turbo"; }

    // API Methods
    void SendMessage(Agent& agent, const std::string& message,
                    std::function<void(const std::string&)> onResponse) override;
    void CancelRequest(Agent& agent) override;

private:
    // OpenAI-specific implementation
};

} // namespace smith::agent
```

### 8.4 Agent Provider Registry

```cpp
// include/agent/agent_registry.h

#pragma once

#include "agent_provider.h"
#include <unordered_map>
#include <memory>

namespace smith::agent {

/**
 * AgentProviderRegistry - Singleton registry for agent providers
 *
 * Manages provider instances and factory functions.
 * Providers are registered at startup and looked up by type or name.
 */
class AgentProviderRegistry {
public:
    static AgentProviderRegistry& Instance();

    /**
     * Register a provider factory
     * @param type Agent type this factory creates
     * @param factory Function that creates provider instances
     */
    void RegisterProvider(AgentType type, ProviderFactory factory);

    /**
     * Get or create a provider for the given type
     * @param type Agent type
     * @return Provider instance (owned by registry), or nullptr if not registered
     */
    IAgentProvider* GetProvider(AgentType type);

    /**
     * Get provider by name
     * @param name Provider name (e.g., "Claude Code")
     * @return Provider instance or nullptr
     */
    IAgentProvider* GetProviderByName(const std::string& name);

    /**
     * Get all registered provider types
     */
    std::vector<AgentType> GetRegisteredTypes() const;

    /**
     * Get all provider names
     */
    std::vector<std::string> GetProviderNames() const;

    /**
     * Check if a provider is registered
     */
    bool HasProvider(AgentType type) const;

private:
    AgentProviderRegistry() = default;

    std::unordered_map<AgentType, ProviderFactory> m_factories;
    std::unordered_map<AgentType, std::unique_ptr<IAgentProvider>> m_providers;
};

} // namespace smith::agent
```

### 8.5 Agent Tracker

```cpp
// include/agent/agent_tracker.h

#pragma once

#include "agent.h"
#include <vector>
#include <memory>
#include <functional>
#include <mutex>

namespace smith::agent {

/**
 * AgentTracker - Manages lifecycle of all agent instances
 *
 * Responsibilities:
 * - Create/destroy agents
 * - Lookup agents by ID or grid position
 * - Persist agents to configuration
 * - Background updates (git info, metrics refresh)
 */
class AgentTracker {
public:
    using AgentCallback = std::function<void(Agent*)>;

    AgentTracker();
    ~AgentTracker();

    // === Agent Lifecycle ===

    /**
     * Create a new agent
     * @return Pointer to created agent (owned by tracker)
     */
    Agent* CreateAgent(const std::string& name,
                      const std::string& workingDirectory,
                      AgentType type);

    /**
     * Create agent from configuration
     */
    Agent* CreateAgentFromConfig(const config::AgentConfig& config);

    /**
     * Remove an agent by ID
     * @return True if agent was found and removed
     */
    bool RemoveAgent(const std::string& id);

    /**
     * Remove all agents
     */
    void RemoveAllAgents();

    // === Lookup ===

    /**
     * Get agent by ID
     */
    Agent* GetAgent(const std::string& id);
    const Agent* GetAgent(const std::string& id) const;

    /**
     * Get agent at grid position
     */
    Agent* GetAgentAt(int row, int col);
    const Agent* GetAgentAt(int row, int col) const;

    /**
     * Get all agents
     */
    std::vector<Agent*> GetAgents();
    std::vector<const Agent*> GetAgents() const;

    /**
     * Get agent count
     */
    size_t GetAgentCount() const { return m_agents.size(); }

    // === Updates ===

    /**
     * Update all agents (call each frame)
     * Handles periodic git refresh, metrics collection, etc.
     */
    void Update(float deltaTime);

    /**
     * Force refresh git info for all agents
     */
    void RefreshAllGitInfo();

    /**
     * Force refresh metrics for all agents
     */
    void RefreshAllMetrics();

    // === Events ===

    void OnAgentCreated(AgentCallback callback);
    void OnAgentRemoved(AgentCallback callback);
    void OnAgentStatusChanged(AgentCallback callback);

    // === Persistence ===

    /**
     * Load agents from config
     */
    void LoadFromConfig(const std::vector<config::AgentConfig>& configs);

    /**
     * Save agents to config format
     */
    std::vector<config::AgentConfig> SaveToConfig() const;

private:
    void NotifyCreated(Agent* agent);
    void NotifyRemoved(Agent* agent);
    void NotifyStatusChanged(Agent* agent);
    void UpdateGitInfo(Agent* agent);
    void UpdateMetrics(Agent* agent);

    std::vector<std::unique_ptr<Agent>> m_agents;
    mutable std::mutex m_mutex;

    // Callbacks
    std::vector<AgentCallback> m_onCreated;
    std::vector<AgentCallback> m_onRemoved;
    std::vector<AgentCallback> m_onStatusChanged;

    // Update timers
    float m_gitRefreshTimer = 0.0f;
    float m_metricsRefreshTimer = 0.0f;
    static constexpr float GIT_REFRESH_INTERVAL = 30.0f;  // seconds
};

} // namespace smith::agent
```

### 8.6 Agent Utilities

```cpp
// include/agent/agent_utils.h

#pragma once

#include "agent.h"
#include <string>
#include <memory>

namespace smith::agent {

/**
 * IMetricParser - Interface for agent-specific metric extraction
 */
class IMetricParser {
public:
    virtual ~IMetricParser() = default;

    /**
     * Parse output text for metrics
     * @param output Text to parse (terminal output or API response)
     * @param metrics Metrics to update
     * @return True if any metrics were found
     */
    virtual bool Parse(const std::string& output, AgentMetrics& metrics) = 0;
};

/**
 * ClaudeMetricParser - Parses Claude Code /cost output
 */
class ClaudeMetricParser : public IMetricParser {
public:
    bool Parse(const std::string& output, AgentMetrics& metrics) override;

private:
    bool ParseCostLine(const std::string& line, TokenMetrics& tokens);
    bool ParseTokenLine(const std::string& line, TokenMetrics& tokens);
    bool ParseModelLine(const std::string& line, TokenMetrics& tokens);
};

/**
 * OpenAIMetricParser - Parses OpenAI API response usage
 */
class OpenAIMetricParser : public IMetricParser {
public:
    bool Parse(const std::string& jsonResponse, AgentMetrics& metrics) override;
};

/**
 * XAIMetricParser - Parses xAI (Grok) API response usage
 */
class XAIMetricParser : public IMetricParser {
public:
    bool Parse(const std::string& jsonResponse, AgentMetrics& metrics) override;
};

/**
 * AgentUtils - Static utility functions for agents
 */
class AgentUtils {
public:
    /**
     * Generate a new agent UUID
     */
    static std::string GenerateId();

    /**
     * Get display string for agent type
     */
    static std::string TypeToString(AgentType type);

    /**
     * Parse agent type from string
     */
    static AgentType StringToType(const std::string& str);

    /**
     * Get display string for agent status
     */
    static std::string StatusToString(AgentStatus status);

    /**
     * Format duration for display (e.g., "2h 15m")
     */
    static std::string FormatDuration(std::chrono::seconds duration);

    /**
     * Format cost for display (e.g., "$0.42")
     */
    static std::string FormatCost(double cost);

    /**
     * Format token count for display (e.g., "45.2k")
     */
    static std::string FormatTokenCount(int tokens);

    /**
     * Check if API key is available for provider
     */
    static bool HasApiKey(const std::string& envVarName);

    /**
     * Get API key from environment variable
     */
    static std::string GetApiKey(const std::string& envVarName);

    /**
     * Create metric parser for agent type
     */
    static std::unique_ptr<IMetricParser> CreateMetricParser(AgentType type);
};

} // namespace smith::agent
```

---

## 9. Terminal System

### 9.1 Terminal Interface

```cpp
// include/terminal/terminal_interface.h

#pragma once

#include "terminal_theme.h"
#include <string>
#include <vector>
#include <functional>

#ifdef _WIN32
#include <Windows.h>
#endif

struct ImVec2;

namespace smith::terminal {

/**
 * Terminal output callback signature
 * @param data Output data bytes
 * @param length Number of bytes
 */
using OutputCallback = std::function<void(const char* data, size_t length)>;

/**
 * Terminal exit callback signature
 * @param exitCode Process exit code
 */
using ExitCallback = std::function<void(int exitCode)>;

/**
 * ITerminal - Abstract interface for terminal implementations
 *
 * Implementations:
 * - ConPTYTerminal: Windows pseudo-console with custom rendering
 * - WebViewTerminal: WebView2 + xterm.js for native-quality rendering
 *
 * Both implementations use ConPTY for process management; they differ
 * in how terminal output is rendered.
 */
class ITerminal {
public:
    virtual ~ITerminal() = default;

    // === Lifecycle ===

    /**
     * Launch a process in the terminal
     * @param command Executable path/name
     * @param args Command arguments
     * @param workingDirectory Starting directory
     * @param cols Initial column count
     * @param rows Initial row count
     * @return True if launched successfully
     */
    virtual bool Launch(const std::string& command,
                       const std::vector<std::string>& args,
                       const std::string& workingDirectory,
                       int cols, int rows) = 0;

    /**
     * Terminate the terminal process
     */
    virtual void Terminate() = 0;

    /**
     * Check if process is running
     */
    virtual bool IsRunning() const = 0;

    /**
     * Get process exit code (valid after IsRunning() returns false)
     */
    virtual int GetExitCode() const = 0;

    // === I/O ===

    /**
     * Write data to terminal input
     * @param data Data to write
     */
    virtual void Write(const std::string& data) = 0;

    /**
     * Write raw bytes to terminal input
     * @param data Byte array
     * @param length Number of bytes
     */
    virtual void Write(const char* data, size_t length) = 0;

    /**
     * Set callback for terminal output
     */
    virtual void SetOutputCallback(OutputCallback callback) = 0;

    /**
     * Set callback for process exit
     */
    virtual void SetExitCallback(ExitCallback callback) = 0;

    // === Sizing ===

    /**
     * Resize terminal
     * @param cols New column count
     * @param rows New row count
     */
    virtual void Resize(int cols, int rows) = 0;

    /**
     * Get current column count
     */
    virtual int GetCols() const = 0;

    /**
     * Get current row count
     */
    virtual int GetRows() const = 0;

    // === Rendering ===

    /**
     * Render terminal content (for ImGui-based terminals)
     * @param position Top-left position in window
     * @param size Available size
     */
    virtual void Render(const ImVec2& position, const ImVec2& size) {
        (void)position; (void)size;
    }

    /**
     * Get native window handle (for WebView-based terminals)
     */
#ifdef _WIN32
    virtual HWND GetNativeHandle() const { return nullptr; }
#else
    virtual void* GetNativeHandle() const { return nullptr; }
#endif

    /**
     * Set position and size of native window
     */
    virtual void SetBounds(int x, int y, int width, int height) {
        (void)x; (void)y; (void)width; (void)height;
    }

    /**
     * Show or hide the terminal
     */
    virtual void SetVisible(bool visible) { (void)visible; }

    // === Capabilities ===

    /**
     * Does this terminal render via native window (WebView)?
     */
    virtual bool HasNativeRendering() const { return false; }

    /**
     * Does this terminal support text selection?
     */
    virtual bool SupportsSelection() const { return false; }

    /**
     * Get currently selected text
     */
    virtual std::string GetSelectedText() const { return ""; }

    /**
     * Clear selection
     */
    virtual void ClearSelection() {}

    // === Theme ===

    /**
     * Apply terminal color theme
     */
    virtual void SetTheme(const TerminalTheme& theme) = 0;

    /**
     * Get current theme
     */
    virtual const TerminalTheme& GetTheme() const = 0;

    // === Scrolling ===

    /**
     * Scroll by delta lines (negative = up, positive = down)
     */
    virtual void Scroll(int deltaLines) { (void)deltaLines; }

    /**
     * Scroll to bottom (most recent output)
     */
    virtual void ScrollToBottom() {}

    /**
     * Get current scroll position (0 = bottom)
     */
    virtual int GetScrollPosition() const { return 0; }

    /**
     * Get total scrollback lines available
     */
    virtual int GetScrollbackLines() const { return 0; }
};

} // namespace smith::terminal
```

### 9.2 Terminal Theme

```cpp
// include/terminal/terminal_theme.h

#pragma once

#include <string>
#include <cstdint>
#include <vector>

namespace smith::terminal {

/**
 * TerminalTheme - Color scheme for terminal rendering
 */
struct TerminalTheme {
    std::string name;

    // Basic colors (ARGB format: 0xAARRGGBB)
    uint32_t background = 0xFF1E1E2E;
    uint32_t foreground = 0xFFCDD6F4;
    uint32_t cursorColor = 0xFFF5E0DC;
    uint32_t selectionBackground = 0x80585B70;

    // ANSI colors (indices 0-15)
    // 0-7: Normal colors (black, red, green, yellow, blue, magenta, cyan, white)
    // 8-15: Bright variants
    uint32_t ansiColors[16] = {
        0xFF45475A, 0xFFF38BA8, 0xFFA6E3A1, 0xFFF9E2AF,  // 0-3
        0xFF89B4FA, 0xFFF5C2E7, 0xFF94E2D5, 0xFFBAC2DE,  // 4-7
        0xFF585B70, 0xFFF38BA8, 0xFFA6E3A1, 0xFFF9E2AF,  // 8-11
        0xFF89B4FA, 0xFFF5C2E7, 0xFF94E2D5, 0xFFA6ADC8   // 12-15
    };

    /**
     * Get ANSI color by index (0-15)
     */
    uint32_t GetAnsiColor(int index) const {
        return (index >= 0 && index < 16) ? ansiColors[index] : foreground;
    }
};

/**
 * TerminalThemeManager - Manages available themes
 */
class TerminalThemeManager {
public:
    static TerminalThemeManager& Instance();

    /**
     * Load themes from JSON file
     */
    bool LoadThemes(const std::string& filePath);

    /**
     * Get theme by name
     */
    const TerminalTheme* GetTheme(const std::string& name) const;

    /**
     * Get default theme
     */
    const TerminalTheme& GetDefaultTheme() const;

    /**
     * Get all available theme names
     */
    std::vector<std::string> GetThemeNames() const;

    /**
     * Add or replace a theme
     */
    void AddTheme(const TerminalTheme& theme);

private:
    TerminalThemeManager();
    void LoadBuiltinThemes();

    std::vector<TerminalTheme> m_themes;
    std::string m_defaultThemeName = "Catppuccin Macchiato";
};

} // namespace smith::terminal
```

### 9.3 WebView Terminal Implementation

```cpp
// include/terminal/webview_terminal.h

#pragma once

#include "terminal_interface.h"
#include "conpty_terminal.h"
#include <memory>
#include <atomic>
#include <queue>
#include <mutex>

#ifdef _WIN32
#include <wrl.h>
#include <WebView2.h>
#endif

namespace smith::terminal {

/**
 * WebViewTerminal - Terminal using WebView2 + xterm.js
 *
 * Architecture:
 * 1. ConPTY handles process I/O (same as ConPTYTerminal)
 * 2. WebView2 hosts xterm.js for rendering
 * 3. JavaScript bridge connects ConPTY output to xterm.js
 *
 * Benefits:
 * - Perfect ANSI escape sequence support
 * - Smooth scrolling
 * - Native text selection
 * - Better Unicode support
 */
class WebViewTerminal : public ITerminal {
public:
    WebViewTerminal();
    ~WebViewTerminal() override;

    // === ITerminal Implementation ===

    bool Launch(const std::string& command,
               const std::vector<std::string>& args,
               const std::string& workingDirectory,
               int cols, int rows) override;

    void Terminate() override;
    bool IsRunning() const override;
    int GetExitCode() const override;

    void Write(const std::string& data) override;
    void Write(const char* data, size_t length) override;
    void SetOutputCallback(OutputCallback callback) override;
    void SetExitCallback(ExitCallback callback) override;

    void Resize(int cols, int rows) override;
    int GetCols() const override { return m_cols; }
    int GetRows() const override { return m_rows; }

#ifdef _WIN32
    HWND GetNativeHandle() const override { return m_hwnd; }
#endif
    void SetBounds(int x, int y, int width, int height) override;
    void SetVisible(bool visible) override;

    bool HasNativeRendering() const override { return true; }
    bool SupportsSelection() const override { return true; }
    std::string GetSelectedText() const override;
    void ClearSelection() override;

    void SetTheme(const TerminalTheme& theme) override;
    const TerminalTheme& GetTheme() const override { return m_theme; }

    void Scroll(int deltaLines) override;
    void ScrollToBottom() override;

    // === WebView-Specific ===

    /**
     * Initialize WebView2 with parent window
     * Must be called before Launch()
     */
    bool Initialize(HWND parentHwnd, const std::string& resourcesPath);

    /**
     * Check if WebView2 is ready
     */
    bool IsWebViewReady() const { return m_webViewReady; }

    /**
     * Check if WebView2 runtime is available on system
     */
    static bool IsWebView2Available();

private:
    // WebView2 initialization
    void CreateWebView(HWND parentHwnd);
    void OnWebViewCreated();
    void LoadTerminalPage();

    // JavaScript bridge
    void SendToXterm(const std::string& data);
    void ExecuteJavaScript(const std::string& script);
    void OnWebMessage(const std::wstring& message);

    // ConPTY output handler
    void OnPtyOutput(const char* data, size_t length);

    // ConPTY for process management
    std::unique_ptr<ConPTYTerminal> m_pty;

#ifdef _WIN32
    // WebView2 components
    HWND m_hwnd = nullptr;
    Microsoft::WRL::ComPtr<ICoreWebView2Controller> m_controller;
    Microsoft::WRL::ComPtr<ICoreWebView2> m_webView;
    Microsoft::WRL::ComPtr<ICoreWebView2Environment> m_environment;
#endif

    // State
    int m_cols = 80;
    int m_rows = 24;
    std::atomic<bool> m_webViewReady{false};
    TerminalTheme m_theme;
    std::string m_resourcesPath;

    // Buffering (for output before WebView ready)
    std::queue<std::string> m_pendingOutput;
    std::mutex m_pendingMutex;

    // Callbacks
    OutputCallback m_outputCallback;
    ExitCallback m_exitCallback;
};

} // namespace smith::terminal
```

### 9.4 Terminal Factory

```cpp
// include/terminal/terminal_factory.h

#pragma once

#include "terminal_interface.h"
#include <memory>

namespace smith::terminal {

/**
 * Terminal backend preference
 */
enum class TerminalBackend {
    Auto,       // Use WebView2 if available, else ImGui
    WebView2,   // Force WebView2 + xterm.js
    ImGui       // Force custom ImGui rendering
};

/**
 * TerminalFactory - Creates terminal instances
 */
class TerminalFactory {
public:
    /**
     * Create a terminal with specified backend
     * @param backend Preferred backend
     * @param parentHwnd Parent window handle (required for WebView2)
     * @param resourcesPath Path to web resources (required for WebView2)
     * @return Terminal instance
     */
    static std::unique_ptr<ITerminal> Create(
        TerminalBackend backend = TerminalBackend::Auto,
        void* parentHwnd = nullptr,
        const std::string& resourcesPath = "");

    /**
     * Get the recommended backend for current system
     */
    static TerminalBackend GetRecommendedBackend();

    /**
     * Check if WebView2 is available
     */
    static bool IsWebView2Available();
};

} // namespace smith::terminal
```

### 9.5 xterm.js Resources

**resources/web/terminal.html:**
```html
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <link rel="stylesheet" href="xterm.css">
    <link rel="stylesheet" href="terminal.css">
</head>
<body>
    <div id="terminal-container">
        <div id="terminal"></div>
    </div>
    <script src="xterm.js"></script>
    <script src="xterm-addon-fit.js"></script>
    <script src="xterm-addon-web-links.js"></script>
    <script src="terminal.js"></script>
</body>
</html>
```

**resources/web/terminal.css:**
```css
html, body {
    margin: 0;
    padding: 0;
    height: 100%;
    overflow: hidden;
    background: transparent;
}

#terminal-container {
    width: 100%;
    height: 100%;
    display: flex;
}

#terminal {
    flex: 1;
}

/* Hide scrollbar (we handle scrolling ourselves) */
.xterm-viewport::-webkit-scrollbar {
    display: none;
}
```

**resources/web/terminal.js:**
```javascript
// Terminal instance
let term = null;
let fitAddon = null;

// Initialize terminal
function initTerminal() {
    term = new Terminal({
        cursorBlink: true,
        cursorStyle: 'block',
        fontFamily: '"JetBrains Mono", "Cascadia Code", Consolas, monospace',
        fontSize: 14,
        lineHeight: 1.2,
        scrollback: 10000,
        allowProposedApi: true
    });

    fitAddon = new FitAddon.FitAddon();
    const webLinksAddon = new WebLinksAddon.WebLinksAddon();

    term.loadAddon(fitAddon);
    term.loadAddon(webLinksAddon);

    term.open(document.getElementById('terminal'));
    fitAddon.fit();

    // Send input to C++ (ConPTY)
    term.onData(data => {
        sendMessage({ type: 'input', data: btoa(data) });
    });

    // Handle resize
    term.onResize(size => {
        sendMessage({
            type: 'resize',
            cols: size.cols,
            rows: size.rows
        });
    });

    // Notify C++ that terminal is ready
    sendMessage({ type: 'ready', cols: term.cols, rows: term.rows });
}

// Receive messages from C++
window.chrome.webview.addEventListener('message', event => {
    const msg = JSON.parse(event.data);

    switch (msg.type) {
        case 'output':
            // Base64 decode and write to terminal
            term.write(atob(msg.data));
            break;

        case 'theme':
            applyTheme(msg.theme);
            break;

        case 'resize':
            term.resize(msg.cols, msg.rows);
            break;

        case 'clear':
            term.clear();
            break;

        case 'focus':
            term.focus();
            break;

        case 'getSelection':
            sendMessage({
                type: 'selection',
                text: term.getSelection()
            });
            break;

        case 'clearSelection':
            term.clearSelection();
            break;

        case 'scrollToBottom':
            term.scrollToBottom();
            break;
    }
});

// Send message to C++
function sendMessage(msg) {
    window.chrome.webview.postMessage(JSON.stringify(msg));
}

// Apply theme colors
function applyTheme(theme) {
    term.options.theme = {
        background: theme.background,
        foreground: theme.foreground,
        cursor: theme.cursor,
        selectionBackground: theme.selection,
        black: theme.ansi[0],
        red: theme.ansi[1],
        green: theme.ansi[2],
        yellow: theme.ansi[3],
        blue: theme.ansi[4],
        magenta: theme.ansi[5],
        cyan: theme.ansi[6],
        white: theme.ansi[7],
        brightBlack: theme.ansi[8],
        brightRed: theme.ansi[9],
        brightGreen: theme.ansi[10],
        brightYellow: theme.ansi[11],
        brightBlue: theme.ansi[12],
        brightMagenta: theme.ansi[13],
        brightCyan: theme.ansi[14],
        brightWhite: theme.ansi[15]
    };
}

// Handle window resize
window.addEventListener('resize', () => {
    if (fitAddon) {
        fitAddon.fit();
    }
});

// Initialize on load
document.addEventListener('DOMContentLoaded', initTerminal);
```

---

## 10. UI System

### 10.1 ImGui Layer

```cpp
// include/ui/imgui_layer.h

#pragma once

#include <string>

struct GLFWwindow;

namespace smith::ui {

/**
 * ImGuiLayer - Manages ImGui initialization and rendering
 */
class ImGuiLayer {
public:
    /**
     * Initialize ImGui with GLFW/OpenGL backend
     */
    static bool Initialize(GLFWwindow* window, const std::string& iniPath = "");

    /**
     * Shutdown ImGui
     */
    static void Shutdown();

    /**
     * Begin new ImGui frame
     */
    static void BeginFrame();

    /**
     * End ImGui frame and render
     */
    static void EndFrame();

    /**
     * Load fonts with custom size and fallbacks
     */
    static void LoadFonts(float sizePixels, const std::string& fontPath = "");

    /**
     * Apply AgentSmith dark theme
     */
    static void ApplyTheme();

    /**
     * Check if ImGui wants to capture keyboard
     */
    static bool WantsCaptureKeyboard();

    /**
     * Check if ImGui wants to capture mouse
     */
    static bool WantsCaptureMouse();
};

} // namespace smith::ui
```

### 10.2 Grid Layout

```cpp
// include/ui/grid_layout.h

#pragma once

#include <vector>
#include <memory>
#include <functional>

namespace smith {
    namespace agent { struct Agent; }
}

namespace smith::ui {

class AgentWindow;

/**
 * GridLayout - Manages grid of agent windows
 *
 * Features:
 * - Configurable grid size (1x1 to 4x4)
 * - Focus management with keyboard shortcuts
 * - Fullscreen mode for single agent
 * - Empty slot handling
 */
class GridLayout {
public:
    using EmptySlotCallback = std::function<void(int row, int col)>;

    GridLayout();
    ~GridLayout();

    /**
     * Set grid dimensions
     */
    void SetGridSize(int rows, int cols);

    /**
     * Get current grid dimensions
     */
    int GetRows() const { return m_rows; }
    int GetCols() const { return m_cols; }

    /**
     * Assign agent to grid slot
     */
    void SetAgent(int row, int col, agent::Agent* agent);

    /**
     * Remove agent from grid (doesn't delete agent)
     */
    void RemoveAgent(int row, int col);

    /**
     * Get agent at position
     */
    agent::Agent* GetAgent(int row, int col);

    /**
     * Get AgentWindow at position
     */
    AgentWindow* GetWindow(int row, int col);

    /**
     * Render the grid
     */
    void Render();

    /**
     * Handle keyboard input
     */
    void HandleInput();

    // === Focus Management ===

    void SetFocus(int row, int col);
    void FocusNext();
    void FocusPrevious();
    void FocusDirection(int deltaRow, int deltaCol);

    int GetFocusedRow() const { return m_focusedRow; }
    int GetFocusedCol() const { return m_focusedCol; }
    AgentWindow* GetFocusedWindow();

    // === Fullscreen ===

    void ToggleFullscreen();
    void ExitFullscreen();
    bool IsFullscreen() const { return m_fullscreen; }

    // === Callbacks ===

    void OnEmptySlotClicked(EmptySlotCallback callback);

private:
    void RenderGrid();
    void RenderFullscreen();
    void RenderEmptySlot(int row, int col);
    void UpdateWindowBounds();

    int m_rows = 2;
    int m_cols = 2;
    int m_focusedRow = 0;
    int m_focusedCol = 0;
    bool m_fullscreen = false;

    // Grid of agent windows (row-major order)
    std::vector<std::unique_ptr<AgentWindow>> m_windows;

    // Callbacks
    EmptySlotCallback m_emptySlotCallback;
};

} // namespace smith::ui
```

### 10.3 Agent Window

```cpp
// include/ui/agent_window.h

#pragma once

#include "terminal/terminal_interface.h"
#include <memory>
#include <string>

namespace smith {
    namespace agent {
        struct Agent;
        class IAgentProvider;
    }
}

namespace smith::ui {

/**
 * AgentWindow - Single agent's terminal view
 *
 * Components:
 * - Terminal (ITerminal - WebView or ImGui based)
 * - Top overlay (status, name, type, git branch, cost)
 * - Bottom overlay (working directory, duration, tokens)
 * - Context menu
 */
class AgentWindow {
public:
    AgentWindow();
    ~AgentWindow();

    /**
     * Attach an agent to this window
     */
    void SetAgent(agent::Agent* agent);

    /**
     * Get attached agent
     */
    agent::Agent* GetAgent() const { return m_agent; }

    /**
     * Detach agent (doesn't delete agent)
     */
    void DetachAgent();

    /**
     * Initialize terminal (call after setting agent)
     */
    bool InitializeTerminal(void* parentHwnd = nullptr);

    /**
     * Render the window contents
     * @param x Window X position
     * @param y Window Y position
     * @param width Window width
     * @param height Window height
     * @param focused Whether this window has focus
     */
    void Render(float x, float y, float width, float height, bool focused);

    /**
     * Handle keyboard input (when focused)
     */
    void HandleInput();

    /**
     * Set focus state
     */
    void SetFocused(bool focused);
    bool IsFocused() const { return m_focused; }

    /**
     * Get terminal instance
     */
    terminal::ITerminal* GetTerminal() const { return m_terminal.get(); }

    /**
     * Restart terminal
     */
    void RestartTerminal();

    /**
     * Stop terminal
     */
    void StopTerminal();

    /**
     * Send text to terminal
     */
    void SendInput(const std::string& text);

    /**
     * Request metrics refresh (e.g., /cost for Claude)
     */
    void RequestMetricsRefresh();

    // === Theme ===

    void SetTheme(const std::string& themeName);
    const std::string& GetThemeName() const;

private:
    void RenderTopOverlay(float x, float y, float width);
    void RenderBottomOverlay(float x, float y, float width, float height);
    void RenderContextMenu();
    void RenderAgentInfoPopup();

    void OnTerminalOutput(const char* data, size_t length);
    void OnTerminalExit(int exitCode);
    void ProcessOutputForMetrics(const std::string& output);

    agent::Agent* m_agent = nullptr;
    agent::IAgentProvider* m_provider = nullptr;
    std::unique_ptr<terminal::ITerminal> m_terminal;

    bool m_focused = false;
    bool m_showAgentInfo = false;
    bool m_showThemeMenu = false;

    // Output buffering for metric parsing
    std::string m_outputBuffer;
};

} // namespace smith::ui
```

---

## 11. Platform Abstraction

### 11.1 Platform Detection

```cpp
// include/platform/platform.h

#pragma once

// Platform detection
#if defined(_WIN32) || defined(_WIN64)
    #define SMITH_PLATFORM_WINDOWS 1
    #define SMITH_PLATFORM_NAME "Windows"
#elif defined(__APPLE__)
    #define SMITH_PLATFORM_MACOS 1
    #define SMITH_PLATFORM_NAME "macOS"
#elif defined(__linux__)
    #define SMITH_PLATFORM_LINUX 1
    #define SMITH_PLATFORM_NAME "Linux"
#else
    #error "Unsupported platform"
#endif

// Compiler detection
#if defined(_MSC_VER)
    #define SMITH_COMPILER_MSVC 1
#elif defined(__clang__)
    #define SMITH_COMPILER_CLANG 1
#elif defined(__GNUC__)
    #define SMITH_COMPILER_GCC 1
#endif

// Debug detection
#if defined(_DEBUG) || defined(DEBUG) || !defined(NDEBUG)
    #define SMITH_DEBUG 1
#else
    #define SMITH_RELEASE 1
#endif

namespace smith::platform {

/**
 * Get platform name string
 */
const char* GetPlatformName();

/**
 * Get executable directory path
 */
std::string GetExecutableDirectory();

/**
 * Get user data directory (for config, logs, etc.)
 */
std::string GetUserDataDirectory();

/**
 * Open URL in default browser
 */
bool OpenUrl(const std::string& url);

/**
 * Open folder in file explorer
 */
bool OpenFolder(const std::string& path);

/**
 * Get environment variable
 */
std::string GetEnvironmentVariable(const std::string& name);

/**
 * Set environment variable
 */
bool SetEnvironmentVariable(const std::string& name, const std::string& value);

} // namespace smith::platform
```

---

## 12. Testing Infrastructure

### 12.1 Test Configuration

```cmake
# cmake/Testing.cmake

include(CTest)
include(GoogleTest)

# Enable testing
enable_testing()

# Test executable
add_executable(agent_smith_tests
    tests/test_main.cpp
    tests/unit/core/test_event_dispatcher.cpp
    tests/unit/logging/test_logger.cpp
    tests/unit/config/test_config_manager.cpp
    tests/unit/agent/test_agent.cpp
    tests/unit/agent/test_agent_tracker.cpp
    tests/unit/agent/test_agent_utils.cpp
    tests/unit/terminal/test_terminal_buffer.cpp
    tests/unit/utils/test_string_utils.cpp
    tests/unit/utils/test_git_utils.cpp
    tests/integration/test_agent_lifecycle.cpp
)

target_link_libraries(agent_smith_tests PRIVATE
    agent_smith_lib  # Core library (non-main code)
    GTest::gtest
    GTest::gtest_main
    GTest::gmock
)

target_include_directories(agent_smith_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/tests
)

# Discover tests
gtest_discover_tests(agent_smith_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    PROPERTIES
        LABELS "unit"
)
```

### 12.2 Test Main

```cpp
// tests/test_main.cpp

#include <gtest/gtest.h>

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
```

### 12.3 Example Unit Tests

```cpp
// tests/unit/agent/test_agent.cpp

#include <gtest/gtest.h>
#include "agent/agent.h"

using namespace smith::agent;

TEST(AgentTest, CreateHasValidId) {
    auto agent = Agent::Create("TestAgent", "C:/test", AgentType::ClaudeCode);

    EXPECT_FALSE(agent.id.empty());
    EXPECT_EQ(agent.id.length(), 36);  // UUID length
}

TEST(AgentTest, CreateSetsCorrectDefaults) {
    auto agent = Agent::Create("TestAgent", "C:/test", AgentType::ClaudeCode);

    EXPECT_EQ(agent.name, "TestAgent");
    EXPECT_EQ(agent.workingDirectory, "C:/test");
    EXPECT_EQ(agent.type, AgentType::ClaudeCode);
    EXPECT_EQ(agent.status, AgentStatus::Idle);
    EXPECT_FALSE(agent.autoAcceptEdits);
    EXPECT_EQ(agent.gridRow, 0);
    EXPECT_EQ(agent.gridCol, 0);
}

TEST(AgentTest, TypeToStringMapping) {
    EXPECT_EQ(Agent::Create("", "", AgentType::ClaudeCode).GetTypeString(), "ClaudeCode");
    EXPECT_EQ(Agent::Create("", "", AgentType::Grok).GetTypeString(), "Grok");
    EXPECT_EQ(Agent::Create("", "", AgentType::ChatGPT).GetTypeString(), "ChatGPT");
    EXPECT_EQ(Agent::Create("", "", AgentType::Custom).GetTypeString(), "Custom");
}

TEST(AgentTest, RequiresTerminalCorrect) {
    EXPECT_TRUE(Agent::Create("", "", AgentType::ClaudeCode).RequiresTerminal());
    EXPECT_FALSE(Agent::Create("", "", AgentType::Grok).RequiresTerminal());
    EXPECT_FALSE(Agent::Create("", "", AgentType::ChatGPT).RequiresTerminal());
    EXPECT_TRUE(Agent::Create("", "", AgentType::Custom).RequiresTerminal());
}
```

```cpp
// tests/unit/agent/test_agent_utils.cpp

#include <gtest/gtest.h>
#include "agent/agent_utils.h"

using namespace smith::agent;

class ClaudeMetricParserTest : public ::testing::Test {
protected:
    ClaudeMetricParser parser;
    AgentMetrics metrics;
};

TEST_F(ClaudeMetricParserTest, ParsesCostOutput) {
    std::string output = R"(
Session cost: $0.1234
Total tokens: 12,345 input + 6,789 output = 19,134 total
Model: claude-sonnet-4-20250514
)";

    EXPECT_TRUE(parser.Parse(output, metrics));
    EXPECT_NEAR(metrics.tokens.totalCost, 0.1234, 0.0001);
    EXPECT_EQ(metrics.tokens.inputTokens, 12345);
    EXPECT_EQ(metrics.tokens.outputTokens, 6789);
    EXPECT_EQ(metrics.tokens.model, "claude-sonnet-4-20250514");
}

TEST_F(ClaudeMetricParserTest, HandlesEmptyOutput) {
    EXPECT_FALSE(parser.Parse("", metrics));
    EXPECT_FALSE(metrics.tokens.valid);
}

TEST(AgentUtilsTest, FormatDuration) {
    using namespace std::chrono_literals;

    EXPECT_EQ(AgentUtils::FormatDuration(0s), "0s");
    EXPECT_EQ(AgentUtils::FormatDuration(45s), "45s");
    EXPECT_EQ(AgentUtils::FormatDuration(90s), "1m 30s");
    EXPECT_EQ(AgentUtils::FormatDuration(3661s), "1h 1m");
    EXPECT_EQ(AgentUtils::FormatDuration(7200s), "2h 0m");
}

TEST(AgentUtilsTest, FormatCost) {
    EXPECT_EQ(AgentUtils::FormatCost(0.0), "$0.00");
    EXPECT_EQ(AgentUtils::FormatCost(0.1234), "$0.12");
    EXPECT_EQ(AgentUtils::FormatCost(1.999), "$2.00");
    EXPECT_EQ(AgentUtils::FormatCost(99.99), "$99.99");
}

TEST(AgentUtilsTest, FormatTokenCount) {
    EXPECT_EQ(AgentUtils::FormatTokenCount(0), "0");
    EXPECT_EQ(AgentUtils::FormatTokenCount(999), "999");
    EXPECT_EQ(AgentUtils::FormatTokenCount(1000), "1.0k");
    EXPECT_EQ(AgentUtils::FormatTokenCount(45200), "45.2k");
    EXPECT_EQ(AgentUtils::FormatTokenCount(1000000), "1.0M");
}
```

---

## 13. Build System

### 13.1 Root CMakeLists.txt

```cmake
# CMakeLists.txt

cmake_minimum_required(VERSION 3.16)
project(AgentSmith VERSION 2.0.0 LANGUAGES CXX)

# C++ Standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR}/bin)

# Options
option(SMITH_BUILD_TESTS "Build unit tests" ON)
option(SMITH_USE_WEBVIEW2 "Enable WebView2 terminal backend" ON)

# Include CMake modules
include(cmake/CompilerFlags.cmake)
include(cmake/Dependencies.cmake)

# Collect source files
file(GLOB_RECURSE SMITH_SOURCES
    src/*.cpp
)

file(GLOB_RECURSE SMITH_HEADERS
    include/*.h
    include/*.hpp
)

# Exclude main.cpp for library (used in tests)
list(FILTER SMITH_SOURCES EXCLUDE REGEX "main\\.cpp$")

# Core library (everything except main)
add_library(agent_smith_lib STATIC ${SMITH_SOURCES} ${SMITH_HEADERS})

target_include_directories(agent_smith_lib PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(agent_smith_lib PUBLIC
    imgui
    glfw
    OpenGL::GL
    nlohmann_json::nlohmann_json
    spdlog::spdlog
    httplib::httplib
)

if(WIN32)
    target_link_libraries(agent_smith_lib PUBLIC
        dwmapi
        d3d11
        dxgi
    )

    if(SMITH_USE_WEBVIEW2)
        target_compile_definitions(agent_smith_lib PUBLIC SMITH_USE_WEBVIEW2=1)
        target_include_directories(agent_smith_lib PUBLIC ${WEBVIEW2_INCLUDE_DIR})
        target_link_directories(agent_smith_lib PUBLIC ${WEBVIEW2_LIB_DIR})
        target_link_libraries(agent_smith_lib PUBLIC WebView2Loader.dll.lib)
    endif()
endif()

# Main executable
add_executable(agent_smith src/main.cpp)
target_link_libraries(agent_smith PRIVATE agent_smith_lib)

if(WIN32)
    # Windows GUI application (no console)
    set_target_properties(agent_smith PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
endif()

# Copy resources to bin directory
add_custom_command(TARGET agent_smith POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/resources
        ${CMAKE_SOURCE_DIR}/bin/resources
)

# Tests
if(SMITH_BUILD_TESTS)
    include(cmake/Testing.cmake)
endif()

# Install
install(TARGETS agent_smith RUNTIME DESTINATION bin)
install(DIRECTORY resources/ DESTINATION bin/resources)
```

### 13.2 Compiler Flags

```cmake
# cmake/CompilerFlags.cmake

if(MSVC)
    # MSVC flags
    add_compile_options(
        /W4           # Warning level 4
        /WX-          # Warnings not as errors (for now)
        /MP           # Multi-processor compilation
        /utf-8        # UTF-8 source and execution charset
        /permissive-  # Strict conformance
    )

    add_compile_definitions(
        _CRT_SECURE_NO_WARNINGS
        NOMINMAX              # Don't define min/max macros
        WIN32_LEAN_AND_MEAN   # Exclude rarely-used Windows headers
        _UNICODE
        UNICODE
    )

    # Debug-specific
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_compile_options(/Od /Zi /RTC1)
        add_compile_definitions(_DEBUG)
    else()
        add_compile_options(/O2 /Oi /GL)
        add_link_options(/LTCG)
    endif()

elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    # Clang/GCC flags
    add_compile_options(
        -Wall
        -Wextra
        -Wpedantic
        -Wno-unused-parameter
    )

    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_compile_options(-g -O0)
    else()
        add_compile_options(-O3)
    endif()
endif()
```

---

## 14. Implementation Phases

### Phase 1: Foundation (Priority: Critical)

**Goal:** Establish new project structure and core abstractions without breaking existing functionality.

#### Tasks:

1.1. **Create directory structure**
   - Create all folders in `include/` hierarchy
   - Create all folders in `src/` hierarchy
   - Create `cmake/` directory with module files
   - Create `tests/` directory structure
   - Create `resources/web/` directory

1.2. **Set up CMake infrastructure**
   - Create `cmake/Dependencies.cmake` with FetchContent
   - Create `cmake/CompilerFlags.cmake`
   - Create `cmake/Testing.cmake`
   - Update root `CMakeLists.txt`

1.3. **Implement logging system**
   - Create `include/logging/log_macros.h`
   - Create `include/logging/logger.h`
   - Create `include/logging/log_sink.h`
   - Implement `src/logging/logger.cpp`
   - Implement `src/logging/log_sinks.cpp` (File, Console, ImGui sinks)
   - Write unit tests for logger

1.4. **Implement AppBase framework**
   - Create `include/core/app_base.h`
   - Implement `src/core/app_base.cpp`
   - Extract GLFW/OpenGL/ImGui boilerplate from current `app.cpp`

1.5. **Migrate Application class**
   - Create `include/core/application.h`
   - Implement `src/core/application.cpp`
   - Inherit from AppBase
   - Update `src/main.cpp`

**Deliverables:**
- New folder structure in place
- Logging system working with SMITH_LOG macro
- Application compiles and runs with new structure
- Basic unit tests passing

---

### Phase 2: Agent Abstraction (Priority: High)

**Goal:** Introduce agent provider interface and implement Claude provider.

#### Tasks:

2.1. **Define agent data structures**
   - Create `include/agent/agent.h` with Agent struct
   - Create `include/core/types.h` with enums
   - Implement `src/agent/agent.cpp` factory methods

2.2. **Create provider interface**
   - Create `include/agent/agent_provider.h` with IAgentProvider
   - Create `include/agent/agent_registry.h`
   - Implement `src/agent/agent_registry.cpp`

2.3. **Implement Claude provider**
   - Create `include/agent/providers/claude_provider.h`
   - Implement `src/agent/providers/claude_provider.cpp`
   - Implement output parsing for status detection
   - Write unit tests

2.4. **Implement agent utilities**
   - Create `include/agent/agent_utils.h`
   - Implement metric parsers (ClaudeMetricParser)
   - Implement utility functions
   - Write unit tests

2.5. **Refactor AgentTracker**
   - Move to `include/agent/agent_tracker.h`
   - Integrate with provider registry
   - Add lifecycle callbacks
   - Write unit tests

**Deliverables:**
- IAgentProvider interface defined
- Claude provider working with metric parsing
- AgentTracker using new provider system
- Agent costs displayed in UI

---

### Phase 3: Terminal Abstraction (Priority: High)

**Goal:** Create terminal interface and WebView2 implementation.

#### Tasks:

3.1. **Define terminal interface**
   - Create `include/terminal/terminal_interface.h`
   - Create `include/terminal/terminal_theme.h`
   - Create `include/terminal/terminal_factory.h`

3.2. **Refactor ConPTY terminal**
   - Move to `include/terminal/conpty_terminal.h`
   - Implement ITerminal interface
   - Keep TerminalBuffer as fallback renderer

3.3. **Add WebView2 dependency**
   - Update `cmake/Dependencies.cmake` for WebView2
   - Test WebView2 availability detection

3.4. **Bundle xterm.js resources**
   - Download xterm.js and addons
   - Create `resources/web/terminal.html`
   - Create `resources/web/terminal.css`
   - Create `resources/web/terminal.js`
   - Update CMake to copy resources

3.5. **Implement WebView terminal**
   - Create `include/terminal/webview_terminal.h`
   - Implement `src/terminal/webview_terminal.cpp`
   - Implement JavaScript bridge
   - Test ConPTY → xterm.js output
   - Test xterm.js → ConPTY input

3.6. **Implement terminal factory**
   - Implement `src/terminal/terminal_factory.cpp`
   - Auto-detect best backend
   - Add configuration option

3.7. **Update AgentWindow**
   - Use ITerminal interface
   - Handle WebView2 positioning
   - Test with both backends

**Deliverables:**
- ITerminal interface working
- WebView2 + xterm.js terminal rendering
- Automatic fallback to ImGui renderer
- Configuration option for terminal backend

---

### Phase 4: API Providers (Priority: Medium)

**Goal:** Implement Grok and ChatGPT API providers.

#### Tasks:

4.1. **Add HTTP client dependency**
   - Ensure cpp-httplib is available
   - Create HTTP utility wrappers

4.2. **Implement Grok provider**
   - Create `include/agent/providers/grok_provider.h`
   - Implement `src/agent/providers/grok_provider.cpp`
   - Implement xAI API integration
   - Parse response metrics
   - Write tests

4.3. **Implement ChatGPT provider**
   - Create `include/agent/providers/chatgpt_provider.h`
   - Implement `src/agent/providers/chatgpt_provider.cpp`
   - Implement OpenAI API integration
   - Parse response metrics
   - Write tests

4.4. **Create chat UI for API agents**
   - Design non-terminal agent window variant
   - Implement message history display
   - Implement input field

4.5. **Update Add Agent dialog**
   - Show API key status
   - Show model selection
   - Validate configuration

**Deliverables:**
- Grok API integration working
- ChatGPT API integration working
- Chat-style UI for API agents
- API key management

---

### Phase 5: Testing & Polish (Priority: Medium)

**Goal:** Comprehensive testing and refinement.

#### Tasks:

5.1. **Unit test coverage**
   - Test all agent utilities
   - Test configuration manager
   - Test terminal buffer (fallback)
   - Test metric parsers
   - Aim for 70%+ coverage

5.2. **Integration tests**
   - Test agent lifecycle (create → start → stop → remove)
   - Test terminal I/O
   - Test configuration persistence

5.3. **Performance optimization**
   - Profile rendering
   - Optimize WebView2 message passing
   - Reduce memory allocations

5.4. **UI polish**
   - Improve metrics display
   - Add keyboard shortcut hints
   - Improve error messages
   - Add loading states

5.5. **Documentation**
   - Update Claude_Context.md
   - Add inline code documentation
   - Create user guide

**Deliverables:**
- Test suite with good coverage
- Performance baseline established
- Polished UI
- Updated documentation

---

### Phase 6: Migration Cleanup (Priority: Low)

**Goal:** Remove deprecated code and finalize architecture.

#### Tasks:

6.1. **Remove deprecated files**
   - Remove old flat header files
   - Remove unused code paths
   - Clean up #ifdef blocks

6.2. **Code review**
   - Check naming conventions
   - Check error handling
   - Check thread safety

6.3. **Final testing**
   - Full regression test
   - Test on clean Windows install
   - Test WebView2 fallback

6.4. **Release preparation**
   - Update version numbers
   - Create release notes
   - Build release binaries

**Deliverables:**
- Clean codebase
- Release-ready build
- Complete documentation

---

## 15. File Manifest

### 15.1 New Files to Create

| File | Phase | Description |
|------|-------|-------------|
| `cmake/Dependencies.cmake` | 1 | FetchContent declarations |
| `cmake/CompilerFlags.cmake` | 1 | Compiler settings |
| `cmake/Testing.cmake` | 1 | Test configuration |
| `include/core/app_base.h` | 1 | AppBase class |
| `include/core/application.h` | 1 | Application singleton |
| `include/core/types.h` | 1 | Common types |
| `include/core/event.h` | 1 | Event types |
| `include/logging/log_macros.h` | 1 | SMITH_LOG macro |
| `include/logging/logger.h` | 1 | Logger class |
| `include/logging/log_sink.h` | 1 | ILogSink interface |
| `include/agent/agent.h` | 2 | Agent struct |
| `include/agent/agent_provider.h` | 2 | IAgentProvider interface |
| `include/agent/agent_registry.h` | 2 | Provider registry |
| `include/agent/agent_utils.h` | 2 | Metric parsers |
| `include/agent/providers/claude_provider.h` | 2 | Claude provider |
| `include/agent/providers/grok_provider.h` | 4 | Grok provider |
| `include/agent/providers/chatgpt_provider.h` | 4 | ChatGPT provider |
| `include/agent/providers/custom_provider.h` | 2 | Custom provider |
| `include/terminal/terminal_interface.h` | 3 | ITerminal interface |
| `include/terminal/terminal_factory.h` | 3 | Terminal factory |
| `include/terminal/webview_terminal.h` | 3 | WebView2 terminal |
| `include/platform/platform.h` | 1 | Platform macros |
| `src/core/app_base.cpp` | 1 | AppBase implementation |
| `src/core/application.cpp` | 1 | Application implementation |
| `src/logging/logger.cpp` | 1 | Logger implementation |
| `src/logging/log_sinks.cpp` | 1 | Sink implementations |
| `src/agent/agent.cpp` | 2 | Agent factory methods |
| `src/agent/agent_registry.cpp` | 2 | Registry implementation |
| `src/agent/agent_utils.cpp` | 2 | Utils implementation |
| `src/agent/providers/claude_provider.cpp` | 2 | Claude implementation |
| `src/agent/providers/grok_provider.cpp` | 4 | Grok implementation |
| `src/agent/providers/chatgpt_provider.cpp` | 4 | ChatGPT implementation |
| `src/terminal/webview_terminal.cpp` | 3 | WebView2 implementation |
| `src/terminal/terminal_factory.cpp` | 3 | Factory implementation |
| `resources/web/terminal.html` | 3 | xterm.js host page |
| `resources/web/terminal.css` | 3 | Custom styles |
| `resources/web/terminal.js` | 3 | JavaScript bridge |
| `tests/test_main.cpp` | 1 | Test entry point |
| `tests/unit/agent/test_agent.cpp` | 2 | Agent tests |
| `tests/unit/agent/test_agent_utils.cpp` | 2 | Utils tests |
| `tests/unit/logging/test_logger.cpp` | 1 | Logger tests |

### 15.2 Files to Migrate/Refactor

| Current File | New Location | Phase |
|--------------|--------------|-------|
| `include/app.h` | `include/core/application.h` | 1 |
| `include/types.h` | Split to multiple | 1-2 |
| `include/config.h` | `include/config/config_manager.h` | 1 |
| `include/agents_tracker.h` | `include/agent/agent_tracker.h` | 2 |
| `include/agent_window.h` | `include/ui/agent_window.h` | 3 |
| `include/grid_layout.h` | `include/ui/grid_layout.h` | 1 |
| `include/conpty_terminal.h` | `include/terminal/conpty_terminal.h` | 3 |
| `include/terminal_buffer.h` | `include/terminal/terminal_buffer.h` | 3 |
| `include/output_log.h` | `include/ui/output_log.h` | 1 |
| `include/git_utils.h` | `include/utils/git_utils.h` | 1 |

### 15.3 External Resources to Download

| Resource | URL | Destination |
|----------|-----|-------------|
| xterm.js | https://cdn.jsdelivr.net/npm/xterm@5.3.0/lib/xterm.js | `resources/web/xterm.js` |
| xterm.css | https://cdn.jsdelivr.net/npm/xterm@5.3.0/css/xterm.css | `resources/web/xterm.css` |
| xterm-addon-fit | https://cdn.jsdelivr.net/npm/xterm-addon-fit@0.8.0/lib/xterm-addon-fit.js | `resources/web/xterm-addon-fit.js` |
| xterm-addon-web-links | https://cdn.jsdelivr.net/npm/xterm-addon-web-links@0.9.0/lib/xterm-addon-web-links.js | `resources/web/xterm-addon-web-links.js` |

---

## 16. Migration Guide

### 16.1 Incremental Migration Strategy

The migration follows an incremental approach where the application remains functional throughout:

1. **Phase 1:** Add new structure alongside existing code
2. **Phase 2-3:** Gradually replace old code with new abstractions
3. **Phase 4-5:** Add new features using new architecture
4. **Phase 6:** Remove deprecated code

### 16.2 Backward Compatibility

During migration:
- Old header paths work via forwarding headers
- Existing config.json format supported
- ImGui terminal remains as fallback

### 16.3 Breaking Changes

After migration complete:
- Include paths change (e.g., `#include "app.h"` → `#include "core/application.h"`)
- Namespace changes (e.g., `Agent` → `smith::agent::Agent`)
- Some internal APIs change

---

## 17. Risk Assessment

| Risk | Impact | Probability | Mitigation |
|------|--------|-------------|------------|
| WebView2 not available | High | Low | Automatic fallback to ImGui terminal |
| xterm.js rendering latency | Medium | Medium | Batch output, efficient encoding |
| Breaking existing functionality | High | Medium | Incremental migration, comprehensive tests |
| API rate limits (Grok/ChatGPT) | Medium | Medium | Implement rate limiting, show status |
| Memory leaks | Medium | Medium | Use smart pointers, sanitizers in CI |
| Cross-platform issues | Low | Low | Windows-first, stubs for other platforms |

---

## Appendix A: API Reference

### A.1 xAI Grok API

```
Endpoint: https://api.x.ai/v1/chat/completions
Method: POST
Headers:
  Authorization: Bearer $GROK_API_KEY
  Content-Type: application/json

Request Body:
{
  "model": "grok-2",
  "messages": [
    {"role": "user", "content": "Hello"}
  ],
  "stream": false
}

Response:
{
  "id": "...",
  "choices": [{
    "message": {"role": "assistant", "content": "..."},
    "finish_reason": "stop"
  }],
  "usage": {
    "prompt_tokens": 10,
    "completion_tokens": 20,
    "total_tokens": 30
  }
}
```

### A.2 OpenAI ChatGPT API

```
Endpoint: https://api.openai.com/v1/chat/completions
Method: POST
Headers:
  Authorization: Bearer $OPENAI_API_KEY
  Content-Type: application/json

Request Body:
{
  "model": "gpt-4-turbo",
  "messages": [
    {"role": "user", "content": "Hello"}
  ],
  "stream": false
}

Response:
{
  "id": "...",
  "choices": [{
    "message": {"role": "assistant", "content": "..."},
    "finish_reason": "stop"
  }],
  "usage": {
    "prompt_tokens": 10,
    "completion_tokens": 20,
    "total_tokens": 30
  }
}
```

---

## Appendix B: Terminal Theme JSON Format

```json
{
  "themes": [
    {
      "name": "Catppuccin Macchiato",
      "background": "#24273A",
      "foreground": "#CAD3F5",
      "cursor": "#F4DBD6",
      "selection": "#5B6078",
      "ansiColors": [
        "#494D64", "#ED8796", "#A6DA95", "#EED49F",
        "#8AADF4", "#F5BDE6", "#8BD5CA", "#B8C0E0",
        "#5B6078", "#ED8796", "#A6DA95", "#EED49F",
        "#8AADF4", "#F5BDE6", "#8BD5CA", "#A5ADCB"
      ]
    }
  ]
}
```

---

*Document Version: 1.0*
*Last Updated: 2026-01-26*
*Author: Claude (Software Architect Agent)*

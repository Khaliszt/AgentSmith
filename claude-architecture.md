# AgentSmith v2.0 - Complete Architecture Specification

> **Purpose:** Complete architectural blueprint for AgentSmith v2.0 designed for consumption by Implementation Manager and Code Implementer agents. Contains all specifications, interfaces, data structures, and implementation details needed to develop the system.

> **Version:** 2.0-DRAFT
> **Last Updated:** 2026-01-27
> **Status:** Ready for Implementation Planning

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
12. [Error Handling Strategy](#12-error-handling-strategy)
13. [Threading Model](#13-threading-model)
14. [Security Considerations](#14-security-considerations)
15. [Testing Infrastructure](#15-testing-infrastructure)
16. [Build System](#16-build-system)
17. [CI/CD Pipeline](#17-cicd-pipeline)
18. [Implementation Phases](#18-implementation-phases)
19. [File Manifest](#19-file-manifest)
20. [Migration Guide](#20-migration-guide)
21. [Risk Assessment](#21-risk-assessment)
22. [Appendices](#22-appendices)

---

## 1. Executive Summary

### 1.1 Project Vision

AgentSmith is a **multi-agent mission control interface** for managing multiple AI coding agents with:
- Embedded terminals with native-quality rendering (WebView2 + xterm.js)
- Real-time status monitoring and cost tracking
- Support for both terminal-based (Claude Code) and API-based (Grok, ChatGPT) agents
- Professional-grade logging, testing, and extensibility

### 1.2 Key Objectives

| Objective | Description | Success Criteria |
|-----------|-------------|------------------|
| **Terminal Rendering Overhaul** | Replace custom ANSI parser with WebView2 + xterm.js | All ANSI sequences render correctly; smooth scrolling |
| **Agent Provider Abstraction** | Extensible interface for terminal and API agents | Add new provider in <100 LOC |
| **Metrics & Cost Tracking** | Parse and display costs, tokens, rate limits | Claude /cost parsed; API usage tracked |
| **Professional Logging** | SMITH_LOG macro with categories/verbosity | Filter by category; persist to file |
| **Testing Infrastructure** | GoogleTest with 70%+ coverage | CI runs tests on every commit |
| **Clean Architecture** | AppBase pattern; dependency injection | Testable components; clear ownership |

### 1.3 Supported Agent Types

| Agent | Type | Integration | Priority | Status |
|-------|------|-------------|----------|--------|
| Claude Code | Terminal | ConPTY + output parsing | P0 | Primary focus |
| Grok | API | xAI REST API | P1 | High priority |
| ChatGPT | API | OpenAI REST API | P1 | High priority |
| Cursor | Terminal | CLI (if available) | P2 | Future |
| Custom | Terminal | User-defined command | P2 | Supported |

### 1.4 Non-Goals (Explicitly Out of Scope)

- Multi-user/collaborative features
- Cloud-based agent orchestration
- Mobile platform support
- Voice interface
- Auto-update mechanism (v2.1+)

---

## 2. Current State Analysis

### 2.1 Existing Architecture

```
Current Architecture (v1.x):
┌─────────────────────────────────────────────────────────────┐
│                         App (monolithic)                     │
│  ┌─────────────┬─────────────┬──────────────┬─────────────┐ │
│  │ ConfigMgr   │ AgentTracker│ InputManager │ OutputLog   │ │
│  └─────────────┴─────────────┴──────────────┴─────────────┘ │
│                              │                               │
│  ┌───────────────────────────┴───────────────────────────┐  │
│  │                      GridLayout                        │  │
│  │  ┌─────────────┬─────────────┬─────────────┐          │  │
│  │  │ AgentWindow │ AgentWindow │ AgentWindow │ ...      │  │
│  │  │ ┌─────────┐ │ ┌─────────┐ │ ┌─────────┐ │          │  │
│  │  │ │ConPTY   │ │ │ConPTY   │ │ │ConPTY   │ │          │  │
│  │  │ │Terminal │ │ │Terminal │ │ │Terminal │ │          │  │
│  │  │ ├─────────┤ │ ├─────────┤ │ ├─────────┤ │          │  │
│  │  │ │Terminal │ │ │Terminal │ │ │Terminal │ │          │  │
│  │  │ │Buffer   │ │ │Buffer   │ │ │Buffer   │ │          │  │
│  │  │ └─────────┘ │ └─────────┘ │ └─────────┘ │          │  │
│  │  └─────────────┴─────────────┴─────────────┘          │  │
│  └───────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
```

### 2.2 Current Files Inventory

| Current File | Lines | Purpose | Migration Action |
|--------------|-------|---------|------------------|
| `include/app.h` | ~150 | Main application | Split to AppBase + Application |
| `include/types.h` | ~200 | All type definitions | Split to core/types + agent/agent |
| `include/config.h` | ~80 | Config loading | Move to config/config_manager.h |
| `include/agents_tracker.h` | ~100 | Agent lifecycle | Move + add provider integration |
| `include/agent_window.h` | ~120 | Single agent UI | Move + use ITerminal |
| `include/grid_layout.h` | ~80 | Grid management | Move, namespace change |
| `include/conpty_terminal.h` | ~100 | Windows PTY | Move + implement ITerminal |
| `include/terminal_buffer.h` | ~150 | ANSI parser | Move, becomes fallback |
| `include/output_log.h` | ~60 | Log display | Move + integrate with Logger |
| `include/git_utils.h` | ~40 | Git info | Move to utils/ |
| `include/input_manager.h` | ~50 | Input routing | Move to core/ |
| `src/*.cpp` | ~2500 | Implementations | Reorganize to match include/ |

### 2.3 Pain Points to Address

| Issue | Impact | Root Cause | Solution |
|-------|--------|------------|----------|
| Terminal rendering bugs | High | Custom ANSI parser incomplete | WebView2 + xterm.js |
| All agents identical | Medium | No abstraction layer | IAgentProvider interface |
| No cost tracking | Medium | No output parsing | AgentUtils metric parsers |
| No tests | High | No test infrastructure | GoogleTest + ctest |
| Hard to navigate | Low | Flat file structure | Organized include/ hierarchy |
| Inconsistent logging | Medium | Multiple logging methods | Unified SMITH_LOG |

---

## 3. Technology Stack

### 3.1 Core Dependencies (Existing - Keep)

| Dependency | Version | Purpose | Integration |
|------------|---------|---------|-------------|
| **C++17** | - | Language standard | Compiler flag |
| **GLFW** | 3.3.9+ | Window/input | FetchContent |
| **OpenGL** | 3.3 Core | Rendering | System |
| **Dear ImGui** | 1.90.1+ | Immediate GUI | FetchContent |
| **nlohmann/json** | 3.11.3+ | JSON parsing | FetchContent |

### 3.2 New Dependencies to Add

| Dependency | Version | Purpose | Size | Integration |
|------------|---------|---------|------|-------------|
| **WebView2 SDK** | 1.0.2210+ | Browser control | ~5MB | NuGet/CMake |
| **spdlog** | 1.12.0+ | Logging | ~1MB | FetchContent |
| **fmt** | 10.1.0+ | Formatting | ~500KB | Via spdlog |
| **GoogleTest** | 1.14.0+ | Unit testing | ~2MB | FetchContent |
| **cpp-httplib** | 0.14.3+ | HTTP client | Header-only | FetchContent |
| **xterm.js** | 5.3.0+ | Terminal render | ~300KB | Bundled |

### 3.3 Windows-Specific Requirements

| Requirement | Min Version | Purpose |
|-------------|-------------|---------|
| Windows 10 | 1809 (Oct 2018) | ConPTY API |
| WebView2 Runtime | Auto-installed | Browser control |
| Visual Studio | 2019+ | C++17 support |

### 3.4 CMake FetchContent Configuration

```cmake
# cmake/Dependencies.cmake

include(FetchContent)
set(FETCHCONTENT_QUIET OFF)

# === Core Dependencies ===

# GLFW
FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.3.9
)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

# Dear ImGui
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.90.1
)

# nlohmann/json
FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
)

# === New Dependencies ===

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
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

# cpp-httplib
FetchContent_Declare(
    httplib
    GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
    GIT_TAG v0.14.3
)
set(HTTPLIB_REQUIRE_OPENSSL OFF CACHE BOOL "" FORCE)

# WebView2 (Windows only)
if(WIN32)
    FetchContent_Declare(
        webview2
        URL https://www.nuget.org/api/v2/package/Microsoft.Web.WebView2/1.0.2210.55
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )

    # Windows Implementation Library (for COM helpers)
    FetchContent_Declare(
        wil
        GIT_REPOSITORY https://github.com/microsoft/wil.git
        GIT_TAG v1.0.231216.1
    )
endif()

# Make available
FetchContent_MakeAvailable(glfw imgui json spdlog googletest httplib)
if(WIN32)
    FetchContent_MakeAvailable(webview2 wil)
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
│   ├── CompilerFlags.cmake            # Compiler settings per platform
│   ├── Testing.cmake                  # Test configuration
│   └── InstallRules.cmake             # Installation rules
│
├── docs/
│   ├── claude_context.md              # Original project context
│   ├── claude-architecture.md         # This document
│   └── api/                           # Generated API docs (Doxygen)
│
├── include/
│   ├── smith.h                        # Umbrella header
│   │
│   ├── core/                          # Core framework
│   │   ├── app_base.h                 # Reusable ImGui app framework
│   │   ├── application.h              # AgentSmith application singleton
│   │   ├── types.h                    # Enums, forward declarations
│   │   ├── result.h                   # Result<T, E> error handling
│   │   ├── event.h                    # Event types
│   │   ├── event_dispatcher.h         # Pub/sub event system
│   │   └── input_manager.h            # Keyboard/mouse routing
│   │
│   ├── logging/                       # Logging subsystem
│   │   ├── log_macros.h               # SMITH_LOG, SMITH_ASSERT macros
│   │   ├── logger.h                   # Logger singleton
│   │   └── log_sink.h                 # ILogSink interface + implementations
│   │
│   ├── config/                        # Configuration management
│   │   ├── config_manager.h           # Load/save/watch configuration
│   │   ├── app_config.h               # Configuration structs
│   │   └── config_schema.h            # Validation schema
│   │
│   ├── agent/                         # Agent abstraction layer
│   │   ├── agent.h                    # Agent data structure
│   │   ├── agent_tracker.h            # Agent lifecycle management
│   │   ├── agent_provider.h           # IAgentProvider interface
│   │   ├── agent_registry.h           # Provider factory/registry
│   │   ├── agent_utils.h              # Metrics, parsing utilities
│   │   ├── agent_metrics.h            # Metric data structures
│   │   ├── conversation.h             # Conversation history (API agents)
│   │   └── providers/
│   │       ├── claude_provider.h      # Claude Code (terminal)
│   │       ├── grok_provider.h        # Grok (xAI API)
│   │       ├── chatgpt_provider.h     # ChatGPT (OpenAI API)
│   │       ├── cursor_provider.h      # Cursor (terminal)
│   │       └── custom_provider.h      # Custom command
│   │
│   ├── terminal/                      # Terminal subsystem
│   │   ├── terminal_interface.h       # ITerminal abstraction
│   │   ├── terminal_theme.h           # Theme structures + manager
│   │   ├── terminal_buffer.h          # ANSI parser (fallback)
│   │   ├── conpty_terminal.h          # ConPTY wrapper
│   │   ├── webview_terminal.h         # WebView2 + xterm.js
│   │   └── terminal_factory.h         # Factory pattern
│   │
│   ├── ui/                            # UI components
│   │   ├── imgui_layer.h              # ImGui initialization
│   │   ├── imgui_extensions.h         # Custom ImGui widgets
│   │   ├── menu_bar.h                 # Main menu
│   │   ├── grid_layout.h              # Agent grid
│   │   ├── agent_window.h             # Agent terminal view
│   │   ├── chat_window.h              # Chat view (API agents)
│   │   ├── output_log.h               # Log viewer
│   │   ├── metrics_panel.h            # Metrics display
│   │   └── dialogs/
│   │       ├── dialog_base.h          # Base dialog class
│   │       ├── add_agent_dialog.h
│   │       ├── settings_dialog.h
│   │       ├── about_dialog.h
│   │       └── confirmation_dialog.h
│   │
│   ├── network/                       # Networking (API agents)
│   │   ├── http_client.h              # Async HTTP client wrapper
│   │   ├── api_client.h               # Base API client
│   │   ├── rate_limiter.h             # Rate limiting
│   │   └── retry_policy.h             # Retry with backoff
│   │
│   ├── platform/                      # Platform abstraction
│   │   ├── platform.h                 # Detection macros
│   │   ├── clipboard.h                # Clipboard ops
│   │   ├── file_dialog.h              # Native file dialogs
│   │   ├── process.h                  # Process spawning
│   │   ├── shell.h                    # Shell integration
│   │   └── secure_storage.h           # Credential storage
│   │
│   └── utils/                         # Utilities
│       ├── git_utils.h                # Git operations
│       ├── string_utils.h             # String manipulation
│       ├── time_utils.h               # Time formatting
│       ├── uuid.h                     # UUID generation
│       ├── base64.h                   # Base64 encode/decode
│       └── json_utils.h               # JSON helpers
│
├── src/                               # Implementation files
│   ├── main.cpp                       # Entry point
│   ├── core/
│   ├── logging/
│   ├── config/
│   ├── agent/
│   │   └── providers/
│   ├── terminal/
│   ├── ui/
│   │   └── dialogs/
│   ├── network/
│   ├── platform/
│   │   ├── windows/
│   │   ├── linux/                     # Stubs for future
│   │   └── macos/                     # Stubs for future
│   └── utils/
│
├── resources/
│   ├── fonts/
│   │   ├── JetBrainsMono-Regular.ttf
│   │   ├── JetBrainsMono-Bold.ttf
│   │   └── NerdFontsSymbols.ttf
│   ├── themes/
│   │   └── terminal_themes.json
│   ├── icons/
│   │   └── agent_smith.ico
│   └── web/
│       ├── terminal.html
│       ├── terminal.css
│       ├── terminal.js
│       ├── xterm.min.js
│       ├── xterm.css
│       ├── xterm-addon-fit.min.js
│       └── xterm-addon-web-links.min.js
│
├── tests/
│   ├── CMakeLists.txt
│   ├── test_main.cpp
│   ├── test_utils.h                   # Test helpers
│   ├── mocks/                         # Mock objects
│   │   ├── mock_terminal.h
│   │   ├── mock_http_client.h
│   │   └── mock_agent_provider.h
│   ├── unit/
│   │   ├── core/
│   │   ├── logging/
│   │   ├── config/
│   │   ├── agent/
│   │   ├── terminal/
│   │   ├── network/
│   │   └── utils/
│   ├── integration/
│   │   ├── test_agent_lifecycle.cpp
│   │   ├── test_terminal_io.cpp
│   │   └── test_api_providers.cpp
│   └── fixtures/                      # Test data files
│       ├── sample_config.json
│       ├── claude_cost_output.txt
│       └── api_responses/
│
├── scripts/
│   ├── build.ps1                      # Windows build script
│   ├── build.sh                       # Unix build script
│   ├── download_xterm.ps1             # Download xterm.js
│   └── run_tests.ps1                  # Test runner
│
└── bin/                               # Output directory
    ├── agent_smith.exe
    ├── agent_smith_tests.exe
    ├── config.json
    ├── agentsmith.log
    └── resources/
```

---

## 5. Core Framework (AppBase)

### 5.1 Design Philosophy

The AppBase pattern (inspired by Dear-ImGui-App-Framework) separates:
- **Framework concerns** (GLFW, OpenGL, ImGui lifecycle) → `AppBase`
- **Application logic** (agents, terminals, UI) → `Application`

This enables:
- Unit testing of application logic without graphics
- Potential reuse of framework for other ImGui apps
- Clear initialization/shutdown sequence

### 5.2 AppBase Class

```cpp
// include/core/app_base.h

#pragma once

#include <string>
#include <chrono>
#include <memory>

struct GLFWwindow;

namespace smith::core {

/**
 * AppBase - Reusable Dear ImGui application framework
 *
 * Handles all boilerplate:
 * - GLFW window creation with error handling
 * - OpenGL 3.3 Core context
 * - Dear ImGui with docking enabled
 * - Main loop with configurable frame rate
 * - Graceful shutdown
 *
 * Thread Safety: Main thread only (enforced)
 */
class AppBase {
public:
    struct Config {
        // Window settings
        std::string title = "AgentSmith";
        int width = 1920;
        int height = 1080;
        bool vsync = true;
        bool maximized = false;
        bool decorated = true;
        bool resizable = true;
        bool alwaysOnTop = false;

        // ImGui settings
        std::string imguiIniPath = "";
        bool enableDocking = true;
        bool enableViewports = false;  // Multi-window (experimental)

        // Font settings
        float fontSize = 16.0f;
        std::string fontPath = "";
        std::vector<std::string> fallbackFontPaths = {};

        // Performance
        int targetFps = 60;  // 0 = unlimited
        bool reduceWhenUnfocused = true;
    };

    AppBase();
    virtual ~AppBase();

    // Delete copy/move
    AppBase(const AppBase&) = delete;
    AppBase& operator=(const AppBase&) = delete;

    /**
     * Initialize and run the application
     * @param config Application configuration
     * @return Exit code (0 = success)
     */
    int Run(const Config& config);

    /**
     * Request graceful exit
     */
    void RequestExit() { m_exitRequested = true; }
    bool IsExitRequested() const { return m_exitRequested; }

    // === Accessors ===

    GLFWwindow* GetWindow() const { return m_window; }
    float GetDeltaTime() const { return m_deltaTime; }
    double GetTotalTime() const;
    int GetWindowWidth() const { return m_windowWidth; }
    int GetWindowHeight() const { return m_windowHeight; }
    float GetDpiScale() const { return m_dpiScale; }
    bool IsFocused() const { return m_focused; }
    int GetFrameCount() const { return m_frameCount; }

protected:
    // === Override Points ===

    /**
     * Called once after initialization, before main loop
     * Use for: loading configuration, creating subsystems
     * @return false to abort startup
     */
    virtual bool OnStartUp() { return true; }

    /**
     * Called every frame before rendering
     * Use for: non-UI updates, polling, timers
     * @param dt Delta time in seconds
     */
    virtual void OnUpdate(float dt) { (void)dt; }

    /**
     * Called every frame for ImGui rendering
     * Use for: all UI code
     */
    virtual void OnImGuiRender() {}

    /**
     * Called before shutdown
     * Use for: saving state, cleanup
     */
    virtual void OnShutDown() {}

    /**
     * Called on window resize
     */
    virtual void OnResize(int width, int height) { (void)width; (void)height; }

    /**
     * Called on file drop
     */
    virtual void OnFileDrop(const std::vector<std::string>& paths) { (void)paths; }

    /**
     * Called on focus change
     */
    virtual void OnFocusChanged(bool focused) { (void)focused; }

    /**
     * Called for unhandled errors (can override for custom handling)
     */
    virtual void OnError(const std::string& message);

private:
    bool Initialize(const Config& config);
    void MainLoop(const Config& config);
    void Shutdown();

    void BeginFrame();
    void EndFrame();
    void LoadFonts(const Config& config);
    void ApplyStyle();

    // GLFW callbacks
    static void ErrorCallback(int error, const char* description);
    static void FramebufferCallback(GLFWwindow* window, int w, int h);
    static void DropCallback(GLFWwindow* window, int count, const char** paths);
    static void FocusCallback(GLFWwindow* window, int focused);

    GLFWwindow* m_window = nullptr;
    bool m_exitRequested = false;
    bool m_initialized = false;
    bool m_focused = true;

    int m_windowWidth = 0;
    int m_windowHeight = 0;
    float m_dpiScale = 1.0f;
    float m_deltaTime = 0.0f;
    int m_frameCount = 0;

    std::chrono::steady_clock::time_point m_startTime;
    std::chrono::steady_clock::time_point m_lastFrameTime;
};

} // namespace smith::core
```

### 5.3 Application Class

```cpp
// include/core/application.h

#pragma once

#include "core/app_base.h"
#include <memory>

namespace smith {
    namespace config { class ConfigManager; }
    namespace logging { class Logger; }
    namespace agent { class AgentTracker; class AgentProviderRegistry; }
    namespace ui { class GridLayout; class MenuBar; class OutputLogPanel; }
}

namespace smith::core {

/**
 * Application - AgentSmith main application singleton
 *
 * Owns all subsystems and coordinates their lifecycle.
 */
class Application final : public AppBase {
public:
    /**
     * Get singleton instance
     */
    static Application& Instance();

    /**
     * Main entry point (call from main())
     */
    static int Main(int argc, char** argv);

    // === Subsystem Access ===

    logging::Logger& GetLogger();
    config::ConfigManager& GetConfig();
    agent::AgentTracker& GetAgentTracker();
    agent::AgentProviderRegistry& GetProviderRegistry();

    // === Paths ===

    const std::string& GetExecutableDir() const { return m_exeDir; }
    const std::string& GetResourcesDir() const { return m_resourcesDir; }
    const std::string& GetConfigPath() const { return m_configPath; }

protected:
    bool OnStartUp() override;
    void OnUpdate(float dt) override;
    void OnImGuiRender() override;
    void OnShutDown() override;
    void OnResize(int width, int height) override;
    void OnFileDrop(const std::vector<std::string>& paths) override;
    void OnError(const std::string& message) override;

private:
    Application();
    ~Application();

    void DiscoverPaths();
    void InitializeLogging();
    void InitializeConfig();
    void InitializeAgentSystem();
    void InitializeUI();
    void RegisterProviders();

    void SaveState();
    void RestoreState();

    // Subsystems (initialization order matters)
    std::unique_ptr<logging::Logger> m_logger;
    std::unique_ptr<config::ConfigManager> m_config;
    std::unique_ptr<agent::AgentProviderRegistry> m_providerRegistry;
    std::unique_ptr<agent::AgentTracker> m_agentTracker;

    // UI components
    std::unique_ptr<ui::MenuBar> m_menuBar;
    std::unique_ptr<ui::GridLayout> m_gridLayout;
    std::unique_ptr<ui::OutputLogPanel> m_outputLog;

    // Paths
    std::string m_exeDir;
    std::string m_resourcesDir;
    std::string m_configPath;
};

// === Global Accessors (convenience) ===

inline Application& App() { return Application::Instance(); }
inline logging::Logger& Log() { return Application::Instance().GetLogger(); }
inline config::ConfigManager& Config() { return Application::Instance().GetConfig(); }

} // namespace smith::core
```

### 5.4 Entry Point

```cpp
// src/main.cpp

#include "core/application.h"

int main(int argc, char** argv) {
    return smith::core::Application::Main(argc, argv);
}

#ifdef _WIN32
#include <Windows.h>
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
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

namespace smith::logging {

enum class LogLevel : int {
    Verbose = 0,  // Detailed tracing
    Debug = 1,    // Debug information
    Info = 2,     // General information
    Warning = 3,  // Potential issues
    Error = 4,    // Recoverable errors
    Fatal = 5     // Unrecoverable (may terminate)
};

// Predefined categories
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

} // namespace smith::logging

// === Main Logging Macro ===
// Usage: SMITH_LOG(Info, "Agent", "Started agent: {}", agentName);

#define SMITH_LOG(Level, Category, Format, ...) \
    ::smith::logging::Logger::Instance().Log( \
        ::smith::logging::LogLevel::Level, \
        Category, \
        __FILE__, __LINE__, __FUNCTION__, \
        Format, ##__VA_ARGS__)

// === Convenience Macros ===

#define SMITH_VERBOSE(Cat, Fmt, ...) SMITH_LOG(Verbose, Cat, Fmt, ##__VA_ARGS__)
#define SMITH_DEBUG(Cat, Fmt, ...)   SMITH_LOG(Debug, Cat, Fmt, ##__VA_ARGS__)
#define SMITH_INFO(Cat, Fmt, ...)    SMITH_LOG(Info, Cat, Fmt, ##__VA_ARGS__)
#define SMITH_WARN(Cat, Fmt, ...)    SMITH_LOG(Warning, Cat, Fmt, ##__VA_ARGS__)
#define SMITH_ERROR(Cat, Fmt, ...)   SMITH_LOG(Error, Cat, Fmt, ##__VA_ARGS__)
#define SMITH_FATAL(Cat, Fmt, ...)   SMITH_LOG(Fatal, Cat, Fmt, ##__VA_ARGS__)

// === Assertions ===

#define SMITH_ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            SMITH_FATAL("Assert", "Assertion failed: {} at {}:{}", \
                        message, __FILE__, __LINE__); \
            std::abort(); \
        } \
    } while (false)

#define SMITH_VERIFY(condition) \
    do { \
        if (!(condition)) { \
            SMITH_ERROR("Verify", "Verification failed at {}:{}", __FILE__, __LINE__); \
        } \
    } while (false)
```

### 6.2 Logger Implementation

```cpp
// include/logging/logger.h

#pragma once

#include "log_macros.h"
#include <memory>
#include <vector>
#include <mutex>
#include <string>
#include <chrono>
#include <functional>
#include <spdlog/spdlog.h>

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

        std::string message = fmt::format(fmt::runtime(format),
                                         std::forward<Args>(args)...);

        LogEntry entry{
            level, category, std::move(message),
            file, line, function,
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

    void Dispatch(const LogEntry& entry);

    std::vector<std::shared_ptr<ILogSink>> m_sinks;
    mutable std::mutex m_mutex;
    LogLevel m_minLevel = LogLevel::Info;
    std::vector<std::string> m_enabledCategories;
    bool m_filterCategories = false;
};

// === Built-in Sinks ===

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

    const std::deque<LogEntry>& GetEntries() const;
    void Clear();
private:
    std::deque<LogEntry> m_entries;
    size_t m_maxEntries;
    mutable std::mutex m_mutex;
};

} // namespace smith::logging
```

---

## 7. Configuration System

### 7.1 Configuration Structures

```cpp
// include/config/app_config.h

#pragma once

#include <string>
#include <vector>
#include <optional>

namespace smith::config {

struct WindowConfig {
    int width = 1920;
    int height = 1080;
    int posX = -1;  // -1 = centered
    int posY = -1;
    bool maximized = false;
    bool vsync = true;
};

struct GridConfig {
    int rows = 2;
    int cols = 2;
    int focusedRow = 0;
    int focusedCol = 0;
};

struct TerminalConfig {
    std::string defaultTheme = "Catppuccin Macchiato";
    std::string fontFamily = "JetBrains Mono";
    int fontSize = 14;
    int scrollbackLines = 10000;
    bool useWebView = true;
    bool cursorBlink = true;
};

struct LoggingConfig {
    std::string minLevel = "Info";
    bool logToFile = true;
    std::string logFilePath = "agentsmith.log";
    bool logToConsole = false;
    std::string categoryFilter = "";
};

struct AgentConfig {
    std::string id;
    std::string name;
    std::string type;  // "ClaudeCode", "Grok", "ChatGPT", etc.
    std::string workingDirectory;
    std::string command;
    std::vector<std::string> args;
    std::string terminalTheme;
    bool autoAcceptEdits = false;
    int gridRow = 0;
    int gridCol = 0;

    // API-specific
    std::string apiKeyEnvVar;
    std::string model;
    std::string systemPrompt;

    // Metrics refresh
    int metricsRefreshSeconds = 300;
};

struct ApiProviderConfig {
    std::string name;
    std::string baseUrl;
    std::string apiKeyEnvVar;
    std::string defaultModel;
    int timeoutSeconds = 60;
    int maxRetries = 3;
};

struct AppConfig {
    WindowConfig window;
    GridConfig grid;
    TerminalConfig terminal;
    LoggingConfig logging;
    std::vector<AgentConfig> agents;
    std::vector<ApiProviderConfig> apiProviders;

    // Runtime (not persisted)
    bool showOutputLog = false;
    bool showMetricsPanel = false;
    bool showImGuiDemo = false;
};

// Default API configurations
inline std::vector<ApiProviderConfig> GetDefaultApiProviders() {
    return {
        {"xAI", "https://api.x.ai/v1", "GROK_API_KEY", "grok-2", 60, 3},
        {"OpenAI", "https://api.openai.com/v1", "OPENAI_API_KEY", "gpt-4-turbo", 60, 3}
    };
}

} // namespace smith::config
```

### 7.2 ConfigManager

```cpp
// include/config/config_manager.h

#pragma once

#include "app_config.h"
#include "core/result.h"
#include <functional>
#include <filesystem>

namespace smith::config {

class ConfigManager {
public:
    using ChangeCallback = std::function<void(const AppConfig&)>;

    ConfigManager();
    ~ConfigManager();

    /**
     * Load configuration from file
     * Creates default config if file doesn't exist
     */
    core::Result<void> Load(const std::filesystem::path& path);

    /**
     * Save configuration to file
     * @param path Optional override path
     */
    core::Result<void> Save(const std::filesystem::path& path = "");

    /**
     * Get current configuration (const)
     */
    const AppConfig& Get() const { return m_config; }

    /**
     * Get mutable reference for modifications
     * Must call MarkDirty() after changes
     */
    AppConfig& Modify() { return m_config; }

    /**
     * Mark as modified (for auto-save)
     */
    void MarkDirty() { m_dirty = true; }
    bool IsDirty() const { return m_dirty; }

    /**
     * Register change callback
     * Returns ID for unregistration
     */
    int OnChange(ChangeCallback callback);
    void RemoveCallback(int id);

    /**
     * Enable file watching for hot reload
     */
    void EnableFileWatch(bool enable);

    /**
     * Get loaded file path
     */
    const std::filesystem::path& GetFilePath() const { return m_filePath; }

private:
    void ApplyDefaults();
    void NotifyChange();
    void WatchThread();

    AppConfig m_config;
    std::filesystem::path m_filePath;
    bool m_dirty = false;
    bool m_watchEnabled = false;

    std::vector<std::pair<int, ChangeCallback>> m_callbacks;
    int m_nextCallbackId = 0;

    std::thread m_watchThread;
    std::atomic<bool> m_watchRunning{false};
};

} // namespace smith::config
```

---

## 8. Agent System

### 8.1 Agent Data Structure

```cpp
// include/agent/agent.h

#pragma once

#include "agent_metrics.h"
#include "core/types.h"
#include <string>
#include <vector>
#include <chrono>
#include <optional>

namespace smith::agent {

enum class AgentType {
    ClaudeCode,
    Grok,
    ChatGPT,
    Cursor,
    Custom
};

enum class AgentStatus {
    Idle,       // Not started
    Starting,   // Launching process/connecting
    Running,    // Active
    Waiting,    // Awaiting user input
    Thinking,   // AI processing
    Error,      // Error occurred
    Stopped     // Explicitly stopped
};

struct GitInfo {
    std::string branch;
    std::string commitHash;
    int aheadCount = 0;
    int behindCount = 0;
    int uncommittedChanges = 0;
    bool isRepository = false;
    std::chrono::system_clock::time_point lastUpdated;
};

/**
 * Agent - Represents a single AI agent instance
 */
struct Agent {
    // === Identity (immutable) ===
    std::string id;
    std::string name;
    AgentType type;

    // === Configuration ===
    std::string workingDirectory;
    std::string command;
    std::vector<std::string> args;
    std::string terminalTheme;
    bool autoAcceptEdits = false;

    // API-specific
    std::string apiKeyEnvVar;
    std::string model;
    std::string systemPrompt;

    // === Runtime State ===
    AgentStatus status = AgentStatus::Idle;
    std::string statusMessage;
    int processId = -1;
    bool needsAttention = false;
    std::string lastError;

    // === Session ===
    std::chrono::system_clock::time_point sessionStart;
    std::chrono::system_clock::time_point lastActivity;

    // === Metrics ===
    AgentMetrics metrics;

    // === Git ===
    GitInfo gitInfo;

    // === Grid Position ===
    int gridRow = 0;
    int gridCol = 0;

    // === Factory ===
    static Agent Create(const std::string& name,
                       const std::string& workDir,
                       AgentType type);

    static Agent FromConfig(const config::AgentConfig& cfg);
    config::AgentConfig ToConfig() const;

    // === Helpers ===
    std::string GetTypeString() const;
    static AgentType ParseType(const std::string& str);
    bool RequiresTerminal() const;
    bool RequiresApiKey() const;
    std::chrono::seconds GetSessionDuration() const;
    bool IsActive() const;
};

} // namespace smith::agent
```

### 8.2 Agent Metrics

```cpp
// include/agent/agent_metrics.h

#pragma once

#include <string>
#include <chrono>
#include <optional>

namespace smith::agent {

/**
 * Token usage metrics
 */
struct TokenMetrics {
    int inputTokens = 0;
    int outputTokens = 0;
    int cacheReadTokens = 0;
    int cacheWriteTokens = 0;
    int totalTokens = 0;

    double inputCost = 0.0;
    double outputCost = 0.0;
    double totalCost = 0.0;

    std::string model;
    bool valid = false;

    std::chrono::system_clock::time_point timestamp;

    void Reset() {
        inputTokens = outputTokens = cacheReadTokens = cacheWriteTokens = totalTokens = 0;
        inputCost = outputCost = totalCost = 0.0;
        model.clear();
        valid = false;
    }

    TokenMetrics& operator+=(const TokenMetrics& other) {
        inputTokens += other.inputTokens;
        outputTokens += other.outputTokens;
        cacheReadTokens += other.cacheReadTokens;
        cacheWriteTokens += other.cacheWriteTokens;
        totalTokens += other.totalTokens;
        inputCost += other.inputCost;
        outputCost += other.outputCost;
        totalCost += other.totalCost;
        return *this;
    }
};

/**
 * Rate limit information
 */
struct RateLimitInfo {
    int requestsRemaining = -1;
    int requestsLimit = -1;
    int tokensRemaining = -1;
    int tokensLimit = -1;
    std::chrono::system_clock::time_point resetsAt;
    bool valid = false;
};

/**
 * Session statistics
 */
struct SessionStats {
    int messageCount = 0;
    int userMessageCount = 0;
    int assistantMessageCount = 0;
    int toolCallCount = 0;
    int errorCount = 0;
    std::chrono::seconds totalDuration{0};
};

/**
 * Combined agent metrics
 */
struct AgentMetrics {
    TokenMetrics tokens;
    TokenMetrics sessionTokens;  // Cumulative for session
    RateLimitInfo rateLimit;
    SessionStats session;

    std::chrono::system_clock::time_point lastUpdated;

    void Reset() {
        tokens.Reset();
        sessionTokens.Reset();
        rateLimit = {};
        session = {};
    }
};

} // namespace smith::agent
```

### 8.3 Agent Provider Interface

```cpp
// include/agent/agent_provider.h

#pragma once

#include "agent.h"
#include <functional>
#include <memory>
#include <optional>

namespace smith::agent {

/**
 * IAgentProvider - Abstract interface for agent implementations
 *
 * Each agent type implements this to provide:
 * - Default command/model configuration
 * - I/O processing hooks
 * - Metric extraction
 * - Status detection
 */
class IAgentProvider {
public:
    virtual ~IAgentProvider() = default;

    // === Identity ===

    virtual std::string GetName() const = 0;
    virtual AgentType GetType() const = 0;
    virtual std::string GetDefaultCommand() const = 0;
    virtual std::vector<std::string> GetDefaultArgs() const { return {}; }

    // === Capabilities ===

    virtual bool RequiresTerminal() const { return true; }
    virtual bool RequiresApiKey() const { return false; }
    virtual std::string GetApiKeyEnvVar() const { return ""; }
    virtual std::string GetDefaultModel() const { return ""; }
    virtual std::vector<std::string> GetAvailableModels() const { return {}; }

    // === Lifecycle ===

    virtual void OnAttach(Agent& agent) { (void)agent; }
    virtual void OnDetach(Agent& agent) { (void)agent; }
    virtual void OnStart(Agent& agent) { (void)agent; }
    virtual void OnStop(Agent& agent) { (void)agent; }

    // === Terminal I/O (for RequiresTerminal() == true) ===

    virtual std::string ProcessInput(const std::string& input) { return input; }
    virtual std::string ProcessOutput(const std::string& output) { return output; }

    // === Metrics ===

    virtual bool ParseMetrics(const std::string& output, AgentMetrics& metrics) {
        (void)output; (void)metrics;
        return false;
    }

    virtual std::string GetMetricsCommand() const { return ""; }

    virtual std::chrono::seconds GetMetricsRefreshInterval() const {
        return std::chrono::seconds(300);
    }

    // === Status Detection ===

    virtual std::optional<AgentStatus> DetectStatus(const std::string& output) {
        (void)output;
        return std::nullopt;
    }

    virtual bool DetectNeedsAttention(const std::string& output) {
        (void)output;
        return false;
    }

    // === API Providers (for RequiresTerminal() == false) ===

    using ResponseCallback = std::function<void(const std::string&, bool success)>;
    using StreamCallback = std::function<void(const std::string& chunk)>;

    virtual void SendMessage(Agent& agent,
                            const std::string& message,
                            ResponseCallback onComplete,
                            StreamCallback onStream = nullptr) {
        (void)agent; (void)message; (void)onComplete; (void)onStream;
    }

    virtual void CancelRequest(Agent& agent) { (void)agent; }
};

using ProviderFactory = std::function<std::unique_ptr<IAgentProvider>()>;

} // namespace smith::agent
```

### 8.4 Claude Provider Implementation

```cpp
// include/agent/providers/claude_provider.h

#pragma once

#include "agent/agent_provider.h"
#include <regex>

namespace smith::agent {

/**
 * ClaudeAgentProvider - Claude Code CLI integration
 *
 * Features:
 * - Parses /cost command output for token metrics
 * - Detects thinking/response states
 * - Tracks tool calls
 */
class ClaudeAgentProvider : public IAgentProvider {
public:
    std::string GetName() const override { return "Claude Code"; }
    AgentType GetType() const override { return AgentType::ClaudeCode; }
    std::string GetDefaultCommand() const override { return "claude"; }

    void OnStart(Agent& agent) override;
    std::string ProcessOutput(const std::string& output) override;

    bool ParseMetrics(const std::string& output, AgentMetrics& metrics) override;
    std::string GetMetricsCommand() const override { return "/cost"; }
    std::chrono::seconds GetMetricsRefreshInterval() const override {
        return std::chrono::seconds(300);
    }

    std::optional<AgentStatus> DetectStatus(const std::string& output) override;
    bool DetectNeedsAttention(const std::string& output) override;

private:
    bool ParseCostOutput(const std::string& output, TokenMetrics& metrics);
    bool ParseSessionCost(const std::string& line, TokenMetrics& metrics);
    bool ParseTokenCounts(const std::string& line, TokenMetrics& metrics);
    bool ParseModelInfo(const std::string& line, TokenMetrics& metrics);

    bool IsThinkingIndicator(const std::string& line) const;
    bool IsPromptIndicator(const std::string& line) const;
    bool IsToolCallIndicator(const std::string& line) const;
    bool IsErrorIndicator(const std::string& line) const;
    bool IsRateLimitIndicator(const std::string& line) const;

    // Compiled regex patterns
    static std::regex s_costPattern;
    static std::regex s_tokenPattern;
    static std::regex s_modelPattern;
    static std::regex s_rateLimitPattern;
};

} // namespace smith::agent
```

### 8.5 Grok Provider Implementation

```cpp
// include/agent/providers/grok_provider.h

#pragma once

#include "agent/agent_provider.h"
#include "network/api_client.h"
#include <memory>
#include <atomic>

namespace smith::agent {

/**
 * GrokAgentProvider - xAI Grok API integration
 *
 * Non-terminal provider using HTTP REST API.
 */
class GrokAgentProvider : public IAgentProvider {
public:
    GrokAgentProvider();
    ~GrokAgentProvider() override;

    std::string GetName() const override { return "Grok"; }
    AgentType GetType() const override { return AgentType::Grok; }
    std::string GetDefaultCommand() const override { return ""; }

    bool RequiresTerminal() const override { return false; }
    bool RequiresApiKey() const override { return true; }
    std::string GetApiKeyEnvVar() const override { return "GROK_API_KEY"; }
    std::string GetDefaultModel() const override { return "grok-2"; }

    std::vector<std::string> GetAvailableModels() const override {
        return {"grok-2", "grok-2-mini"};
    }

    void OnAttach(Agent& agent) override;
    void OnDetach(Agent& agent) override;

    void SendMessage(Agent& agent,
                    const std::string& message,
                    ResponseCallback onComplete,
                    StreamCallback onStream) override;
    void CancelRequest(Agent& agent) override;

private:
    std::string BuildRequestBody(const Agent& agent, const std::string& message);
    void ProcessResponse(const std::string& response, Agent& agent);

    std::unique_ptr<network::ApiClient> m_client;
    std::atomic<bool> m_cancelled{false};
};

} // namespace smith::agent
```

### 8.6 Agent Tracker

```cpp
// include/agent/agent_tracker.h

#pragma once

#include "agent.h"
#include <vector>
#include <memory>
#include <functional>
#include <mutex>

namespace smith::agent {

class AgentTracker {
public:
    using AgentCallback = std::function<void(Agent*)>;
    using StatusCallback = std::function<void(Agent*, AgentStatus oldStatus)>;

    AgentTracker();
    ~AgentTracker();

    // === Lifecycle ===

    Agent* CreateAgent(const std::string& name,
                      const std::string& workDir,
                      AgentType type);

    Agent* CreateAgentFromConfig(const config::AgentConfig& config);

    bool RemoveAgent(const std::string& id);
    void RemoveAllAgents();

    // === Lookup ===

    Agent* GetAgent(const std::string& id);
    Agent* GetAgentAt(int row, int col);
    std::vector<Agent*> GetAgents();
    std::vector<Agent*> GetActiveAgents();
    size_t GetAgentCount() const;

    // === Updates ===

    void Update(float deltaTime);
    void RefreshGitInfo(Agent* agent = nullptr);
    void RefreshMetrics(Agent* agent = nullptr);

    void SetAgentStatus(Agent* agent, AgentStatus status, const std::string& message = "");

    // === Events ===

    int OnAgentCreated(AgentCallback callback);
    int OnAgentRemoved(AgentCallback callback);
    int OnAgentStatusChanged(StatusCallback callback);
    void RemoveCallback(int id);

    // === Persistence ===

    void LoadFromConfig(const std::vector<config::AgentConfig>& configs);
    std::vector<config::AgentConfig> SaveToConfig() const;

    // === Aggregate Metrics ===

    TokenMetrics GetTotalTokenMetrics() const;
    double GetTotalCost() const;

private:
    std::vector<std::unique_ptr<Agent>> m_agents;
    mutable std::mutex m_mutex;

    std::vector<std::pair<int, AgentCallback>> m_createdCallbacks;
    std::vector<std::pair<int, AgentCallback>> m_removedCallbacks;
    std::vector<std::pair<int, StatusCallback>> m_statusCallbacks;
    int m_nextCallbackId = 1;

    float m_gitRefreshTimer = 0.0f;
    float m_metricsRefreshTimer = 0.0f;

    static constexpr float GIT_REFRESH_INTERVAL = 30.0f;
    static constexpr float METRICS_REFRESH_INTERVAL = 300.0f;
};

} // namespace smith::agent
```

### 8.7 Agent Utilities

```cpp
// include/agent/agent_utils.h

#pragma once

#include "agent.h"
#include "agent_metrics.h"
#include <memory>

namespace smith::agent {

/**
 * Metric parser interface
 */
class IMetricParser {
public:
    virtual ~IMetricParser() = default;
    virtual bool Parse(const std::string& input, AgentMetrics& metrics) = 0;
};

class ClaudeMetricParser : public IMetricParser {
public:
    bool Parse(const std::string& input, AgentMetrics& metrics) override;
private:
    bool ParseCostLine(const std::string& line, TokenMetrics& m);
    bool ParseTokenLine(const std::string& line, TokenMetrics& m);
};

class OpenAIMetricParser : public IMetricParser {
public:
    bool Parse(const std::string& jsonResponse, AgentMetrics& metrics) override;
};

/**
 * Agent utility functions
 */
namespace AgentUtils {
    // ID generation
    std::string GenerateId();

    // Type conversion
    std::string TypeToString(AgentType type);
    AgentType StringToType(const std::string& str);
    std::string StatusToString(AgentStatus status);

    // Formatting
    std::string FormatDuration(std::chrono::seconds duration);
    std::string FormatCost(double cost);
    std::string FormatTokenCount(int tokens);
    std::string FormatTimestamp(std::chrono::system_clock::time_point tp);

    // API key management
    bool HasApiKey(const std::string& envVar);
    std::string GetApiKey(const std::string& envVar);
    bool ValidateApiKey(const std::string& key, AgentType type);

    // Factory
    std::unique_ptr<IMetricParser> CreateMetricParser(AgentType type);
    std::unique_ptr<IAgentProvider> CreateProvider(AgentType type);
}

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
#include <memory>

#ifdef _WIN32
#include <Windows.h>
#endif

namespace smith::terminal {

using OutputCallback = std::function<void(const char* data, size_t length)>;
using ExitCallback = std::function<void(int exitCode)>;
using ErrorCallback = std::function<void(const std::string& error)>;

/**
 * ITerminal - Abstract terminal interface
 *
 * Implementations:
 * - ImGuiTerminal: Custom ANSI parser + ImGui rendering (fallback)
 * - WebViewTerminal: WebView2 + xterm.js (primary)
 */
class ITerminal {
public:
    virtual ~ITerminal() = default;

    // === Lifecycle ===

    virtual bool Launch(const std::string& command,
                       const std::vector<std::string>& args,
                       const std::string& workingDir,
                       int cols, int rows) = 0;

    virtual void Terminate() = 0;
    virtual bool IsRunning() const = 0;
    virtual int GetExitCode() const = 0;
    virtual int GetProcessId() const = 0;

    // === I/O ===

    virtual void Write(const std::string& data) = 0;
    virtual void Write(const char* data, size_t length) = 0;
    virtual void SetOutputCallback(OutputCallback cb) = 0;
    virtual void SetExitCallback(ExitCallback cb) = 0;
    virtual void SetErrorCallback(ErrorCallback cb) = 0;

    // === Size ===

    virtual void Resize(int cols, int rows) = 0;
    virtual int GetCols() const = 0;
    virtual int GetRows() const = 0;

    // === Rendering ===

    // For ImGui-based terminals
    virtual void Render(float x, float y, float width, float height) {
        (void)x; (void)y; (void)width; (void)height;
    }

    // For WebView-based terminals
#ifdef _WIN32
    virtual HWND GetNativeHandle() const { return nullptr; }
#endif
    virtual void SetBounds(int x, int y, int width, int height) {
        (void)x; (void)y; (void)width; (void)height;
    }
    virtual void SetVisible(bool visible) { (void)visible; }
    virtual void Focus() {}

    // === Capabilities ===

    virtual bool HasNativeRendering() const { return false; }
    virtual bool SupportsSelection() const { return false; }
    virtual std::string GetSelectedText() const { return ""; }
    virtual void ClearSelection() {}
    virtual void SelectAll() {}
    virtual void Copy() {}
    virtual void Paste(const std::string& text) { Write(text); }

    // === Theme ===

    virtual void SetTheme(const TerminalTheme& theme) = 0;
    virtual const TerminalTheme& GetTheme() const = 0;

    // === Scrolling ===

    virtual void Scroll(int deltaLines) { (void)deltaLines; }
    virtual void ScrollToTop() {}
    virtual void ScrollToBottom() {}
    virtual int GetScrollPosition() const { return 0; }
    virtual int GetScrollbackLines() const { return 0; }

    // === Search (future) ===

    virtual bool Find(const std::string& text, bool caseSensitive = false) {
        (void)text; (void)caseSensitive;
        return false;
    }
    virtual void FindNext() {}
    virtual void FindPrevious() {}
    virtual void ClearFind() {}
};

} // namespace smith::terminal
```

### 9.2 WebView Terminal

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
 * WebViewTerminal - WebView2 + xterm.js terminal
 *
 * Architecture:
 * 1. ConPTY provides pseudo-terminal for process I/O
 * 2. WebView2 hosts xterm.js for rendering
 * 3. JavaScript bridge connects the two
 */
class WebViewTerminal : public ITerminal {
public:
    WebViewTerminal();
    ~WebViewTerminal() override;

    /**
     * Initialize WebView2 (must call before Launch)
     * @param parentHwnd Parent window handle
     * @param resourcesPath Path to web/ resources
     * @return true if initialization started (async completion)
     */
    bool Initialize(HWND parentHwnd, const std::string& resourcesPath);

    /**
     * Check if WebView2 is ready
     */
    bool IsReady() const { return m_ready; }

    /**
     * Wait for WebView2 initialization (blocks)
     */
    bool WaitForReady(int timeoutMs = 5000);

    /**
     * Check if WebView2 is available on this system
     */
    static bool IsAvailable();

    // ITerminal implementation
    bool Launch(const std::string& command,
               const std::vector<std::string>& args,
               const std::string& workingDir,
               int cols, int rows) override;

    void Terminate() override;
    bool IsRunning() const override;
    int GetExitCode() const override;
    int GetProcessId() const override;

    void Write(const std::string& data) override;
    void Write(const char* data, size_t length) override;
    void SetOutputCallback(OutputCallback cb) override;
    void SetExitCallback(ExitCallback cb) override;
    void SetErrorCallback(ErrorCallback cb) override;

    void Resize(int cols, int rows) override;
    int GetCols() const override { return m_cols; }
    int GetRows() const override { return m_rows; }

#ifdef _WIN32
    HWND GetNativeHandle() const override { return m_hwnd; }
#endif
    void SetBounds(int x, int y, int width, int height) override;
    void SetVisible(bool visible) override;
    void Focus() override;

    bool HasNativeRendering() const override { return true; }
    bool SupportsSelection() const override { return true; }
    std::string GetSelectedText() const override;
    void ClearSelection() override;
    void SelectAll() override;
    void Copy() override;
    void Paste(const std::string& text) override;

    void SetTheme(const TerminalTheme& theme) override;
    const TerminalTheme& GetTheme() const override { return m_theme; }

    void Scroll(int deltaLines) override;
    void ScrollToTop() override;
    void ScrollToBottom() override;

private:
    void CreateWebView();
    void OnWebViewCreated(HRESULT hr);
    void LoadTerminalPage();
    void FlushPendingOutput();

    void SendToXterm(const std::string& type, const std::string& data);
    void ExecuteScript(const std::string& script);
    void OnWebMessage(const std::wstring& message);
    void OnPtyOutput(const char* data, size_t length);
    void OnPtyExit(int exitCode);

    // ConPTY for process I/O
    std::unique_ptr<ConPTYTerminal> m_pty;

#ifdef _WIN32
    // WebView2
    HWND m_hwnd = nullptr;
    Microsoft::WRL::ComPtr<ICoreWebView2Environment> m_environment;
    Microsoft::WRL::ComPtr<ICoreWebView2Controller> m_controller;
    Microsoft::WRL::ComPtr<ICoreWebView2> m_webView;
#endif

    // State
    std::atomic<bool> m_ready{false};
    std::atomic<bool> m_visible{true};
    int m_cols = 80;
    int m_rows = 24;
    TerminalTheme m_theme;
    std::string m_resourcesPath;

    // Output buffering (before WebView ready)
    std::queue<std::string> m_pendingOutput;
    std::mutex m_pendingMutex;

    // Selection cache
    mutable std::string m_cachedSelection;
    mutable std::mutex m_selectionMutex;

    // Callbacks
    OutputCallback m_outputCallback;
    ExitCallback m_exitCallback;
    ErrorCallback m_errorCallback;
};

} // namespace smith::terminal
```

### 9.3 Terminal Factory

```cpp
// include/terminal/terminal_factory.h

#pragma once

#include "terminal_interface.h"
#include <memory>

namespace smith::terminal {

enum class TerminalBackend {
    Auto,      // WebView2 if available, else ImGui
    WebView2,  // Force WebView2 (fails if unavailable)
    ImGui      // Force ImGui fallback
};

class TerminalFactory {
public:
    /**
     * Create a terminal instance
     */
    static std::unique_ptr<ITerminal> Create(
        TerminalBackend backend = TerminalBackend::Auto,
        void* parentHwnd = nullptr,
        const std::string& resourcesPath = "");

    /**
     * Get recommended backend
     */
    static TerminalBackend GetRecommended();

    /**
     * Check if WebView2 is available
     */
    static bool IsWebView2Available();

    /**
     * Get WebView2 version string
     */
    static std::string GetWebView2Version();
};

} // namespace smith::terminal
```

---

## 10. UI System

### 10.1 Grid Layout

```cpp
// include/ui/grid_layout.h

#pragma once

#include <vector>
#include <memory>
#include <functional>

namespace smith::agent { struct Agent; }

namespace smith::ui {

class AgentWindow;

class GridLayout {
public:
    using EmptySlotCallback = std::function<void(int row, int col)>;
    using FocusCallback = std::function<void(AgentWindow* window)>;

    GridLayout();
    ~GridLayout();

    // Grid management
    void SetSize(int rows, int cols);
    int GetRows() const { return m_rows; }
    int GetCols() const { return m_cols; }

    // Agent assignment
    void SetAgent(int row, int col, agent::Agent* agent);
    void RemoveAgent(int row, int col);
    agent::Agent* GetAgent(int row, int col);
    AgentWindow* GetWindow(int row, int col);

    // Rendering
    void Render();
    void HandleInput();

    // Focus
    void SetFocus(int row, int col);
    void FocusNext();
    void FocusPrevious();
    void FocusDirection(int dRow, int dCol);
    AgentWindow* GetFocusedWindow();
    int GetFocusedRow() const { return m_focusRow; }
    int GetFocusedCol() const { return m_focusCol; }

    // Fullscreen
    void ToggleFullscreen();
    void ExitFullscreen();
    bool IsFullscreen() const { return m_fullscreen; }

    // Callbacks
    void OnEmptySlotClicked(EmptySlotCallback cb) { m_emptySlotCb = cb; }
    void OnFocusChanged(FocusCallback cb) { m_focusCb = cb; }

private:
    void RenderNormal();
    void RenderFullscreen();
    void RenderEmptySlot(int row, int col, float x, float y, float w, float h);
    void UpdateBounds();

    int m_rows = 2;
    int m_cols = 2;
    int m_focusRow = 0;
    int m_focusCol = 0;
    bool m_fullscreen = false;

    std::vector<std::unique_ptr<AgentWindow>> m_windows;
    EmptySlotCallback m_emptySlotCb;
    FocusCallback m_focusCb;
};

} // namespace smith::ui
```

### 10.2 Agent Window

```cpp
// include/ui/agent_window.h

#pragma once

#include "terminal/terminal_interface.h"
#include <memory>
#include <string>

namespace smith::agent {
    struct Agent;
    class IAgentProvider;
}

namespace smith::ui {

class AgentWindow {
public:
    AgentWindow();
    ~AgentWindow();

    // Agent binding
    void SetAgent(agent::Agent* agent);
    agent::Agent* GetAgent() const { return m_agent; }
    void DetachAgent();

    // Terminal
    bool InitializeTerminal(void* parentHwnd = nullptr);
    terminal::ITerminal* GetTerminal() const { return m_terminal.get(); }
    void RestartTerminal();
    void StopTerminal();

    // Rendering
    void Render(float x, float y, float width, float height, bool focused);
    void HandleInput();

    // Focus
    void SetFocused(bool focused);
    bool IsFocused() const { return m_focused; }

    // Input
    void SendInput(const std::string& text);
    void RequestMetricsRefresh();

    // Theme
    void SetTheme(const std::string& themeName);
    std::string GetThemeName() const;

private:
    void RenderTopOverlay(float x, float y, float width);
    void RenderBottomOverlay(float x, float y, float width, float height);
    void RenderContextMenu();
    void RenderAgentInfoPopup();
    void RenderThemeMenu();

    void OnTerminalOutput(const char* data, size_t length);
    void OnTerminalExit(int exitCode);
    void ProcessOutputForMetrics(const std::string& output);

    agent::Agent* m_agent = nullptr;
    agent::IAgentProvider* m_provider = nullptr;
    std::unique_ptr<terminal::ITerminal> m_terminal;

    bool m_focused = false;
    bool m_showInfo = false;
    bool m_showThemeMenu = false;

    std::string m_outputBuffer;
    float m_lastMetricsRequest = 0.0f;
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
#if defined(_WIN32)
    #define SMITH_PLATFORM_WINDOWS 1
    #define SMITH_PLATFORM_NAME "Windows"
#elif defined(__APPLE__)
    #define SMITH_PLATFORM_MACOS 1
    #define SMITH_PLATFORM_NAME "macOS"
#elif defined(__linux__)
    #define SMITH_PLATFORM_LINUX 1
    #define SMITH_PLATFORM_NAME "Linux"
#endif

// Architecture
#if defined(_M_X64) || defined(__x86_64__)
    #define SMITH_ARCH_X64 1
#elif defined(_M_ARM64) || defined(__aarch64__)
    #define SMITH_ARCH_ARM64 1
#endif

// Debug mode
#if defined(_DEBUG) || defined(DEBUG) || !defined(NDEBUG)
    #define SMITH_DEBUG 1
#endif

#include <string>

namespace smith::platform {

const char* GetPlatformName();
const char* GetArchitecture();
std::string GetOsVersion();
std::string GetExecutableDir();
std::string GetUserDataDir();
std::string GetTempDir();

bool OpenUrl(const std::string& url);
bool OpenFolder(const std::string& path);
bool OpenFileWith(const std::string& path, const std::string& app = "");

std::string GetEnv(const std::string& name);
bool SetEnv(const std::string& name, const std::string& value);

} // namespace smith::platform
```

---

## 12. Error Handling Strategy

### 12.1 Result Type

```cpp
// include/core/result.h

#pragma once

#include <variant>
#include <string>
#include <optional>

namespace smith::core {

/**
 * Error - Structured error information
 */
struct Error {
    enum class Code {
        None = 0,
        InvalidArgument,
        NotFound,
        AlreadyExists,
        PermissionDenied,
        Timeout,
        NetworkError,
        ParseError,
        IoError,
        SystemError,
        Unknown
    };

    Code code = Code::Unknown;
    std::string message;
    std::string details;
    std::string source;  // File:line or component

    Error() = default;
    Error(Code c, std::string msg) : code(c), message(std::move(msg)) {}

    bool IsOk() const { return code == Code::None; }
    operator bool() const { return !IsOk(); }  // true if error

    std::string ToString() const;
};

/**
 * Result<T, E> - Either a value or an error
 */
template<typename T, typename E = Error>
class Result {
public:
    // Success constructors
    Result(const T& value) : m_data(value) {}
    Result(T&& value) : m_data(std::move(value)) {}

    // Error constructors
    Result(const E& error) : m_data(error) {}
    Result(E&& error) : m_data(std::move(error)) {}

    bool IsOk() const { return std::holds_alternative<T>(m_data); }
    bool IsError() const { return std::holds_alternative<E>(m_data); }
    operator bool() const { return IsOk(); }

    T& Value() { return std::get<T>(m_data); }
    const T& Value() const { return std::get<T>(m_data); }

    E& Error() { return std::get<E>(m_data); }
    const E& Error() const { return std::get<E>(m_data); }

    T ValueOr(const T& defaultValue) const {
        return IsOk() ? Value() : defaultValue;
    }

    template<typename F>
    auto Map(F&& f) -> Result<decltype(f(std::declval<T>())), E> {
        if (IsOk()) return f(Value());
        return Error();
    }

private:
    std::variant<T, E> m_data;
};

// Specialization for void
template<typename E>
class Result<void, E> {
public:
    Result() : m_error(std::nullopt) {}
    Result(const E& error) : m_error(error) {}

    bool IsOk() const { return !m_error.has_value(); }
    bool IsError() const { return m_error.has_value(); }
    operator bool() const { return IsOk(); }

    E& Error() { return *m_error; }
    const E& Error() const { return *m_error; }

private:
    std::optional<E> m_error;
};

// Helper functions
template<typename T>
Result<T> Ok(T&& value) { return Result<T>(std::forward<T>(value)); }

inline Result<void> Ok() { return Result<void>(); }

template<typename T = void>
Result<T> Err(Error::Code code, const std::string& message) {
    return Result<T>(Error(code, message));
}

} // namespace smith::core
```

### 12.2 Error Handling Guidelines

1. **Use Result<T>** for operations that can fail
2. **Log errors** at the point of failure with context
3. **Propagate errors** up the call stack
4. **Handle errors** at appropriate boundaries (UI, API)
5. **Never swallow errors** silently
6. **Use SMITH_ASSERT** for programming errors (invariants)
7. **Use SMITH_VERIFY** for runtime checks that shouldn't fail

---

## 13. Threading Model

### 13.1 Thread Responsibilities

| Thread | Purpose | Components |
|--------|---------|------------|
| **Main** | UI rendering, event loop | AppBase, ImGui, GridLayout |
| **ConPTY Read** | Terminal output reading | Per-terminal background thread |
| **HTTP** | API requests | cpp-httplib internal pool |
| **Git** | Git operations | Background tasks |
| **File Watch** | Config hot reload | ConfigManager watch thread |

### 13.2 Thread Safety Rules

1. **UI operations**: Main thread only
2. **Agent modification**: Protected by mutex in AgentTracker
3. **Logging**: Thread-safe (internal mutex)
4. **Callbacks**: Always marshaled to main thread
5. **Terminal I/O**: Thread-safe write, callback on read thread

### 13.3 Main Thread Marshaling

```cpp
// include/core/main_thread.h

#pragma once

#include <functional>
#include <queue>
#include <mutex>

namespace smith::core {

class MainThreadDispatcher {
public:
    static MainThreadDispatcher& Instance();

    /**
     * Queue a function to run on main thread
     */
    void Post(std::function<void()> fn);

    /**
     * Process queued functions (call from main loop)
     */
    void ProcessQueue();

private:
    std::queue<std::function<void()>> m_queue;
    std::mutex m_mutex;
};

// Helper macro
#define SMITH_MAIN_THREAD(fn) \
    ::smith::core::MainThreadDispatcher::Instance().Post(fn)

} // namespace smith::core
```

---

## 14. Security Considerations

### 14.1 API Key Management

1. **Never store keys in code or config files**
2. **Use environment variables** (GROK_API_KEY, OPENAI_API_KEY)
3. **Mask keys in logs** (show only last 4 characters)
4. **Consider Windows Credential Manager** for persistent storage

### 14.2 Process Isolation

1. **Working directory restrictions**: Validate paths
2. **Command injection prevention**: Sanitize agent commands
3. **No shell=true**: Launch processes directly

### 14.3 Network Security

1. **HTTPS only** for API endpoints
2. **Certificate validation** enabled
3. **No sensitive data in URLs**

---

## 15. Testing Infrastructure

### 15.1 Test Categories

| Category | Purpose | Tools |
|----------|---------|-------|
| Unit | Individual components | GoogleTest |
| Integration | Component interaction | GoogleTest + mocks |
| E2E | Full application flows | Manual + scripts |

### 15.2 Mock Objects

```cpp
// tests/mocks/mock_terminal.h

#pragma once

#include "terminal/terminal_interface.h"
#include <gmock/gmock.h>

namespace smith::testing {

class MockTerminal : public terminal::ITerminal {
public:
    MOCK_METHOD(bool, Launch, (const std::string&, const std::vector<std::string>&,
                               const std::string&, int, int), (override));
    MOCK_METHOD(void, Terminate, (), (override));
    MOCK_METHOD(bool, IsRunning, (), (const, override));
    MOCK_METHOD(void, Write, (const std::string&), (override));
    // ... etc
};

} // namespace smith::testing
```

### 15.3 Test Fixtures

```cpp
// tests/fixtures/

// sample_config.json - Valid config for testing
// claude_cost_output.txt - Sample /cost command output
// api_responses/grok_success.json - Sample API response
// api_responses/grok_error.json - Sample error response
```

---

## 16. Build System

### 16.1 CMakeLists.txt (Root)

```cmake
cmake_minimum_required(VERSION 3.16)
project(AgentSmith VERSION 2.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Output
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR}/bin)

# Options
option(SMITH_BUILD_TESTS "Build tests" ON)
option(SMITH_USE_WEBVIEW2 "Enable WebView2" ON)
option(SMITH_ENABLE_ASAN "Enable AddressSanitizer" OFF)

# Modules
include(cmake/CompilerFlags.cmake)
include(cmake/Dependencies.cmake)

# Sources
file(GLOB_RECURSE SOURCES src/*.cpp)
file(GLOB_RECURSE HEADERS include/*.h)
list(FILTER SOURCES EXCLUDE REGEX "main\\.cpp$")

# Library
add_library(smith_lib STATIC ${SOURCES} ${HEADERS})
target_include_directories(smith_lib PUBLIC include)
target_link_libraries(smith_lib PUBLIC
    imgui glfw OpenGL::GL
    nlohmann_json::nlohmann_json
    spdlog::spdlog
    httplib::httplib
)

if(WIN32 AND SMITH_USE_WEBVIEW2)
    target_compile_definitions(smith_lib PUBLIC SMITH_USE_WEBVIEW2=1)
    # WebView2 linking...
endif()

# Executable
add_executable(agent_smith src/main.cpp)
target_link_libraries(agent_smith PRIVATE smith_lib)
if(WIN32)
    set_target_properties(agent_smith PROPERTIES WIN32_EXECUTABLE TRUE)
endif()

# Resources
add_custom_command(TARGET agent_smith POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/resources
        ${CMAKE_SOURCE_DIR}/bin/resources
)

# Tests
if(SMITH_BUILD_TESTS)
    include(cmake/Testing.cmake)
endif()
```

---

## 17. CI/CD Pipeline

### 17.1 GitHub Actions Workflow

```yaml
# .github/workflows/build.yml

name: Build

on:
  push:
    branches: [main, develop]
  pull_request:
    branches: [main]

jobs:
  build-windows:
    runs-on: windows-latest
    steps:
      - uses: actions/checkout@v4

      - name: Configure CMake
        run: cmake -B build -G "Visual Studio 17 2022"

      - name: Build
        run: cmake --build build --config Release

      - name: Test
        run: ctest --test-dir build -C Release --output-on-failure

      - name: Upload artifacts
        uses: actions/upload-artifact@v4
        with:
          name: agent_smith-windows
          path: bin/agent_smith.exe
```

---

## 18. Implementation Phases

### Phase 1: Foundation (Critical)

**Deliverables:**
- [ ] New folder structure created
- [ ] CMake modules (Dependencies, CompilerFlags, Testing)
- [ ] SMITH_LOG logging system with sinks
- [ ] AppBase framework class
- [ ] Application singleton inheriting AppBase
- [ ] Basic unit tests passing

**Files to create:** ~20

### Phase 2: Agent Abstraction (High)

**Deliverables:**
- [ ] Agent struct with metrics
- [ ] IAgentProvider interface
- [ ] AgentProviderRegistry
- [ ] ClaudeAgentProvider with /cost parsing
- [ ] AgentTracker refactored
- [ ] Unit tests for metrics parsing

**Files to create:** ~15

### Phase 3: Terminal Abstraction (High)

**Deliverables:**
- [ ] ITerminal interface
- [ ] ConPTYTerminal implements ITerminal
- [ ] WebViewTerminal with xterm.js
- [ ] TerminalFactory with auto-detection
- [ ] xterm.js resources bundled
- [ ] Theme system working

**Files to create:** ~10

### Phase 4: API Providers (Medium)

**Deliverables:**
- [ ] GrokAgentProvider
- [ ] ChatGPTAgentProvider
- [ ] HTTP client wrapper
- [ ] Rate limiting
- [ ] Chat UI for API agents

**Files to create:** ~10

### Phase 5: Polish (Medium)

**Deliverables:**
- [ ] 70%+ test coverage
- [ ] Performance optimized
- [ ] Documentation complete
- [ ] CI/CD pipeline

### Phase 6: Cleanup (Low)

**Deliverables:**
- [ ] Legacy code removed
- [ ] Release build tested
- [ ] Installation tested

---

## 19. File Manifest

### New Files to Create: 65+

**cmake/** (3 files)
**include/core/** (7 files)
**include/logging/** (3 files)
**include/config/** (3 files)
**include/agent/** (7 files)
**include/agent/providers/** (5 files)
**include/terminal/** (6 files)
**include/ui/** (8 files)
**include/network/** (4 files)
**include/platform/** (6 files)
**include/utils/** (6 files)
**src/** (mirrors include)
**tests/** (20+ files)
**resources/web/** (6 files)

### External Resources to Download

| Resource | URL | Size |
|----------|-----|------|
| xterm.js | jsdelivr CDN | ~150KB |
| xterm.css | jsdelivr CDN | ~10KB |
| xterm-addon-fit.js | jsdelivr CDN | ~5KB |
| xterm-addon-web-links.js | jsdelivr CDN | ~5KB |
| JetBrains Mono | jetbrains.com | ~200KB |

---

## 20. Migration Guide

### Step 1: Create Structure (Non-Breaking)
Add new folders alongside existing code.

### Step 2: Implement Foundation
AppBase, Logger work independently.

### Step 3: Gradual Migration
Move files one-by-one, update includes.

### Step 4: Integration
Wire new systems into Application.

### Step 5: Cleanup
Remove old files once fully migrated.

---

## 21. Risk Assessment

| Risk | Impact | Probability | Mitigation |
|------|--------|-------------|------------|
| WebView2 unavailable | High | Low | ImGui fallback |
| API rate limits | Medium | Medium | Rate limiter, backoff |
| Breaking changes | High | Medium | Incremental migration |
| Performance regression | Medium | Low | Profiling, benchmarks |

---

## 22. Appendices

### A. Claude /cost Output Format

```
╭─────────────────────────────────────────────────────────────╮
│ Token Usage                                                  │
├──────────────────────────────┬──────────────────────────────┤
│ Input tokens                 │                       12,345 │
│ Output tokens                │                        6,789 │
│ Cache read tokens            │                        1,234 │
│ Cache write tokens           │                          567 │
├──────────────────────────────┼──────────────────────────────┤
│ Total tokens                 │                       20,935 │
╰──────────────────────────────┴──────────────────────────────╯

Session cost: $0.1234
Model: claude-sonnet-4-20250514
```

### B. API Response Formats

**xAI Grok:**
```json
{
  "id": "chatcmpl-...",
  "choices": [{"message": {"content": "..."}}],
  "usage": {"prompt_tokens": 10, "completion_tokens": 20}
}
```

**OpenAI:**
```json
{
  "id": "chatcmpl-...",
  "choices": [{"message": {"content": "..."}}],
  "usage": {"prompt_tokens": 10, "completion_tokens": 20, "total_tokens": 30}
}
```

### C. Terminal Theme JSON

```json
{
  "name": "Catppuccin Macchiato",
  "background": "#24273A",
  "foreground": "#CAD3F5",
  "cursor": "#F4DBD6",
  "selection": "#5B6078",
  "ansi": ["#494D64", "#ED8796", "#A6DA95", "#EED49F",
           "#8AADF4", "#F5BDE6", "#8BD5CA", "#B8C0E0",
           "#5B6078", "#ED8796", "#A6DA95", "#EED49F",
           "#8AADF4", "#F5BDE6", "#8BD5CA", "#A5ADCB"]
}
```

---

*Document Version: 2.0*
*Ready for Implementation Manager consumption*

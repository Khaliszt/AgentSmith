# AgentSmith v2.0 Implementation Roadmap

> **Document Type:** Implementation Plan
> **Version:** 1.0
> **Created:** 2026-01-27
> **Author:** Implementation Manager Agent
> **Status:** Ready for Execution

---

## Executive Summary

This document provides a detailed, executable implementation roadmap for the AgentSmith v2.0 architecture overhaul. The plan transforms the existing monolithic v1.x codebase into a modular, testable, and extensible multi-agent mission control interface.

### Key Transformation Goals

| Aspect | Current State (v1.x) | Target State (v2.0) |
|--------|---------------------|---------------------|
| Architecture | Monolithic App class | AppBase framework + modular subsystems |
| Terminal | Custom ANSI parser (incomplete) | WebView2 + xterm.js (native quality) |
| Agent Types | ClaudeCode only (effectively) | ClaudeCode, Grok, ChatGPT, Cursor, Custom |
| Logging | Scattered console output | Unified SMITH_LOG with sinks |
| Testing | None | GoogleTest with 70%+ coverage |
| Dependencies | GLFW, ImGui, JSON | +spdlog, WebView2, cpp-httplib, GoogleTest |

### Overall Risk Assessment

**Overall Risk Level: MEDIUM-HIGH**

- **Primary Risk:** WebView2 integration complexity
- **Mitigation:** Phased approach with ImGui fallback always available
- **Confidence Level:** High (well-defined interfaces enable parallel development)

---

## Phase Overview

| Phase | Name | Duration | Risk | Dependencies |
|-------|------|----------|------|--------------|
| 1 | Foundation | 3-4 days | Medium | None |
| 2 | Agent Abstraction | 2-3 days | Low | Phase 1 |
| 3 | Terminal Abstraction | 4-5 days | High | Phase 1 |
| 4 | API Providers | 3-4 days | Medium | Phases 2, 3 |
| 5 | Polish & Testing | 2-3 days | Low | Phases 1-4 |
| 6 | Cleanup & Release | 1-2 days | Low | Phase 5 |

**Total Estimated Duration:** 15-21 days

---

## Critical Path Analysis

```
Phase 1 (Foundation)
    |
    +---> Phase 2 (Agent Abstraction) ----+
    |                                      |
    +---> Phase 3 (Terminal Abstraction) --+--> Phase 4 (API Providers)
                                                       |
                                                       v
                                               Phase 5 (Polish)
                                                       |
                                                       v
                                               Phase 6 (Cleanup)
```

**Critical Path:** Phase 1 -> Phase 3 -> Phase 4 -> Phase 5 -> Phase 6

Phases 2 and 3 can be **parallelized** after Phase 1 completes.

---

## Phase 1: Foundation

### Objective
Establish the new project structure, build system, logging infrastructure, and core framework (AppBase pattern).

### Duration Estimate
3-4 days (with buffer)

### Risk Level
**MEDIUM** - Build system changes can affect existing functionality

### Prerequisites
- Access to repository
- CMake 3.16+
- Visual Studio 2019+
- Internet access for FetchContent

---

### Task 1.1: Create Directory Structure

**Complexity:** Low
**Estimated Time:** 30 minutes
**Blocks:** All subsequent tasks

**Files to Create:**
```
C:\FarfadetsCorp\AgentSmith\
  cmake\                    (new directory)
  include\
    core\                   (new directory)
    logging\                (new directory)
    config\                 (new directory)
    agent\                  (new directory)
      providers\            (new directory)
    terminal\               (new directory)
    ui\                     (new directory)
      dialogs\              (new directory)
    network\                (new directory)
    platform\               (new directory)
    utils\                  (new directory)
  src\
    core\                   (new directory)
    logging\                (new directory)
    config\                 (new directory)
    agent\                  (new directory)
      providers\            (new directory)
    terminal\               (new directory)
    ui\                     (new directory)
      dialogs\              (new directory)
    network\                (new directory)
    platform\               (new directory)
      windows\              (new directory)
    utils\                  (new directory)
  tests\                    (new directory)
    unit\                   (new directory)
    integration\            (new directory)
    mocks\                  (new directory)
    fixtures\               (new directory)
  resources\
    web\                    (new directory)
  scripts\                  (new directory)
```

**Acceptance Criteria:**
- [ ] All directories exist
- [ ] No existing files were moved or deleted
- [ ] Build still works with existing code

**Implementation Steps:**
1. Create all new directories using mkdir or filesystem commands
2. Verify the existing build still compiles
3. Commit with message: "Create v2.0 directory structure"

**Rollback Procedure:**
1. Delete all newly created empty directories
2. No file modifications, so no further action needed

---

### Task 1.2: Create CMake Modules

**Complexity:** Medium
**Estimated Time:** 2 hours
**Blocks:** Tasks 1.4, 1.5, 1.6
**Depends On:** Task 1.1

**Files to Create:**

#### `cmake/Dependencies.cmake`
```cmake
# C:\FarfadetsCorp\AgentSmith\cmake\Dependencies.cmake

include(FetchContent)
set(FETCHCONTENT_QUIET OFF)

# === Existing Dependencies (keep versions consistent) ===

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

# cpp-httplib (header-only HTTP client)
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

    # Windows Implementation Library (COM helpers)
    FetchContent_Declare(
        wil
        GIT_REPOSITORY https://github.com/microsoft/wil.git
        GIT_TAG v1.0.231216.1
    )
endif()

# Make dependencies available
FetchContent_MakeAvailable(glfw imgui json spdlog googletest httplib)
if(WIN32)
    FetchContent_MakeAvailable(webview2 wil)
endif()
```

#### `cmake/CompilerFlags.cmake`
```cmake
# C:\FarfadetsCorp\AgentSmith\cmake\CompilerFlags.cmake

# Compiler-specific settings
if(MSVC)
    # MSVC settings
    add_compile_options(
        /W4                 # Warning level 4
        /permissive-        # Standards conformance
        /utf-8              # Source file encoding
        /MP                 # Multi-processor compilation
    )

    # Disable specific warnings
    add_compile_options(
        /wd4100             # Unreferenced formal parameter
        /wd4201             # Nameless struct/union
    )

    # Debug-specific
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_compile_options(/Zi /Od)
    else()
        add_compile_options(/O2 /DNDEBUG)
    endif()

elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(
        -Wall -Wextra -Wpedantic
        -Wno-unused-parameter
    )

    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_compile_options(-g -O0)
    else()
        add_compile_options(-O2 -DNDEBUG)
    endif()
endif()

# Platform definitions
if(WIN32)
    add_definitions(-DPLATFORM_WINDOWS -DUNICODE -D_UNICODE)
    add_definitions(-DNOMINMAX)  # Prevent Windows.h min/max macros
elseif(UNIX AND NOT APPLE)
    add_definitions(-DPLATFORM_LINUX)
elseif(APPLE)
    add_definitions(-DPLATFORM_MACOS)
endif()
```

#### `cmake/Testing.cmake`
```cmake
# C:\FarfadetsCorp\AgentSmith\cmake\Testing.cmake

enable_testing()

# Test executable
add_executable(agent_smith_tests
    tests/test_main.cpp
    # Unit tests will be added here as they're created
)

target_include_directories(agent_smith_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/tests
)

target_link_libraries(agent_smith_tests PRIVATE
    smith_lib
    GTest::gtest
    GTest::gmock
)

# Register with CTest
include(GoogleTest)
gtest_discover_tests(agent_smith_tests)
```

**Acceptance Criteria:**
- [ ] `cmake/Dependencies.cmake` exists and is valid CMake
- [ ] `cmake/CompilerFlags.cmake` exists and is valid CMake
- [ ] `cmake/Testing.cmake` exists and is valid CMake
- [ ] All FetchContent declarations use pinned versions

**Rollback Procedure:**
1. Delete the cmake/ directory contents
2. Restore original CMakeLists.txt if modified

---

### Task 1.3: Create Logging System

**Complexity:** Medium
**Estimated Time:** 3 hours
**Blocks:** All subsequent phases (logging used everywhere)
**Depends On:** Tasks 1.1, 1.2

**Files to Create:**

#### `include/logging/log_macros.h`
```cpp
// C:\FarfadetsCorp\AgentSmith\include\logging\log_macros.h

#pragma once

#include <string>

namespace smith::logging {

enum class LogLevel : int {
    Verbose = 0,
    Debug = 1,
    Info = 2,
    Warning = 3,
    Error = 4,
    Fatal = 5
};

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

const char* LogLevelToString(LogLevel level);
LogLevel StringToLogLevel(const std::string& str);

} // namespace smith::logging

// Forward declaration for macro
namespace smith::logging { class Logger; }

#define SMITH_LOG(Level, Category, Format, ...) \
    ::smith::logging::Logger::Instance().Log( \
        ::smith::logging::LogLevel::Level, \
        Category, \
        __FILE__, __LINE__, __FUNCTION__, \
        Format, ##__VA_ARGS__)

#define SMITH_VERBOSE(Cat, Fmt, ...) SMITH_LOG(Verbose, Cat, Fmt, ##__VA_ARGS__)
#define SMITH_DEBUG(Cat, Fmt, ...)   SMITH_LOG(Debug, Cat, Fmt, ##__VA_ARGS__)
#define SMITH_INFO(Cat, Fmt, ...)    SMITH_LOG(Info, Cat, Fmt, ##__VA_ARGS__)
#define SMITH_WARN(Cat, Fmt, ...)    SMITH_LOG(Warning, Cat, Fmt, ##__VA_ARGS__)
#define SMITH_ERROR(Cat, Fmt, ...)   SMITH_LOG(Error, Cat, Fmt, ##__VA_ARGS__)
#define SMITH_FATAL(Cat, Fmt, ...)   SMITH_LOG(Fatal, Cat, Fmt, ##__VA_ARGS__)

#define SMITH_ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            SMITH_FATAL("Assert", "Assertion failed: {} at {}:{}", \
                        message, __FILE__, __LINE__); \
            std::abort(); \
        } \
    } while (false)
```

#### `include/logging/logger.h`
```cpp
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
```

#### `include/logging/log_sink.h`
```cpp
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

    const std::deque<LogEntry>& GetEntries() const;
    void Clear();

private:
    std::deque<LogEntry> m_entries;
    size_t m_maxEntries;
    mutable std::mutex m_mutex;
};

} // namespace smith::logging
```

#### `src/logging/logger.cpp`
```cpp
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
```

#### `src/logging/log_sink.cpp`
```cpp
// C:\FarfadetsCorp\AgentSmith\src\logging\log_sink.cpp

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

const std::deque<LogEntry>& ImGuiSink::GetEntries() const {
    return m_entries;
}

void ImGuiSink::Clear() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_entries.clear();
}

} // namespace smith::logging
```

**Acceptance Criteria:**
- [ ] All logging header files exist in `include/logging/`
- [ ] All logging source files exist in `src/logging/`
- [ ] `SMITH_LOG` macro compiles and works
- [ ] File, Console, and ImGui sinks are implemented
- [ ] Category filtering works
- [ ] Log level filtering works

**Unit Tests to Create:**
- `tests/unit/logging/test_logger.cpp`
  - Test log level filtering
  - Test category filtering
  - Test multiple sinks
  - Test thread safety

**Rollback Procedure:**
1. Delete `include/logging/` contents
2. Delete `src/logging/` contents
3. Remove logging from CMakeLists.txt if added

---

### Task 1.4: Create Core Types and Result Type

**Complexity:** Low
**Estimated Time:** 1 hour
**Blocks:** Tasks 1.5, Phase 2, Phase 3
**Depends On:** Task 1.1

**Files to Create:**

#### `include/core/types.h`
```cpp
// C:\FarfadetsCorp\AgentSmith\include\core\types.h

#pragma once

#include <string>
#include <cstdint>

namespace smith::core {

// Forward declarations
class AppBase;
class Application;

} // namespace smith::core

namespace smith::agent {

enum class AgentType {
    ClaudeCode,
    Grok,
    ChatGPT,
    Cursor,
    Custom
};

enum class AgentStatus {
    Idle,
    Starting,
    Running,
    Waiting,
    Thinking,
    Error,
    Stopped
};

// Forward declarations
struct Agent;
struct AgentMetrics;
class IAgentProvider;
class AgentTracker;

} // namespace smith::agent

namespace smith::terminal {

class ITerminal;

} // namespace smith::terminal
```

#### `include/core/result.h`
```cpp
// C:\FarfadetsCorp\AgentSmith\include\core\result.h

#pragma once

#include <variant>
#include <string>
#include <optional>

namespace smith::core {

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
    std::string source;

    Error() = default;
    Error(Code c, std::string msg) : code(c), message(std::move(msg)) {}

    bool IsOk() const { return code == Code::None; }
    operator bool() const { return !IsOk(); }

    std::string ToString() const {
        std::string result = message;
        if (!details.empty()) result += " (" + details + ")";
        if (!source.empty()) result += " at " + source;
        return result;
    }
};

template<typename T, typename E = Error>
class Result {
public:
    Result(const T& value) : m_data(value) {}
    Result(T&& value) : m_data(std::move(value)) {}
    Result(const E& error) : m_data(error) {}
    Result(E&& error) : m_data(std::move(error)) {}

    bool IsOk() const { return std::holds_alternative<T>(m_data); }
    bool IsError() const { return std::holds_alternative<E>(m_data); }
    operator bool() const { return IsOk(); }

    T& Value() { return std::get<T>(m_data); }
    const T& Value() const { return std::get<T>(m_data); }
    T ValueOr(const T& defaultValue) const { return IsOk() ? Value() : defaultValue; }

    E& GetError() { return std::get<E>(m_data); }
    const E& GetError() const { return std::get<E>(m_data); }

    template<typename F>
    auto Map(F&& f) -> Result<decltype(f(std::declval<T>())), E> {
        if (IsOk()) return f(Value());
        return GetError();
    }

private:
    std::variant<T, E> m_data;
};

template<typename E>
class Result<void, E> {
public:
    Result() : m_error(std::nullopt) {}
    Result(const E& error) : m_error(error) {}

    bool IsOk() const { return !m_error.has_value(); }
    bool IsError() const { return m_error.has_value(); }
    operator bool() const { return IsOk(); }

    E& GetError() { return *m_error; }
    const E& GetError() const { return *m_error; }

private:
    std::optional<E> m_error;
};

template<typename T>
Result<T> Ok(T&& value) { return Result<T>(std::forward<T>(value)); }

inline Result<void> Ok() { return Result<void>(); }

template<typename T = void>
Result<T> Err(Error::Code code, const std::string& message) {
    return Result<T>(Error(code, message));
}

} // namespace smith::core
```

**Acceptance Criteria:**
- [ ] `include/core/types.h` exists with forward declarations
- [ ] `include/core/result.h` exists with Result<T> template
- [ ] Result<T> compiles with various types
- [ ] Error struct can hold error information

---

### Task 1.5: Create AppBase Framework Class

**Complexity:** High
**Estimated Time:** 4 hours
**Blocks:** Task 1.6
**Depends On:** Tasks 1.1, 1.2, 1.3, 1.4

**Files to Create:**

#### `include/core/app_base.h`
Full implementation as specified in architecture document section 5.2.

#### `src/core/app_base.cpp`
Implementation including:
- GLFW initialization
- OpenGL 3.3 Core context setup
- ImGui initialization with docking
- Main loop with frame timing
- Font loading
- Style application
- Callback registration

**Key Implementation Notes:**
1. Extract GLFW/ImGui setup from existing `src/app.cpp`
2. Keep existing functionality working during migration
3. Virtual methods allow Application to override

**Acceptance Criteria:**
- [ ] `include/core/app_base.h` matches architecture spec
- [ ] `src/core/app_base.cpp` compiles and links
- [ ] AppBase can be instantiated (for testing)
- [ ] Virtual methods are called in correct order
- [ ] Frame timing works correctly

**Unit Tests:**
- `tests/unit/core/test_app_base.cpp`
  - Test initialization sequence
  - Test virtual method calls
  - Test frame timing accuracy

**Rollback Procedure:**
1. Delete `include/core/app_base.h`
2. Delete `src/core/app_base.cpp`
3. Existing app.cpp remains unchanged

---

### Task 1.6: Create Application Singleton

**Complexity:** High
**Estimated Time:** 3 hours
**Blocks:** Phase 2, Phase 3
**Depends On:** Task 1.5

**Files to Create:**

#### `include/core/application.h`
Full implementation as specified in architecture document section 5.3.

#### `src/core/application.cpp`
Implementation including:
- Singleton pattern
- Subsystem initialization order
- Path discovery
- Integration with existing components (bridge)

**Migration Strategy:**
1. Application wraps existing App class initially
2. Gradually move functionality from App to Application
3. Eventually remove old App class

**Acceptance Criteria:**
- [ ] Application::Instance() returns singleton
- [ ] Application::Main() entry point works
- [ ] Subsystem initialization order is correct
- [ ] Paths are discovered correctly
- [ ] Existing functionality still works

---

### Task 1.7: Update Root CMakeLists.txt

**Complexity:** Medium
**Estimated Time:** 2 hours
**Blocks:** All compilation
**Depends On:** Tasks 1.2, 1.3, 1.4, 1.5, 1.6

**Changes to Make:**

Update `C:\FarfadetsCorp\AgentSmith\CMakeLists.txt`:

1. Include new cmake modules
2. Add smith_lib static library target
3. Add new source files to library
4. Keep existing executable working
5. Add test executable (conditional)

**Key Changes:**
```cmake
# Add at top after project()
include(cmake/CompilerFlags.cmake)
include(cmake/Dependencies.cmake)

# Option for tests
option(SMITH_BUILD_TESTS "Build tests" ON)
option(SMITH_USE_WEBVIEW2 "Enable WebView2" ON)

# Create static library with all sources
add_library(smith_lib STATIC
    # New v2.0 sources
    src/core/app_base.cpp
    src/core/application.cpp
    src/logging/logger.cpp
    src/logging/log_sink.cpp
    # ... existing sources migrated here
)

# Main executable links to library
add_executable(agent_smith ...)
target_link_libraries(agent_smith PRIVATE smith_lib)

# Tests (conditional)
if(SMITH_BUILD_TESTS)
    include(cmake/Testing.cmake)
endif()
```

**Acceptance Criteria:**
- [ ] CMake configure succeeds
- [ ] CMake build succeeds
- [ ] Tests compile (if enabled)
- [ ] Existing agent_smith.exe runs
- [ ] New smith_lib is created

**Rollback Procedure:**
1. Restore original CMakeLists.txt from git
2. All new files remain but are not compiled

---

### Task 1.8: Create Test Infrastructure

**Complexity:** Low
**Estimated Time:** 1 hour
**Depends On:** Task 1.2

**Files to Create:**

#### `tests/test_main.cpp`
```cpp
// C:\FarfadetsCorp\AgentSmith\tests\test_main.cpp

#include <gtest/gtest.h>

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
```

#### `tests/test_utils.h`
```cpp
// C:\FarfadetsCorp\AgentSmith\tests\test_utils.h

#pragma once

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <string>
#include <filesystem>

namespace smith::testing {

// Get path to test fixtures
inline std::filesystem::path GetFixturesPath() {
    return std::filesystem::path(__FILE__).parent_path() / "fixtures";
}

// Read fixture file contents
std::string ReadFixture(const std::string& filename);

} // namespace smith::testing
```

#### `tests/fixtures/sample_config.json`
```json
{
    "window": {
        "width": 1920,
        "height": 1080,
        "maximized": false
    },
    "grid": {
        "rows": 2,
        "cols": 2
    },
    "terminal": {
        "defaultTheme": "Catppuccin Macchiato",
        "fontSize": 14
    },
    "agents": []
}
```

**Acceptance Criteria:**
- [ ] test_main.cpp compiles
- [ ] GoogleTest links correctly
- [ ] `ctest` command runs (even with 0 tests)
- [ ] Fixture files exist

---

### Phase 1 Validation Checkpoint

Before proceeding to Phase 2, verify:

- [ ] All new directories exist
- [ ] CMake configures without errors
- [ ] CMake builds without errors
- [ ] Existing agent_smith.exe runs correctly
- [ ] SMITH_LOG macro works in test code
- [ ] Logger outputs to file and console
- [ ] AppBase can be instantiated
- [ ] Application singleton works
- [ ] Test executable runs (even if 0 tests pass)

**Validation Commands:**
```powershell
# Clean build
cd C:\FarfadetsCorp\AgentSmith
Remove-Item -Recurse -Force build
cmake -B build -G "Visual Studio 17 2022"
cmake --build build --config Release

# Run tests
ctest --test-dir build -C Release --output-on-failure

# Run application
.\bin\agent_smith.exe
```

---

## Phase 2: Agent Abstraction

### Objective
Create the agent abstraction layer with IAgentProvider interface, implement ClaudeAgentProvider with metrics parsing, and refactor AgentTracker.

### Duration Estimate
2-3 days

### Risk Level
**LOW** - Additive changes, existing code continues to work

### Prerequisites
- Phase 1 complete
- SMITH_LOG available

---

### Task 2.1: Create Agent Data Structures

**Complexity:** Medium
**Estimated Time:** 2 hours
**Blocks:** Tasks 2.2, 2.3, 2.4
**Depends On:** Phase 1

**Files to Create:**

#### `include/agent/agent_metrics.h`
Full implementation as specified in architecture document section 8.2.

#### `include/agent/agent.h`
Full implementation as specified in architecture document section 8.1.

#### `src/agent/agent.cpp`
Implementation of factory methods and helpers.

**Key Implementation Notes:**
- Migrate existing Agent struct from types.h
- Add new fields (metrics, API support)
- Keep backward compatibility during migration

**Acceptance Criteria:**
- [ ] Agent struct compiles
- [ ] AgentMetrics struct compiles
- [ ] Factory methods create valid agents
- [ ] FromConfig/ToConfig work correctly

---

### Task 2.2: Create IAgentProvider Interface

**Complexity:** Medium
**Estimated Time:** 2 hours
**Blocks:** Tasks 2.3, 2.4
**Depends On:** Task 2.1

**Files to Create:**

#### `include/agent/agent_provider.h`
Full implementation as specified in architecture document section 8.3.

**Acceptance Criteria:**
- [ ] Interface compiles
- [ ] All virtual methods have sensible defaults
- [ ] Terminal vs API distinction is clear

---

### Task 2.3: Implement ClaudeAgentProvider

**Complexity:** High
**Estimated Time:** 4 hours
**Depends On:** Tasks 2.1, 2.2

**Files to Create:**

#### `include/agent/providers/claude_provider.h`
As specified in architecture document section 8.4.

#### `src/agent/providers/claude_provider.cpp`

**Key Implementation:**
1. Parse `/cost` command output
2. Detect thinking/response states
3. Track tool calls
4. Extract token metrics

**Regex Patterns to Implement:**
```cpp
// Cost parsing
std::regex s_costPattern(R"(Session cost:\s*\$(\d+\.?\d*))");
std::regex s_tokenPattern(R"((\w+)\s+tokens\s*\│\s*([\d,]+))");
std::regex s_modelPattern(R"(Model:\s*(\S+))");
```

**Acceptance Criteria:**
- [ ] Parses sample /cost output correctly
- [ ] Extracts token counts
- [ ] Extracts session cost
- [ ] Detects model name
- [ ] Unit tests pass

**Unit Tests:**
- `tests/unit/agent/test_claude_provider.cpp`
  - Test cost parsing with sample output
  - Test token extraction
  - Test status detection

**Test Fixture:**
- `tests/fixtures/claude_cost_output.txt` (copy from architecture appendix)

---

### Task 2.4: Create Agent Registry

**Complexity:** Medium
**Estimated Time:** 2 hours
**Depends On:** Tasks 2.2, 2.3

**Files to Create:**

#### `include/agent/agent_registry.h`
```cpp
// C:\FarfadetsCorp\AgentSmith\include\agent\agent_registry.h

#pragma once

#include "agent_provider.h"
#include "core/types.h"
#include <memory>
#include <unordered_map>
#include <functional>

namespace smith::agent {

class AgentProviderRegistry {
public:
    using ProviderFactory = std::function<std::unique_ptr<IAgentProvider>()>;

    static AgentProviderRegistry& Instance();

    void RegisterProvider(AgentType type, ProviderFactory factory);
    std::unique_ptr<IAgentProvider> CreateProvider(AgentType type);
    IAgentProvider* GetProvider(AgentType type);

    std::vector<AgentType> GetRegisteredTypes() const;
    bool IsRegistered(AgentType type) const;

    // Register built-in providers
    void RegisterBuiltins();

private:
    AgentProviderRegistry() = default;

    std::unordered_map<AgentType, ProviderFactory> m_factories;
    std::unordered_map<AgentType, std::unique_ptr<IAgentProvider>> m_instances;
};

} // namespace smith::agent
```

#### `src/agent/agent_registry.cpp`

**Acceptance Criteria:**
- [ ] Registry compiles
- [ ] Can register provider factories
- [ ] Can create providers by type
- [ ] Built-in providers are registered

---

### Task 2.5: Create Agent Utilities

**Complexity:** Medium
**Estimated Time:** 2 hours
**Depends On:** Tasks 2.1, 2.3

**Files to Create:**

#### `include/agent/agent_utils.h`
As specified in architecture document section 8.7.

#### `src/agent/agent_utils.cpp`

**Key Functions:**
- ID generation (UUID or timestamp-based)
- Type/status string conversion
- Duration/cost/token formatting
- API key management

**Acceptance Criteria:**
- [ ] GenerateId() creates unique IDs
- [ ] Type conversions work bidirectionally
- [ ] Formatting functions produce readable output
- [ ] API key functions work with env vars

---

### Task 2.6: Refactor AgentTracker

**Complexity:** High
**Estimated Time:** 3 hours
**Depends On:** Tasks 2.1, 2.4, 2.5

**Files to Create:**

#### `include/agent/agent_tracker.h`
As specified in architecture document section 8.6.

#### `src/agent/agent_tracker.cpp`

**Migration Strategy:**
1. Create new AgentTracker in smith::agent namespace
2. Keep old AgentSmith::AgentsTracker working
3. Bridge between them during transition
4. Eventually remove old tracker

**Acceptance Criteria:**
- [ ] New tracker compiles in smith::agent
- [ ] Agent lifecycle methods work
- [ ] Callbacks fire correctly
- [ ] Thread-safe with mutex
- [ ] Config load/save works

---

### Phase 2 Validation Checkpoint

Before proceeding to Phase 3, verify:

- [ ] All agent headers compile
- [ ] ClaudeAgentProvider parses /cost output
- [ ] AgentRegistry registers and creates providers
- [ ] AgentTracker manages agent lifecycle
- [ ] Unit tests for metrics parsing pass
- [ ] Existing application still runs

**Validation Commands:**
```powershell
cmake --build build --config Release
ctest --test-dir build -C Release -R "agent" --output-on-failure
.\bin\agent_smith.exe
```

---

## Phase 3: Terminal Abstraction

### Objective
Create the terminal abstraction layer with ITerminal interface, implement WebView2 + xterm.js terminal, and maintain ImGui fallback.

### Duration Estimate
4-5 days

### Risk Level
**HIGH** - WebView2 integration is complex, COM programming required

### Prerequisites
- Phase 1 complete
- Phase 2 complete
- WebView2 SDK available via FetchContent

### Pre-Phase 3 Setup Actions

Before starting Phase 3 tasks, complete these setup actions:

1. **Create resources directory:**
   ```powershell
   mkdir resources/web
   ```

2. **Update CMakeLists.txt for terminal sources:**
   Add the following to smith_lib sources (will be done in Task 3.2):
   - `src/terminal/conpty_terminal.cpp`
   - `src/terminal/terminal_theme.cpp`
   - `src/terminal/webview_terminal.cpp`
   - `src/terminal/terminal_factory.cpp`

3. **Verify WebView2 in Dependencies.cmake:**
   WebView2 SDK should already be fetched via FetchContent (added in Phase 1).

---

### Architecture Note: ConPTY + xterm.js

**IMPORTANT:** ConPTY and xterm.js serve different purposes and work together:

| Component | Purpose |
|-----------|---------|
| **ConPTY** | Process management - launches CLI processes (claude, etc.), pipes I/O, handles resize signals |
| **xterm.js** | Rendering - displays terminal output, handles ANSI colors/escape codes, captures keyboard input |

**Data flow:**
```
User Keyboard → xterm.js (WebView) → ConPTY → CLI Process (claude)
                     ↑                  │
                     └──────────────────┘
                       Process Output
```

xterm.js replaces our custom ANSI parser and ImGui renderer, but ConPTY is still required to actually run terminal-based processes.

---

### Task Dependency Graph

```
Task 3.0 (Expand ITerminal) ────┬────> Task 3.1 (Themes) ────> Task 3.2 (ConPTY)
                                │                                      │
                                └────> Task 3.3 (xterm.js resources)   │
                                              │                        │
                                              └────────┬───────────────┘
                                                       │
                                                       v
                                               Task 3.4 (WebView)
                                                       │
                                                       v
                                               Task 3.5 (Factory)
                                                       │
                                                       v
                                               Task 3.6 (Provider Integration)
                                                       │
                                                       v
                                               Phase 3 Complete
```

**Note:** Task 3.7 (ImGui Fallback) is **deferred** to Phase 5 since WebView2 is widely available on Windows 10+.

---

### Task 3.0: Expand ITerminal Interface (NEW - CRITICAL)

**Complexity:** Medium
**Estimated Time:** 2 hours
**Blocks:** ALL other Phase 3 tasks
**Depends On:** Phase 2

**Problem:** The current `terminal/terminal_interface.h` is a minimal stub created in Phase 2 for ClaudeAgentProvider compatibility. It lacks critical methods from the architecture specification.

**Current Interface (incomplete):**
- `Write()`, `IsRunning()`, `SetOutputCallback()`, `Resize()`, `Clear()`, `GetSize()`

**Missing Methods (must add):**
- Process lifecycle: `Launch()`, `Terminate()`, `GetExitCode()`, `GetProcessId()`
- Callbacks: `SetExitCallback()`, `SetErrorCallback()`
- Window management: `GetNativeHandle()`, `SetBounds()`, `SetVisible()`, `Focus()`
- Clipboard: `GetSelectedText()`, `SelectAll()`, `Copy()`, `Paste()`
- Scrolling: `ScrollUp()`, `ScrollDown()`, `ScrollToTop()`, `ScrollToBottom()`
- Theme: `SetTheme()`, `GetTheme()`

**Files to Modify:**

#### `include/terminal/terminal_interface.h`
Expand to match architecture document section 9.1 (full ITerminal interface).

**Acceptance Criteria:**
- [ ] All methods from architecture spec present
- [ ] Process lifecycle methods defined
- [ ] Callback types for exit/error handling
- [ ] Native window handle support for WebView2
- [ ] Selection and clipboard support
- [ ] Compiles without errors

---

### Task 3.1: Create Terminal Theme System

**Complexity:** Medium
**Estimated Time:** 2 hours
**Blocks:** Tasks 3.2, 3.3
**Depends On:** Phase 1

**Files to Create:**

#### `include/terminal/terminal_interface.h`
Full implementation as specified in architecture document section 9.1.

#### `include/terminal/terminal_theme.h`
```cpp
// C:\FarfadetsCorp\AgentSmith\include\terminal\terminal_theme.h

#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace smith::terminal {

struct TerminalTheme {
    std::string name;
    std::string background;   // "#RRGGBB" format
    std::string foreground;
    std::string cursor;
    std::string selection;
    std::string ansi[16];     // 0-7 normal, 8-15 bright

    // Convert to JSON for xterm.js
    std::string ToJson() const;

    // Convert to ImGui colors (ABGR uint32_t)
    uint32_t GetBackgroundImGui() const;
    uint32_t GetForegroundImGui() const;
};

class TerminalThemeManager {
public:
    static TerminalThemeManager& Instance();

    void LoadThemes(const std::string& jsonPath);
    void LoadBuiltinThemes();

    const TerminalTheme& GetTheme(const std::string& name) const;
    const TerminalTheme& GetDefaultTheme() const;
    std::vector<std::string> GetThemeNames() const;

private:
    std::vector<TerminalTheme> m_themes;
    size_t m_defaultIndex = 0;
};

} // namespace smith::terminal
```

**Acceptance Criteria:**
- [ ] ITerminal interface compiles
- [ ] All methods have documentation
- [ ] Theme structure works with both backends

---

### Task 3.2: Migrate and Adapt ConPTY Terminal

**Complexity:** Medium-High
**Estimated Time:** 4 hours
**Depends On:** Tasks 3.0, 3.1

**Why ConPTY is Still Needed:**
ConPTY handles process management (launching `claude` CLI, piping I/O). xterm.js only handles rendering. They work together - ConPTY provides the PTY backend, xterm.js provides the display frontend.

**Files to Migrate:**

#### From `legacy/` to new structure:
- `legacy/include/conpty_terminal.h` → `include/terminal/conpty_terminal.h`
- `legacy/src/conpty_terminal.cpp` → `src/terminal/conpty_terminal.cpp`

**Key Changes:**
1. Inherit from ITerminal (implement full interface from Task 3.0)
2. Add `Launch()` method for process creation (replaces direct constructor launch)
3. Add exit callback support
4. Add theme support (store theme, pass to WebView via callbacks)
5. Implement all ITerminal methods
6. Update CMakeLists.txt to include new sources and link `kernel32` for ConPTY APIs

**Acceptance Criteria:**
- [ ] ConPTYTerminal implements full ITerminal interface
- [ ] `Launch()` method creates process attached to PTY
- [ ] Exit callbacks fire when process terminates
- [ ] Theme can be set and retrieved
- [ ] Existing terminal functionality preserved
- [ ] Compiles and links correctly

---

### Task 3.3: Download and Bundle xterm.js Resources

**Complexity:** Low
**Estimated Time:** 1 hour
**Blocks:** Task 3.4

**Files to Download:**

Create script `scripts/download_xterm.ps1`:
```powershell
# C:\FarfadetsCorp\AgentSmith\scripts\download_xterm.ps1

$webDir = "$PSScriptRoot\..\resources\web"
New-Item -ItemType Directory -Force -Path $webDir | Out-Null

$files = @{
    "https://cdn.jsdelivr.net/npm/xterm@5.3.0/lib/xterm.min.js" = "xterm.min.js"
    "https://cdn.jsdelivr.net/npm/xterm@5.3.0/css/xterm.css" = "xterm.css"
    "https://cdn.jsdelivr.net/npm/@xterm/addon-fit@0.10.0/lib/addon-fit.min.js" = "xterm-addon-fit.min.js"
    "https://cdn.jsdelivr.net/npm/@xterm/addon-web-links@0.11.0/lib/addon-web-links.min.js" = "xterm-addon-web-links.min.js"
}

foreach ($url in $files.Keys) {
    $dest = Join-Path $webDir $files[$url]
    Write-Host "Downloading $($files[$url])..."
    Invoke-WebRequest -Uri $url -OutFile $dest
}

Write-Host "xterm.js resources downloaded to $webDir"
```

**Files to Create:**

#### `resources/web/terminal.html`
```html
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <title>AgentSmith Terminal</title>
    <link rel="stylesheet" href="xterm.css">
    <style>
        body, html { margin: 0; padding: 0; height: 100%; overflow: hidden; }
        #terminal { height: 100%; }
    </style>
</head>
<body>
    <div id="terminal"></div>
    <script src="xterm.min.js"></script>
    <script src="xterm-addon-fit.min.js"></script>
    <script src="xterm-addon-web-links.min.js"></script>
    <script src="terminal.js"></script>
</body>
</html>
```

#### `resources/web/terminal.js`
```javascript
// C:\FarfadetsCorp\AgentSmith\resources\web\terminal.js

(function() {
    'use strict';

    const fitAddon = new FitAddon.FitAddon();
    const webLinksAddon = new WebLinksAddon.WebLinksAddon();

    const terminal = new Terminal({
        cursorBlink: true,
        fontSize: 14,
        fontFamily: 'JetBrains Mono, Consolas, monospace',
        theme: {
            background: '#24273a',
            foreground: '#cad3f5'
        }
    });

    terminal.loadAddon(fitAddon);
    terminal.loadAddon(webLinksAddon);
    terminal.open(document.getElementById('terminal'));
    fitAddon.fit();

    // Handle resize
    window.addEventListener('resize', () => fitAddon.fit());

    // Handle terminal input -> send to C++
    terminal.onData(data => {
        window.chrome.webview.postMessage({ type: 'input', data: data });
    });

    // Handle terminal resize -> send to C++
    terminal.onResize(size => {
        window.chrome.webview.postMessage({
            type: 'resize',
            cols: size.cols,
            rows: size.rows
        });
    });

    // C++ -> Terminal communication
    window.terminalApi = {
        write: function(data) {
            terminal.write(data);
        },
        clear: function() {
            terminal.clear();
        },
        reset: function() {
            terminal.reset();
        },
        setTheme: function(theme) {
            terminal.options.theme = theme;
        },
        getSelection: function() {
            return terminal.getSelection();
        },
        selectAll: function() {
            terminal.selectAll();
        },
        clearSelection: function() {
            terminal.clearSelection();
        },
        focus: function() {
            terminal.focus();
        },
        resize: function(cols, rows) {
            terminal.resize(cols, rows);
        },
        scrollToBottom: function() {
            terminal.scrollToBottom();
        },
        scrollToTop: function() {
            terminal.scrollToTop();
        }
    };

    // Notify C++ that terminal is ready
    window.chrome.webview.postMessage({ type: 'ready' });
})();
```

**Acceptance Criteria:**
- [ ] Download script works
- [ ] All xterm.js files present in resources/web/
- [ ] terminal.html loads correctly
- [ ] terminal.js provides API

---

### Task 3.4: Implement WebViewTerminal

**Complexity:** Very High
**Estimated Time:** 10 hours (increased from 8)
**Depends On:** Tasks 3.0, 3.1, 3.2, 3.3

**Architecture:**
WebViewTerminal combines WebView2 (for xterm.js rendering) with ConPTY (for process I/O):
```
WebViewTerminal
├── WebView2 Controller (renders xterm.js)
├── ConPTYTerminal (manages process I/O)
└── JavaScript Bridge (connects the two)
```

**Files to Create:**

#### `include/terminal/webview_terminal.h`
As specified in architecture document section 9.2.

#### `src/terminal/webview_terminal.cpp`

**Key Implementation Challenges:**
1. WebView2 COM initialization (async, callback-based)
2. JavaScript bridge for bidirectional communication
3. ConPTY integration - route output to xterm.js, input from xterm.js to ConPTY
4. Coordinate system mapping (GLFW -> Win32 for WebView2 positioning)
5. Thread safety for output buffering between ConPTY read thread and WebView2

**Implementation Order:**
1. Basic WebView2 creation and initialization
2. Load terminal.html from resources/web/
3. Implement Write() -> JavaScript (`window.terminalApi.write()`)
4. Implement input callback (JavaScript `postMessage` -> C++ -> ConPTY)
5. Integrate ConPTY for process management
6. Implement resize handling (both WebView2 and ConPTY)
7. Implement theme switching
8. Add selection/clipboard support

**Acceptance Criteria:**
- [ ] WebView2 initializes successfully
- [ ] terminal.html loads from resources/web/
- [ ] Write() displays text in xterm.js
- [ ] User input from xterm.js reaches ConPTY process
- [ ] Process output from ConPTY displays in xterm.js
- [ ] Terminal resizes correctly (both WebView2 and PTY)
- [ ] Themes can be changed at runtime
- [ ] Copy/paste works

**Integration Test:**
1. Create WebViewTerminal
2. Launch `cmd.exe` or `powershell.exe` via ConPTY
3. Verify prompt displays in xterm.js
4. Type commands, verify they execute
5. Resize window, verify terminal adjusts
6. Test copy/paste functionality

---

### Task 3.5: Create Terminal Factory

**Complexity:** Low
**Estimated Time:** 1 hour
**Depends On:** Tasks 3.2, 3.4

**Files to Create:**

#### `include/terminal/terminal_factory.h`
As specified in architecture document section 9.3.

#### `src/terminal/terminal_factory.cpp`

**Key Logic:**
```cpp
bool TerminalFactory::IsWebView2Available() {
    // Check if WebView2 runtime is installed
    LPWSTR version = nullptr;
    HRESULT hr = GetAvailableCoreWebView2BrowserVersionString(nullptr, &version);
    bool available = SUCCEEDED(hr) && version != nullptr;
    if (version) CoTaskMemFree(version);
    return available;
}

std::unique_ptr<ITerminal> TerminalFactory::Create(TerminalBackend backend, ...) {
    if (backend == TerminalBackend::Auto) {
        backend = IsWebView2Available() ? TerminalBackend::WebView2 : TerminalBackend::ImGui;
    }

    if (backend == TerminalBackend::WebView2) {
        auto term = std::make_unique<WebViewTerminal>();
        if (term->Initialize(parentHwnd, resourcesPath)) {
            return term;
        }
        // Fall back to ImGui if WebView2 fails
        SMITH_WARN(Category::Terminal, "WebView2 failed, falling back to ImGui");
    }

    return std::make_unique<ImGuiTerminal>();  // Fallback
}
```

**Acceptance Criteria:**
- [ ] Factory creates WebView2 terminal when available
- [ ] Factory falls back to ImGui when WebView2 unavailable
- [ ] WebView2 version can be queried

---

### Task 3.6: Update ClaudeAgentProvider Integration (NEW)

**Complexity:** Medium
**Estimated Time:** 2 hours
**Depends On:** Tasks 3.2, 3.5

**Problem:** ClaudeAgentProvider currently expects the terminal to already be running. It needs to be updated to use the new `Launch()` method and handle terminal lifecycle properly.

**Files to Modify:**

#### `include/agent/providers/claude_provider.h`
- Update terminal integration to use `Launch()`

#### `src/agent/providers/claude_provider.cpp`
- Call `terminal->Launch()` in `Start()` method
- Handle exit callback to detect process termination
- Wire up theme support

**Key Changes:**
1. In `Start()`: Call `m_terminal->Launch(command, args, workingDir, cols, rows)`
2. Set exit callback to transition to `AgentStatus::Stopped` when process exits
3. Set error callback for terminal errors
4. Pass theme from agent config to terminal

**Acceptance Criteria:**
- [ ] ClaudeAgentProvider uses `Launch()` to start process
- [ ] Exit callback properly transitions agent status
- [ ] Theme is applied from agent configuration
- [ ] Agent can be started, stopped, and restarted

---

### Task 3.7: Adapt Existing TerminalBuffer as Fallback (DEFERRED)

**Status:** DEFERRED TO PHASE 5

**Rationale:** WebView2 is available on Windows 10 version 1803+ and all Windows 11 systems. The ImGui fallback is a nice-to-have but not critical for initial v2.0 release. Deferring this allows focus on the primary WebView2 implementation.

**When to Implement:** Phase 5 (Polish & Testing) or if WebView2 issues are discovered during testing.

**Scope When Implemented:**
- Move `legacy/terminal_buffer.h/.cpp` to `include/terminal/` and `src/terminal/`
- Create `ImGuiTerminal` wrapper implementing ITerminal
- Integrate with TerminalFactory as fallback option

---

### Phase 3 Validation Checkpoint

Before proceeding to Phase 4, verify:

- [ ] ITerminal interface fully expanded (Task 3.0)
- [ ] Terminal themes work (Task 3.1)
- [ ] ConPTYTerminal migrated and implements ITerminal (Task 3.2)
- [ ] xterm.js resources downloaded and bundled (Task 3.3)
- [ ] WebViewTerminal initializes and renders via xterm.js (Task 3.4)
- [ ] TerminalFactory auto-selects WebView2 (Task 3.5)
- [ ] ClaudeAgentProvider uses new terminal interface (Task 3.6)
- [ ] Terminal displays process output correctly
- [ ] User input reaches process
- [ ] Terminal resizes correctly
- [ ] Copy/paste works

**Validation Test:**
1. Run application
2. Create agent with Claude Code type
3. Verify WebView2 + xterm.js terminal displays correctly
4. Type commands, verify they reach claude CLI
5. Verify `/cost` output is parsed and metrics updated
6. Resize window, verify terminal adjusts
7. Test copy/paste functionality
8. Stop and restart agent, verify lifecycle works

**Note:** ImGui fallback (Task 3.7) is deferred to Phase 5.

---

## Phase 4: Network & API Integration ✅ COMPLETED

### Objective
Create core infrastructure for API-based agents: HTTP client, API client wrapper, rate limiting, and conversation management.

### Duration Estimate
3-4 days (Actual: 3 days)

### Risk Level
**MEDIUM** - Network operations, dependency integration

### Status
**COMPLETED** - Commit: `d6dc0d4` - "Implement Phase 4: Network & API Integration"
- All 42 tests passing (100%)
- Ready for Phase 5 provider implementations

### Prerequisites
- Phase 2 complete (IAgentProvider interface)
- Phase 3 complete (terminal for terminal-based agents)

### What Was Completed

**Core Components:**
1. **HttpClient** - Low-level HTTP/HTTPS operations using cpp-httplib
   - Synchronous and asynchronous GET/POST
   - Streaming support (Server-Sent Events for LLM APIs)
   - Thread-safe cancellation
   - Configurable timeouts and SSL verification

2. **ApiClient** - OpenAI-compatible API client
   - Chat completion requests (sync + streaming)
   - Support for xAI Grok and OpenAI ChatGPT APIs
   - Automatic retry with exponential backoff
   - Token usage tracking and estimation
   - JSON request/response formatting

3. **RateLimiter** - Thread-safe rate limiting
   - Sliding window algorithm for request tracking
   - Token consumption tracking
   - Dynamic limit updates from API response headers
   - Time-until-available calculations

4. **Conversation** - Chat history management
   - Message role tracking (System, User, Assistant)
   - Token counting with auto-estimation
   - History retrieval (full and last N messages)
   - System prompt management
   - API format serialization (JSON)

**Testing:**
- 42/42 unit tests passing (100%)
- RateLimiter: 17 tests
- Conversation: 20 tests
- Integration with existing ConPTY terminal tests: 5 tests

**Build System:**
- Added cpp-httplib dependency via FetchContent
- Updated CMakeLists.txt with network source files
- All components compile cleanly on Windows with MSVC

### What Was Deferred to Phase 5
- GrokAgentProvider implementation (uses ApiClient)
- ChatGPTAgentProvider implementation (uses ApiClient)
- Chat UI components (ChatDisplay, AgentPanel, etc.)
- UI integration for API agents

The infrastructure is complete and tested. Phase 5 will build the provider implementations and UI on top of this foundation.

### Phase 4 Implementation Summary

**Files Created:**
- `include/network/http_client.h` + `.cpp` (230 + 413 lines)
- `include/network/api_client.h` + `.cpp` (307 + 504 lines)
- `include/network/rate_limiter.h` + `.cpp` (128 + 277 lines)
- `include/agent/conversation.h` + `.cpp` (176 + 157 lines)
- `tests/test_conversation.cpp` (254 lines - 20 tests)
- `tests/unit/network/test_rate_limiter.cpp` (210 lines - 17 tests)
- `tests/standalone_conversation_test.cpp` (189 lines)

**Modified Files:**
- `CMakeLists.txt` - Added network sources and httplib dependency
- `cmake/Testing.cmake` - Added new test files

**Total:** 13 files, 2,854 lines added

---

## Phase 5: API Agent Providers & UI Integration

### Objective
Complete the multi-agent system by implementing API-based agent providers (Grok, ChatGPT) and modernizing the UI to support both terminal and chat-based agents in a unified interface.

### Duration Estimate
4-5 weeks

### Risk Level
**MEDIUM-HIGH** - Complex UI integration, threading for streaming, provider implementation

### Prerequisites
- Phase 4 complete (Network & API infrastructure)
- Phase 3 complete (Terminal system)
- Phase 2 complete (Agent abstraction)

### What Will Be Implemented

**API Agent Providers:**
1. **ApiAgentProviderBase** - Shared foundation for all API providers
   - Common streaming logic with thread-safe chunk queue
   - Rate limiting integration
   - Conversation management
   - Metrics tracking from API responses

2. **GrokAgentProvider** - xAI Grok integration
   - xAI API endpoint configuration
   - Grok-specific model selection (grok-2-latest, grok-2-vision)
   - Streaming chat completions

3. **ChatGPTAgentProvider** - OpenAI ChatGPT integration
   - OpenAI API endpoint configuration
   - ChatGPT model selection (gpt-4-turbo, gpt-4o)
   - Streaming chat completions

**Modern UI Components:**
1. **ChatDisplay** - ImGui chat interface for API agents
   - Message history rendering with role indicators
   - Streaming message display with animated indicator
   - Auto-scroll and manual scroll control
   - Copy message/code blocks
   - Syntax highlighting for code (optional)

2. **AgentPanel** - Unified panel for both terminal and chat agents
   - Dual display mode (terminal OR chat)
   - Top bar with status, metrics, controls
   - Input area with send button
   - Keyboard shortcuts (Ctrl+Enter to send)
   - Focus management

3. **AgentGrid** - Multi-agent grid layout manager
   - Dynamic grid layout (2x2, 3x3, flexible)
   - Panel resizing and focus management
   - Fullscreen mode for single agent
   - Add/remove agent UI integration

4. **AddAgentDialog** - Modal dialog for creating agents
   - Agent type selection (ClaudeCode, Grok, ChatGPT)
   - Configuration inputs (API keys, models, working directory)
   - Input validation
   - Model dropdown for API agents

**Application Integration:**
- Wire all components together in Application class
- Setup agent lifecycle callbacks
- Implement menu bar with "Add Agent" button
- Config save/load for agents
- Shutdown cleanup

### Implementation Tasks

**Task 5.1:** Create ApiAgentProviderBase (Foundation)
- Duration: 2 days
- Files: `include/agent/providers/api_provider_base.h/cpp`
- Implements shared streaming, conversation, and rate limiting logic

**Task 5.2:** Implement GrokAgentProvider
- Duration: 1 day
- Files: `include/agent/providers/grok_provider.h/cpp`
- xAI API integration with Grok-specific configuration

**Task 5.3:** Implement ChatGPTAgentProvider
- Duration: 1 day
- Files: `include/agent/providers/chatgpt_provider.h/cpp`
- OpenAI API integration with ChatGPT-specific configuration

**Task 5.4:** Create ChatDisplay Component
- Duration: 2 days
- Files: `include/ui/chat_display.h/cpp`
- ImGui rendering of conversation history

**Task 5.5:** Modernize AgentPanel
- Duration: 2 days
- Files: `include/ui/agent_panel.h/cpp`
- Dual-mode panel supporting both terminal and chat

**Task 5.6:** Create AgentGrid Manager
- Duration: 2 days
- Files: `include/ui/agent_grid.h/cpp`
- Multi-agent layout and focus management

**Task 5.7:** Create AddAgentDialog
- Duration: 1 day
- Files: `include/ui/dialogs/add_agent_dialog.h/cpp`
- Modal dialog for agent creation

**Task 5.8:** Application Integration
- Duration: 2 days
- Modify: `include/core/application.h/cpp`
- Wire all subsystems together

**Task 5.9:** Registry Updates
- Duration: 0.5 day
- Modify: `src/agent/agent_registry.cpp`
- Register new providers

**Task 5.10:** Testing & Integration
- Duration: 3 days
- Files: Multiple test files
- Comprehensive testing of complete system

### Key Architecture Decisions

**1. Base Class for API Providers**
```cpp
ApiAgentProviderBase (shared logic)
    ├── GrokAgentProvider
    └── ChatGPTAgentProvider
```
Eliminates code duplication, provides consistent streaming behavior.

**2. Dual-Mode UI**
```
AgentPanel
    ├── ChatDisplay (for API agents)
    └── TerminalDisplay (for Claude Code)
```
One panel type handles both agent modes seamlessly.

**3. Thread-Safe Streaming**
- Background thread for API streaming (non-blocking)
- Chunk queue for main thread processing
- No UI rendering in background threads

### Success Criteria

Phase 5 is complete when:
- [ ] User can add Grok agent via dialog
- [ ] User can add ChatGPT agent via dialog
- [ ] User can chat with API agents and see streaming responses
- [ ] User can run Claude Code, Grok, and ChatGPT simultaneously
- [ ] All agents display in grid layout with proper focus management
- [ ] Metrics (tokens, cost) update in real-time
- [ ] Application persists agent configurations across restarts
- [ ] 60+ tests passing (including Phase 4's 42)
- [ ] No crashes during 30-minute multi-agent session
- [ ] Code review passes (thread safety, patterns, documentation)

### Files to Create (18 new files)

**Agent Providers:**
- `include/agent/providers/api_provider_base.h/cpp`
- `include/agent/providers/grok_provider.h/cpp`
- `include/agent/providers/chatgpt_provider.h/cpp`

**UI Components:**
- `include/ui/chat_display.h/cpp`
- `include/ui/agent_panel.h/cpp`
- `include/ui/agent_grid.h/cpp`
- `include/ui/dialogs/add_agent_dialog.h/cpp`

**Tests:**
- `tests/unit/agent/test_api_provider_base.cpp`
- `tests/unit/agent/test_grok_provider.cpp`
- `tests/unit/agent/test_chatgpt_provider.cpp`
- `tests/integration/test_multi_agent_system.cpp`

### Files to Modify (4 files)
- `src/agent/agent_registry.cpp`
- `include/core/application.h`
- `src/core/application.cpp`
- `cmake/Testing.cmake`

---

### Old Phase 4 Task Reference (For Historical Context)

The original IMPLEMENTATION_ROADMAP.md Phase 4 described implementing Grok/ChatGPT providers + Chat UI in 3-4 days. However, during implementation, we split this into:
- **Phase 4 (Completed):** Core infrastructure (HttpClient, ApiClient, RateLimiter, Conversation)
- **Phase 5 (Current):** Provider implementations + comprehensive UI modernization

This split allows for:
1. Better testing of infrastructure before building providers
2. More comprehensive UI design than originally planned
3. Proper architectural foundation for future providers

See Phase 5 above for the current implementation plan.

---

## Phase 6: Polish & Testing (Previously Phase 5)

### Objective
Achieve 70%+ test coverage, optimize performance, complete documentation, handle ImGui terminal fallback (deferred from Phase 3).

### Duration Estimate
2-3 days

### Risk Level
**LOW** - Additive work, no architectural changes

### Note
This phase was previously labeled "Phase 5" but has been renumbered to "Phase 6" after splitting the old Phase 4 into two phases (Phase 4: Infrastructure, Phase 5: Providers + UI).

---

#### Old Phase 4 Task Details (For Reference Only - Do Not Implement)

The sections below are from the original Phase 4 plan. These tasks have been superseded by the Phase 4 (completed) and Phase 5 (current) split described above.

<details>
<summary>Click to expand old Phase 4 task details (historical reference only)</summary>

### OLD Task 4.1: Create HTTP Client Wrapper
```cpp
// C:\FarfadetsCorp\AgentSmith\include\network\http_client.h

#pragma once

#include "core/result.h"
#include <string>
#include <map>
#include <functional>
#include <future>

namespace smith::network {

struct HttpResponse {
    int statusCode = 0;
    std::string body;
    std::map<std::string, std::string> headers;
    std::string error;
    bool success() const { return statusCode >= 200 && statusCode < 300; }
};

struct HttpRequest {
    std::string method = "GET";
    std::string url;
    std::string body;
    std::map<std::string, std::string> headers;
    int timeoutSeconds = 60;
};

using ResponseCallback = std::function<void(const HttpResponse&)>;
using StreamCallback = std::function<void(const std::string& chunk)>;

class HttpClient {
public:
    HttpClient();
    ~HttpClient();

    // Synchronous requests
    HttpResponse Get(const std::string& url,
                    const std::map<std::string, std::string>& headers = {});
    HttpResponse Post(const std::string& url,
                     const std::string& body,
                     const std::map<std::string, std::string>& headers = {});

    // Asynchronous requests
    std::future<HttpResponse> GetAsync(const std::string& url,
                                       const std::map<std::string, std::string>& headers = {});
    std::future<HttpResponse> PostAsync(const std::string& url,
                                        const std::string& body,
                                        const std::map<std::string, std::string>& headers = {});

    // Streaming (for SSE)
    void PostStream(const std::string& url,
                   const std::string& body,
                   const std::map<std::string, std::string>& headers,
                   StreamCallback onChunk,
                   ResponseCallback onComplete);

    void CancelAll();
    void SetTimeout(int seconds) { m_timeout = seconds; }

private:
    int m_timeout = 60;
    std::atomic<bool> m_cancelled{false};
};

} // namespace smith::network
```

#### `src/network/http_client.cpp`
Implementation using cpp-httplib.

**Acceptance Criteria:**
- [ ] GET/POST requests work
- [ ] Async requests work
- [ ] Streaming (SSE) works for chat responses
- [ ] Timeout handling works
- [ ] Cancellation works

---

### Task 4.2: Create API Client Base

**Complexity:** Medium
**Estimated Time:** 2 hours
**Depends On:** Task 4.1

**Files to Create:**

#### `include/network/api_client.h`
```cpp
// Base class for API providers (OpenAI-compatible)
class ApiClient {
public:
    struct Config {
        std::string baseUrl;
        std::string apiKey;
        std::string model;
        int timeoutSeconds = 60;
        int maxRetries = 3;
    };

    ApiClient(const Config& config);

    // Chat completion
    void SendChatCompletion(
        const std::vector<Message>& messages,
        const std::string& systemPrompt,
        ResponseCallback onComplete,
        StreamCallback onStream = nullptr);

    void Cancel();

    // Token counting (approximate)
    int EstimateTokens(const std::string& text) const;

protected:
    std::string BuildChatRequest(const std::vector<Message>& messages,
                                 const std::string& systemPrompt);
    void ParseChatResponse(const std::string& json, AgentMetrics& metrics);

    HttpClient m_http;
    Config m_config;
};
```

#### `include/network/rate_limiter.h`
```cpp
class RateLimiter {
public:
    RateLimiter(int requestsPerMinute, int tokensPerMinute);

    bool CanMakeRequest() const;
    void RecordRequest(int tokenCount);
    std::chrono::seconds TimeUntilAvailable() const;

    void UpdateFromHeaders(const std::map<std::string, std::string>& headers);

private:
    // Sliding window rate limiting
};
```

**Acceptance Criteria:**
- [ ] ApiClient sends chat completions
- [ ] Response parsing works
- [ ] RateLimiter prevents over-requesting
- [ ] Headers update rate limit info

---

### Task 4.3: Implement GrokAgentProvider

**Complexity:** Medium
**Estimated Time:** 3 hours
**Depends On:** Tasks 4.1, 4.2

**Files to Create:**

#### `include/agent/providers/grok_provider.h`
As specified in architecture document section 8.5.

#### `src/agent/providers/grok_provider.cpp`

**Key Implementation:**
- xAI API endpoint: https://api.x.ai/v1/chat/completions
- OpenAI-compatible request/response format
- Streaming support for real-time responses
- Token/cost tracking from response

**Test Fixture:**
- `tests/fixtures/api_responses/grok_success.json`
- `tests/fixtures/api_responses/grok_error.json`

**Acceptance Criteria:**
- [ ] Can send messages to Grok API
- [ ] Receives and parses responses
- [ ] Streaming works
- [ ] Metrics extracted from response
- [ ] Error handling works

---

### Task 4.4: Implement ChatGPTAgentProvider

**Complexity:** Medium
**Estimated Time:** 2 hours
**Depends On:** Tasks 4.1, 4.2

**Files to Create:**

#### `include/agent/providers/chatgpt_provider.h`
```cpp
class ChatGPTAgentProvider : public IAgentProvider {
    // Similar to GrokProvider but for OpenAI API
};
```

#### `src/agent/providers/chatgpt_provider.cpp`

**Key Differences from Grok:**
- API endpoint: https://api.openai.com/v1/chat/completions
- Different model names (gpt-4-turbo, etc.)
- Slightly different response format

**Acceptance Criteria:**
- [ ] OpenAI API integration works
- [ ] Different models supported
- [ ] Same interface as Grok provider

---

### Task 4.5: Create Chat UI Component

**Complexity:** Medium
**Estimated Time:** 3 hours
**Depends On:** Tasks 4.3, 4.4

**Files to Create:**

#### `include/ui/chat_window.h`
```cpp
// C:\FarfadetsCorp\AgentSmith\include\ui\chat_window.h

#pragma once

#include "agent/agent.h"
#include "agent/conversation.h"
#include <string>
#include <vector>

namespace smith::ui {

class ChatWindow {
public:
    ChatWindow();
    ~ChatWindow();

    void SetAgent(agent::Agent* agent);
    void Render(float x, float y, float width, float height, bool focused);
    void HandleInput();

    void AddMessage(const agent::Message& message);
    void ClearHistory();

    void SendMessage(const std::string& text);

private:
    void RenderMessages();
    void RenderInputArea();
    void RenderTypingIndicator();

    agent::Agent* m_agent = nullptr;
    std::vector<agent::Message> m_messages;
    char m_inputBuffer[4096] = "";
    bool m_isWaiting = false;
    std::string m_streamingResponse;
};

} // namespace smith::ui
```

#### `include/agent/conversation.h`
```cpp
struct Message {
    enum class Role { System, User, Assistant };
    Role role;
    std::string content;
    std::chrono::system_clock::time_point timestamp;
    int tokenCount = 0;
};

class Conversation {
public:
    void AddMessage(Message::Role role, const std::string& content);
    std::vector<Message> GetHistory() const;
    void Clear();
    int GetTotalTokens() const;

private:
    std::vector<Message> m_messages;
};
```

**Acceptance Criteria:**
- [ ] Chat UI renders message history
- [ ] Input area accepts text
- [ ] Messages display with role differentiation
- [ ] Streaming responses update in real-time
- [ ] Typing indicator shows during API calls

---

### Task 4.6: Update AgentWindow for API Agents

**Complexity:** Medium
**Estimated Time:** 2 hours
**Depends On:** Task 4.5

**Changes to Make:**

Modify `include/ui/agent_window.h` and `src/ui/agent_window.cpp`:

1. Detect if agent requires terminal or is API-based
2. Render ChatWindow instead of terminal for API agents
3. Handle input differently for each type

```cpp
void AgentWindow::Render(...) {
    if (m_agent && m_agent->RequiresTerminal()) {
        RenderTerminal();
    } else {
        RenderChat();
    }
}
```

**Acceptance Criteria:**
- [ ] Terminal agents use WebView/ImGui terminal
- [ ] API agents use ChatWindow
- [ ] Seamless switching based on agent type

---

### Phase 4 Validation Checkpoint ✅ COMPLETED

All validation criteria met:
- [x] HTTP client makes successful requests
- [x] ApiClient handles sync/streaming completions
- [x] RateLimiter prevents overuse
- [x] Conversation tracks message history
- [x] All 42 tests passing (100%)

**Ready for Phase 5:** Provider implementations and UI integration

---

</details>

---

## Phase 6: Polish & Testing (Formerly Phase 5)

### Objective
Achieve 70%+ test coverage, optimize performance, complete documentation, and set up CI/CD.

### Duration Estimate
2-3 days

### Risk Level
**LOW** - Additive work, no architectural changes

---

### Task 5.1: Complete Unit Test Coverage

**Complexity:** Medium
**Estimated Time:** 4 hours

**Test Files to Create:**

```
tests/unit/
  core/
    test_result.cpp
    test_app_base.cpp
  logging/
    test_logger.cpp
    test_log_sinks.cpp
  config/
    test_config_manager.cpp
  agent/
    test_agent.cpp
    test_agent_tracker.cpp
    test_claude_provider.cpp
    test_grok_provider.cpp
  terminal/
    test_terminal_theme.cpp
  network/
    test_http_client.cpp
    test_rate_limiter.cpp
  utils/
    test_string_utils.cpp
```

**Coverage Target:** 70%

**Acceptance Criteria:**
- [ ] All unit test files created
- [ ] Tests pass
- [ ] Coverage meets target

---

### Task 5.2: Create Integration Tests

**Complexity:** Medium
**Estimated Time:** 3 hours

**Test Files to Create:**

```
tests/integration/
  test_agent_lifecycle.cpp    # Create, start, stop, remove agents
  test_terminal_io.cpp        # Terminal input/output
  test_api_providers.cpp      # API roundtrip (mock server)
```

**Acceptance Criteria:**
- [ ] Integration tests pass
- [ ] Tests are deterministic

---

### Task 5.3: Performance Optimization

**Complexity:** Medium
**Estimated Time:** 2 hours

**Areas to Profile:**
1. Terminal output rendering
2. Log write performance
3. Config file I/O
4. WebView2 initialization time

**Acceptance Criteria:**
- [ ] Terminal doesn't lag with high output
- [ ] Application starts in <3 seconds
- [ ] No memory leaks detected

---

### Task 5.4: Set Up CI/CD

**Complexity:** Low
**Estimated Time:** 2 hours

**Files to Create:**

#### `.github/workflows/build.yml`
As specified in architecture document section 17.1.

**Acceptance Criteria:**
- [ ] GitHub Actions workflow created
- [ ] Build succeeds on push
- [ ] Tests run on PR

---

### Phase 6 Validation Checkpoint

- [ ] All unit tests pass (target: 70+ tests)
- [ ] All integration tests pass
- [ ] Coverage >= 70%
- [ ] No critical performance issues
- [ ] CI/CD pipeline works
- [ ] ImGui terminal fallback implemented (if needed)

---

## Phase 7: Cleanup & Release (Formerly Phase 6)

### Objective
Remove legacy code, create release build, test installation, finalize v2.0 release.

### Note
This phase was previously labeled "Phase 6" but has been renumbered to "Phase 7" after the Phase 4/5 split.

### Duration Estimate
1-2 days

### Risk Level
**LOW** - Removal of unused code

---

### Task 6.1: Remove Legacy Code

**Complexity:** Low
**Estimated Time:** 2 hours

**Files to Remove (after migration complete):**
- Old `include/types.h` (merged into new locations)
- Old `include/app.h` (replaced by core/application.h)
- Old `include/agents_tracker.h` (replaced by agent/agent_tracker.h)

**Acceptance Criteria:**
- [ ] No dead code remains
- [ ] Application compiles with only new code
- [ ] All functionality preserved

---

### Task 6.2: Create Release Build

**Complexity:** Low
**Estimated Time:** 1 hour

**Steps:**
1. Build in Release mode
2. Copy resources to bin/
3. Test executable runs standalone
4. Create installer (optional)

**Acceptance Criteria:**
- [ ] Release build works
- [ ] Resources bundled correctly
- [ ] Application runs without dev environment

---

### Task 6.3: Final Validation

**Complexity:** Low
**Estimated Time:** 2 hours

**Checklist:**
- [ ] Fresh clone builds successfully
- [ ] All tests pass
- [ ] Application starts and runs
- [ ] Terminal agents work (Claude Code)
- [ ] API agents work (Grok, ChatGPT)
- [ ] Configuration saves/loads
- [ ] No console errors

---

## Risk Mitigation Strategies

### R1: WebView2 Unavailable

**Risk:** WebView2 runtime not installed on user system.

**Detection:** TerminalFactory::IsWebView2Available() returns false.

**Mitigation:**
1. ImGui fallback always available
2. First-run check with user prompt to install
3. Link to WebView2 runtime installer

### R2: API Rate Limits

**Risk:** User hits API rate limits.

**Detection:** HTTP 429 response or rate limit headers.

**Mitigation:**
1. RateLimiter class prevents overuse
2. Exponential backoff on 429
3. Clear user feedback on limits

### R3: Build System Breaks

**Risk:** CMake changes break existing build.

**Detection:** CI fails on build.

**Mitigation:**
1. Incremental changes with validation
2. Keep old CMakeLists.txt backup
3. Rollback procedure for each task

### R4: Performance Regression

**Risk:** New terminal backend slower than old.

**Detection:** Profiling, user reports.

**Mitigation:**
1. Benchmark before/after
2. ImGui fallback available
3. Performance as Phase 5 focus

---

## Resource Requirements Summary

### External Downloads

| Resource | URL | Size | Phase |
|----------|-----|------|-------|
| xterm.js | jsdelivr CDN | ~150KB | 3 |
| xterm.css | jsdelivr CDN | ~10KB | 3 |
| xterm-addon-fit.js | jsdelivr CDN | ~5KB | 3 |
| xterm-addon-web-links.js | jsdelivr CDN | ~5KB | 3 |
| JetBrains Mono font | jetbrains.com | ~200KB | 3 |

### Build System Dependencies (FetchContent)

| Dependency | Version | Phase |
|------------|---------|-------|
| spdlog | 1.12.0 | 1 |
| GoogleTest | 1.14.0 | 1 |
| cpp-httplib | 0.14.3 | 4 |
| WebView2 SDK | 1.0.2210+ | 3 |
| WIL | 1.0.231216.1 | 3 |

### Development Environment

- Windows 10 1809+ (for ConPTY)
- Visual Studio 2019+ (C++17)
- CMake 3.16+
- Git
- Internet access (FetchContent)

---

## Glossary

| Term | Definition |
|------|------------|
| AppBase | Reusable ImGui application framework base class |
| ConPTY | Windows Console Pseudo Terminal API |
| IAgentProvider | Abstract interface for agent implementations |
| ITerminal | Abstract interface for terminal backends |
| SMITH_LOG | Unified logging macro |
| WebView2 | Microsoft Edge-based browser control |
| xterm.js | JavaScript terminal emulator library |

---

## Appendix A: File Dependencies Graph

```
                          cmake/Dependencies.cmake
                                    |
                          cmake/CompilerFlags.cmake
                                    |
                                    v
              +-------------------------------------------+
              |            CMakeLists.txt (root)          |
              +-------------------------------------------+
                     |                    |
                     v                    v
           include/logging/*        include/core/*
                     |                    |
                     +-------+------------+
                             |
                             v
                   include/agent/*
                             |
              +--------------+--------------+
              |                             |
              v                             v
    include/terminal/*            include/network/*
              |                             |
              +-------------+---------------+
                            |
                            v
                    include/ui/*
```

---

## Appendix B: Existing File Migration Map

| Existing File | Target Location | Action |
|---------------|-----------------|--------|
| include/app.h | include/core/application.h | Replace |
| include/types.h | include/core/types.h + include/agent/agent.h | Split |
| include/config.h | include/config/config_manager.h | Move + Enhance |
| include/agents_tracker.h | include/agent/agent_tracker.h | Move + Refactor |
| include/agent_window.h | include/ui/agent_window.h | Move |
| include/grid_layout.h | include/ui/grid_layout.h | Move |
| include/conpty_terminal.h | include/terminal/conpty_terminal.h | Move |
| include/terminal_buffer.h | include/terminal/terminal_buffer.h | Move |
| include/output_log.h | include/ui/output_log.h | Move |
| include/git_utils.h | include/utils/git_utils.h | Move |
| include/input_manager.h | include/core/input_manager.h | Move |

---

*Document End*

**Next Steps:**
1. Review this roadmap with the development team
2. Prioritize based on available resources
3. Begin Phase 1 execution
4. Update this document as implementation progresses

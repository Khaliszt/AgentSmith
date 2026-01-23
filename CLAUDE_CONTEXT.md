# AgentSmith - Claude Context Document

> This document provides context for AI agents (Claude Code, Aider, etc.) to continue development on this project.

## Project Overview

**AgentSmith** is a mission control interface for managing multiple AI coding agents. It displays a grid of embedded terminal windows, each running an AI agent (Claude Code, Aider, Cursor, etc.) with real-time status overlays showing git branch info, agent status, and working directory.

**Tech Stack:**
- C++17
- Dear ImGui (immediate mode GUI)
- GLFW + OpenGL 3.3 (windowing/rendering)
- nlohmann/json (configuration)
- Platform APIs for terminal embedding (Win32/X11/Cocoa)

**Target Platforms:** Windows, Linux, macOS

---

## Architecture

```
┌─────────────────────────────────────────────────────────────────────┐
│                            App                                       │
│  - GLFW window creation, OpenGL context                             │
│  - ImGui main loop                                                  │
│  - Menu bar, dialogs                                                │
│  - Owns: ConfigManager, AgentManager, GridLayout                    │
└───────────────┬─────────────────┬─────────────────┬─────────────────┘
                │                 │                 │
                ▼                 ▼                 ▼
┌───────────────────┐ ┌───────────────────┐ ┌───────────────────┐
│   ConfigManager   │ │   AgentManager    │ │    GridLayout     │
│                   │ │                   │ │                   │
│ - Load/save JSON  │ │ - CRUD agents     │ │ - NxM panel grid  │
│ - Window prefs    │ │ - Start/stop all  │ │ - Focus/keyboard  │
│ - Grid settings   │ │ - Batch git update│ │ - Panel sizing    │
└───────────────────┘ └───────────────────┘ └─────────┬─────────┘
                                                      │
                                                      ▼
                                          ┌───────────────────┐
                                          │    AgentPanel     │
                                          │                   │
                                          │ - Renders 1 agent │
                                          │ - Top/bottom      │
                                          │   overlays        │
                                          │ - Context menu    │
                                          │ - Terminal launch │
                                          └─────────┬─────────┘
                                                    │
                              ┌─────────────────────┴─────────────────────┐
                              ▼                                           ▼
                  ┌───────────────────┐                       ┌───────────────────┐
                  │   TerminalEmbed   │                       │     GitUtils      │
                  │                   │                       │                   │
                  │ - Platform-       │                       │ - Run git cmds    │
                  │   specific        │                       │ - Parse output    │
                  │ - Launch/embed    │                       │ - Branch, ahead/  │
                  │   terminals       │                       │   behind, dirty   │
                  └───────────────────┘                       └───────────────────┘
```

---

## File Structure

```
AgentSmith/
├── bin/                      # Output directory (created by build)
│   ├── agent_smith.exe       # Main executable
│   └── config.json           # Runtime configuration
│
├── build/                    # CMake build directory (gitignore this)
│
├── include/
│   ├── types.h               # Core structs: Agent, GitInfo, AppConfig, enums
│   ├── app.h                 # Main application class
│   ├── config.h              # ConfigManager - JSON load/save
│   ├── agent_manager.h       # AgentManager - agent lifecycle
│   ├── grid_layout.h         # GridLayout - NxM grid of panels
│   ├── agent_panel.h         # AgentPanel - single panel UI
│   ├── terminal_embed.h      # TerminalEmbed - platform terminal spawning
│   └── git_utils.h           # GitUtils - git command helpers
│
├── src/
│   ├── main.cpp              # Entry point
│   ├── app.cpp               # Window setup, main loop, menus, dialogs
│   ├── config.cpp            # JSON serialization
│   ├── agent_manager.cpp     # Agent CRUD, batch operations
│   ├── grid_layout.cpp       # Grid math, keyboard navigation
│   ├── agent_panel.cpp       # Panel rendering, overlays
│   ├── terminal_embed.cpp    # Platform-specific terminal code
│   └── git_utils.cpp         # Shell out to git
│
├── CMakeLists.txt            # Build configuration (fetches dependencies)
├── config.json.template      # Default configuration template
├── README.md                 # User documentation
└── CLAUDE_CONTEXT.md         # This file
```

---

## Key Classes & Responsibilities

| Class | File | Responsibility |
|-------|------|----------------|
| `App` | app.h/cpp | Top-level orchestrator. Window, ImGui, main loop, menus, dialogs. |
| `ConfigManager` | config.h/cpp | Load/save `config.json`. Holds AppConfig struct. |
| `AgentManager` | agent_manager.h/cpp | Owns `vector<Agent>`. Create, delete, start, stop agents. |
| `GridLayout` | grid_layout.h/cpp | Manages NxM grid of AgentPanels. Focus, keyboard nav, sizing. |
| `AgentPanel` | agent_panel.h/cpp | Renders one agent panel with overlays. Owns TerminalEmbed. |
| `TerminalEmbed` | terminal_embed.h/cpp | Platform abstraction for launching/embedding terminals. |
| `GitUtils` | git_utils.h/cpp | Static helpers to run git commands and parse output. |

---

## Key Data Structures (types.h)

```cpp
enum class AgentStatus { Idle, Running, Waiting, Error, Stopped };
enum class AgentType { ClaudeCode, Aider, Cursor, Custom };

struct GitInfo {
    std::string branch;
    std::string repo_name;
    std::string last_commit_hash;
    int uncommitted_changes;
    int ahead, behind;
    bool is_git_repo;
};

struct Agent {
    std::string id, name;
    AgentType type;
    std::string working_directory;
    std::string command;
    std::vector<std::string> args;
    AgentStatus status;
    int pid;
    GitInfo git_info;
    void* native_terminal_handle;
    int grid_row, grid_col;
};

struct GridConfig {
    int rows, cols;           // Grid dimensions (1-4 each)
    float panel_padding;
    float info_overlay_height;
    bool show_git_info;
    bool show_status_indicator;
    bool show_file_activity;  // Future feature
};

struct AppConfig {
    int window_width, window_height;
    bool fullscreen, dark_mode;
    GridConfig grid;
    std::string default_terminal_command;
    std::string default_shell;
    std::vector<Agent> agents;
};
```

---

## Build Instructions

```bash
cd AgentSmith
mkdir build
cd build
cmake ..
cmake --build .
```

Executable outputs to `AgentSmith/bin/agent_smith.exe`

**Dependencies** (auto-fetched by CMake):
- GLFW 3.3.9
- Dear ImGui 1.90.1
- nlohmann/json 3.11.3

---

## Current Status

### Working ✅
- CMake build system with auto-fetching dependencies
- Basic window creation with ImGui
- Grid layout rendering (1x1 to 3x3)
- Agent panel UI with top/bottom overlays
- Git info display (branch, ahead/behind, dirty count)
- Status indicators (color-coded dots)
- Menu bar with layout presets
- Add Agent dialog
- Settings dialog
- Keyboard navigation (Ctrl+arrows, Ctrl+Tab, Ctrl+1-9)
- Configuration save/load (JSON)
- Platform terminal launching (basic)

### Partially Working ⚠️
- Terminal embedding: Launches external terminal but doesn't truly embed inside the ImGui window (platform limitation - needs ConPTY on Windows, X11 reparenting on Linux)

### Not Started ❌
- See TODO list below

---

## TODO List (Priority Order)

### High Priority
1. **True terminal embedding** - Use ConPTY on Windows to render terminal output directly in ImGui instead of spawning external windows
2. **Process monitoring** - Detect when agent process exits, update status accordingly
3. **Working file visualizer** - Show which files each agent is currently reading/writing

### Medium Priority
4. **Token usage tracking** - Parse agent output to extract token counts, display in overlay
5. **Diff viewer** - Preview uncommitted changes in a modal
6. **Agent output capture** - Capture stdout/stderr for logging and analysis
7. **Session recording** - Record agent sessions for playback

### Low Priority
8. **Agent coordination** - Send messages/commands between agents
9. **Custom themes** - Theme editor, more color schemes
10. **Plugin system** - Extensible visualizers
11. **Remote agents** - Connect to agents running on other machines
12. **File tree visualizer** - Show project structure with modified file highlighting

### Code Quality
- [ ] Suppress APIENTRY macro redefinition warning
- [ ] Add proper error handling throughout
- [ ] Add logging system
- [ ] Unit tests

---

## Known Issues

1. **APIENTRY warning** - Harmless macro redefinition warning during build (GLFW vs Windows headers)
2. **Terminal not embedded** - Currently spawns separate terminal window rather than embedding inside ImGui panel
3. **No process cleanup** - If app crashes, spawned terminals may be orphaned

---

## Extension Points

### Adding a new agent type
1. Add enum value to `AgentType` in `types.h`
2. Update `GetAgentTypeText()` in `types.h`
3. Update `AgentManager::AssignDefaultCommand()` in `agent_manager.cpp`
4. Update the agent type combo in `App::RenderAddAgentDialog()` in `app.cpp`

### Adding a new overlay element
1. Modify `AgentPanel::RenderTopOverlay()` or `RenderBottomOverlay()` in `agent_panel.cpp`
2. Add any new data fields to `Agent` or `GitInfo` structs in `types.h`
3. Update data fetching in `GitUtils` or create new utility class

### Adding a new visualizer panel
1. Create new class similar to `AgentPanel`
2. Add to `GridLayout` or create separate dockable window
3. Register in `App::Run()` render loop

---

## Coding Conventions

- **Namespaces:** All code in `AgentSmith` namespace
- **Naming:** PascalCase for classes/structs, snake_case for functions/variables, m_ prefix for member variables
- **Headers:** Use `#pragma once`
- **Platform code:** Use `#ifdef PLATFORM_WINDOWS` / `PLATFORM_LINUX` / `PLATFORM_MACOS`
- **ImGui:** Use immediate mode patterns, avoid storing UI state where possible

---

## Useful Commands

```bash
# Full rebuild
rmdir /s /q build && mkdir build && cd build && cmake .. && cmake --build .

# Quick rebuild (after code changes)
cd build && cmake --build .

# Release build
cmake .. -DCMAKE_BUILD_TYPE=Release && cmake --build . --config Release

# Run
..\bin\agent_smith.exe
```

---

## Contact / Origin

This project was created collaboratively between a human developer and Claude (Anthropic). The conversation history may not be available, but this document should provide sufficient context to continue development.

**Key Design Decisions:**
- ImGui chosen for fast, lightweight UI suitable for developer tools
- Grid layout chosen over tabs for simultaneous visibility of multiple agents
- External terminal spawning as MVP, with true embedding as future goal
- Git info chosen as primary status indicator since agents typically work in git repos

---

*Last updated: January 2025*

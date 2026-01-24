# AgentSmith Terminal Embedding Implementation Plan

## Overview

Transform AgentSmith from spawning external terminal windows to **true terminal embedding** using Windows ConPTY. Each agent gets an embedded terminal rendered directly in ImGui with full input/output support.

## Architecture

```
App
 ├── ConfigManager (loads config.json)
 ├── AgentsTracker (singleton - manages all agents)
 ├── InputManager (routes keyboard to focused window)
 └── GridLayout
      └── AgentWindow[] (evolved from AgentPanel)
           └── ConPTYTerminal
                └── TerminalBuffer (ANSI parser + cell grid)
```

## New Classes

### 1. ConPTYTerminal (`include/conpty_terminal.h`, `src/conpty_terminal.cpp`)
**Responsibility**: Windows Pseudo Console management

- Creates PTY via `CreatePseudoConsole()`
- Launches child process (claude.exe) attached to PTY
- Background thread reads output pipe
- Thread-safe write to input pipe
- Handles terminal resize

### 2. TerminalBuffer (`include/terminal_buffer.h`, `src/terminal_buffer.cpp`)
**Responsibility**: Text buffer + ANSI escape sequence parsing

- Stores grid of TerminalCell (character + colors + attributes)
- State machine parser for ANSI/VT sequences
- Cursor tracking, scrolling, scrollback history
- Renders to ImGui via draw list

### 3. AgentWindow (`include/agent_window.h`, `src/agent_window.cpp`)
**Responsibility**: Single agent's terminal view (evolves from AgentPanel)

- Owns ConPTYTerminal instance
- Session tracking: start time, tokens remaining, project name
- Renders terminal content + overlays
- Right-click context menu with:
  - "Agent Info" popup (type, command, duration, directory)
  - "Auto-Accept Edits" toggle (default OFF)
  - "Fullscreen" toggle
  - "Restart Terminal" / "Stop Terminal"
- Displays agent type badge (Claude/Aider/Cursor/Custom) in top overlay

### 4. AgentsTracker (`include/agents_tracker.h`, `src/agents_tracker.cpp`)
**Responsibility**: Singleton managing all agents

- Loads agents from config on startup
- Create/destroy agent instances
- Track active/running agents
- Observer pattern for state changes

### 5. InputManager (`include/input_manager.h`, `src/input_manager.cpp`)
**Responsibility**: Keyboard input routing

- Routes keys to focused AgentWindow
- Global shortcuts: Ctrl+Tab (cycle), Ctrl+1-9 (direct select)
- Translates keys to terminal sequences (arrows, function keys, etc.)

## Data Structures

### TerminalCell
```cpp
struct TerminalCell {
    char32_t character = U' ';
    uint32_t fg_color = 0xFFFFFFFF;
    uint32_t bg_color = 0x00000000;
    uint8_t attributes = 0;  // Bold, italic, underline flags
};
```

### Agent Extensions (types.h)
```cpp
// Add to Agent struct:
std::chrono::system_clock::time_point session_start;
int tokens_remaining = -1;
std::string project_name;
bool needs_attention = false;

// Per-agent settings (configurable via right-click)
bool auto_accept_edits = false;  // Default OFF for new agents
```

### Agent Type Support
The existing `AgentType` enum already supports multiple AI agents:
```cpp
enum class AgentType {
    ClaudeCode,  // claude CLI
    Aider,       // aider CLI
    Cursor,      // cursor --folder
    Custom       // user-defined command
};
```

Each type has its own default command (set in `AssignDefaultCommand()`).
The "Agent Info" popup will display:
- **Agent Type**: Claude Code / Aider / Cursor / Custom
- **Command**: The actual CLI command being run
- **Session Duration**: Time since terminal launched
- **Working Directory**: Project path
- **Auto-Accept Edits**: ON/OFF toggle (right-click to change)

## Data Flow

**Output (PTY -> Screen):**
```
claude.exe stdout -> ConPTY -> ReadFile(pipe) -> TerminalBuffer::ProcessInput()
-> ANSI parser -> cell grid updated -> AgentWindow::Render() -> ImGui draw list
```

**Input (Keyboard -> PTY):**
```
GLFW key event -> InputManager -> AgentWindow::HandleKeyboardInput()
-> key-to-sequence conversion -> ConPTYTerminal::Write() -> WriteFile(pipe) -> claude.exe stdin
```

## Implementation Phases

### Phase 1: ConPTY Infrastructure
- Create `conpty_terminal.h/.cpp`
- Implement PTY creation, pipe management
- Launch child process with PTY
- Background read thread
- Test with cmd.exe

### Phase 2: Terminal Buffer + ANSI Parser
- Create `terminal_buffer.h/.cpp`
- TerminalCell/TerminalLine structures
- ANSI state machine (CSI sequences, SGR colors)
- Cursor movement, scrolling, clear commands
- Test with colorized output

### Phase 3: AgentWindow Evolution
- Rename/evolve AgentPanel -> AgentWindow
- Integrate ConPTYTerminal
- Render terminal buffer in ImGui
- Session tracking (duration, tokens, project)
- "Agent Info" popup on right-click
- Fullscreen toggle

### Phase 4: InputManager
- Create `input_manager.h/.cpp`
- Key-to-terminal-sequence translation
- Ctrl+Tab agent cycling
- Ctrl+1-9 direct selection
- Focus management

### Phase 5: AgentsTracker Singleton
- Create `agents_tracker.h/.cpp`
- Replace AgentManager usage in App
- Observer pattern for state changes

### Phase 6: Integration
- Update CMakeLists.txt with new files
- Wire components together in App
- Add monospace font loading
- Polish alerts/attention indicators

## Files to Create
- `include/conpty_terminal.h`
- `src/conpty_terminal.cpp`
- `include/terminal_buffer.h`
- `src/terminal_buffer.cpp`
- `include/agent_window.h`
- `src/agent_window.cpp`
- `include/agents_tracker.h`
- `src/agents_tracker.cpp`
- `include/input_manager.h`
- `src/input_manager.cpp`

## Files to Modify
- `include/types.h` - Add session tracking fields to Agent
- `CMakeLists.txt` - Add new source files
- `include/app.h` / `src/app.cpp` - Use AgentsTracker, InputManager
- `src/grid_layout.cpp` - Use AgentWindow instead of AgentPanel

## Files to Remove/Replace
- `include/agent_panel.h` - Evolved into agent_window.h
- `src/agent_panel.cpp` - Evolved into agent_window.cpp
- `include/agent_manager.h` - Replaced by agents_tracker.h
- `src/agent_manager.cpp` - Replaced by agents_tracker.cpp

## Key Patterns

**ConPTY Creation:**
```cpp
CreatePipe(&hPipeInRead, &hPipeInWrite, ...);
CreatePipe(&hPipeOutRead, &hPipeOutWrite, ...);
CreatePseudoConsole(size, hPipeInRead, hPipeOutWrite, 0, &hPC);
// Keep hPipeInWrite (we write) and hPipeOutRead (we read)
```

**ANSI Parser State Machine:**
```cpp
enum ParserState { Normal, Escape, CSI, OSC };
// Normal: regular chars, \r, \n, ESC triggers Escape state
// Escape: '[' -> CSI, ']' -> OSC
// CSI: accumulate until 0x40-0x7E final byte, then execute
```

**Terminal Rendering:**
```cpp
for each visible line:
    for each cell in line:
        draw background rect if non-transparent
        draw character with fg_color
    draw cursor block at cursor position
```

## Right-Click Context Menu

The context menu for each AgentWindow provides:
```
┌─────────────────────────┐
│ Agent: MyProject-Claude │
├─────────────────────────┤
│ ○ Agent Info...         │
│ ─────────────────────── │
│ □ Auto-Accept Edits     │  <- Checkbox, default OFF
│ ─────────────────────── │
│   Fullscreen            │
│   Restart Terminal      │
│   Stop Terminal         │
├─────────────────────────┤
│   Refresh Git Info      │
│   Open Directory...     │
├─────────────────────────┤
│   Remove Agent          │
└─────────────────────────┘
```

**Auto-Accept Edits**: When ON, the agent CLI (if it supports it) will automatically accept file edits without user confirmation. This is OFF by default for safety - user must explicitly enable it per agent.

## Agent Info Popup

Shows detailed information about the agent:
```
┌─────────────────────────────────────┐
│          AGENT INFORMATION          │
├─────────────────────────────────────┤
│ Name:        MyProject-Claude       │
│ Type:        Claude Code            │
│ Command:     claude                 │
│ Directory:   C:\Projects\MyProject  │
│ Branch:      Claude_FeatureX        │
├─────────────────────────────────────┤
│ Session Started: 2:34 PM            │
│ Session Duration: 1h 23m            │
│ Tokens Remaining: ~45,000           │
├─────────────────────────────────────┤
│ Auto-Accept: OFF                    │
│ Status: Running                     │
└─────────────────────────────────────┘
```

## Verification

1. **Build**: `cmake --build build` succeeds with no errors
2. **Launch**: App opens, shows grid of empty panels
3. **Add Agent**: Create agent (select type: Claude/Aider/Cursor/Custom)
4. **Launch Terminal**: Click "Launch Terminal" - embedded terminal appears
5. **Type**: Keyboard input appears in terminal
6. **Run Agent**: Agent CLI starts in embedded terminal
7. **Interact**: Full bidirectional communication works
8. **Switch Agents**: Ctrl+Tab cycles between agents
9. **Fullscreen**: Double-click or hotkey fullscreens single agent
10. **Agent Info**: Right-click -> "Agent Info" shows type, duration, config
11. **Auto-Accept**: Right-click -> toggle "Auto-Accept Edits" (verify default is OFF)
12. **Multi-Type**: Create agents of different types, verify correct commands launch

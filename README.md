# AgentSmith

> *"Never send a human to do a machine's job."*

A mission control interface for managing multiple AI coding agents with embedded terminals, real-time git status, and unified monitoring.

![AgentSmith Screenshot](docs/screenshot.png)

## Features

### Core
- **Grid-based layout** - View 1-9 agents simultaneously in configurable grid (1x1 to 3x3)
- **Embedded terminals** - Each agent runs in its own terminal panel
- **Real-time git info** - Branch, ahead/behind status, uncommitted changes
- **Status indicators** - Visual status for each agent (Running, Waiting, Error, Idle)
- **Keyboard navigation** - Quick switching between agent panels

### Agent Support
- **Claude Code** - Anthropic's CLI coding agent
- **Aider** - Open source AI pair programming
- **Cursor** - AI-first code editor
- **Custom** - Any command-line tool

### Info Overlays
Each panel displays:
- Agent name and type badge
- Status indicator (color-coded dot)
- Git branch with ahead/behind counts
- Uncommitted changes count
- Working directory path

## Future Roadmap

- [ ] **Working file visualizer** - See which files each agent is currently editing
- [ ] **Token usage tracking** - Monitor API costs per agent
- [ ] **Diff viewer** - Preview changes before committing
- [ ] **Agent coordination** - Send messages between agents
- [ ] **Session recording** - Playback agent sessions
- [ ] **Custom themes** - More color schemes
- [ ] **Plugin system** - Extend with custom visualizers

## Building

### Prerequisites

**Linux (Ubuntu/Debian):**
```bash
sudo apt update
sudo apt install -y \
    build-essential \
    cmake \
    libglfw3-dev \
    libgl1-mesa-dev \
    libx11-dev
```

**macOS:**
```bash
brew install cmake glfw
```

**Windows:**
- Visual Studio 2019+ with C++ workload
- vcpkg: `vcpkg install glfw3`

### Compile

```bash
cd AgentSmith
mkdir build && cd build
cmake ..
cmake --build . --config Release
./agent_smith
```

## Usage

### Adding Agents

1. **File → Add Agent** (or `Ctrl+N`)
2. Enter agent name
3. Browse to working directory
4. Select agent type
5. Click Create

### Grid Layouts

Use **View → Grid Layout** to switch between:
- 1x1 (single agent focus)
- 1x2, 2x1 (side by side)
- 2x2 (quad view)
- 2x3, 3x3 (many agents)

### Keyboard Shortcuts

| Shortcut | Action |
|----------|--------|
| `Ctrl+N` | Add new agent |
| `Ctrl+,` | Settings |
| `Ctrl+Arrow` | Navigate between panels |
| `Ctrl+Tab` | Next panel |
| `Ctrl+Shift+Tab` | Previous panel |
| `Ctrl+1-9` | Jump to panel N |

### Context Menu

Right-click on any panel for:
- Launch/Stop/Restart terminal
- Refresh git info
- Open directory in file manager
- Remove agent

## Configuration

Configuration is stored in `config.json`:

```json
{
  "window": {
    "width": 1920,
    "height": 1080,
    "fullscreen": false
  },
  "grid": {
    "rows": 2,
    "cols": 2,
    "show_git_info": true,
    "show_status_indicator": true
  },
  "agents": [
    {
      "name": "Frontend",
      "working_directory": "/path/to/frontend",
      "type": 0,
      "command": "claude"
    }
  ]
}
```

## Architecture

```
AgentSmith/
├── include/
│   ├── types.h           # Core data structures
│   ├── app.h             # Main application
│   ├── grid_layout.h     # Grid management
│   ├── agent_panel.h     # Individual panel rendering
│   ├── agent_manager.h   # Agent lifecycle
│   ├── terminal_embed.h  # Platform terminal embedding
│   ├── git_utils.h       # Git command interface
│   └── config.h          # Configuration management
├── src/
│   ├── main.cpp
│   ├── app.cpp
│   ├── grid_layout.cpp
│   ├── agent_panel.cpp
│   ├── agent_manager.cpp
│   ├── terminal_embed.cpp
│   ├── git_utils.cpp
│   └── config.cpp
└── CMakeLists.txt
```

## Dependencies

- [Dear ImGui](https://github.com/ocornut/imgui) - Immediate mode GUI
- [GLFW](https://www.glfw.org/) - Window/input handling
- [nlohmann/json](https://github.com/nlohmann/json) - JSON parsing
- Platform APIs for terminal embedding (Win32/X11/Cocoa)

## License

MIT License

## Contributing

Contributions welcome! Areas of interest:
- Better terminal embedding (ConPTY on Windows, proper X11 reparenting)
- File activity monitoring
- Additional agent type integrations
- UI/UX improvements

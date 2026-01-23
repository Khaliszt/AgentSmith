#pragma once

#include "types.h"
#include <functional>

namespace AgentSmith {

//=============================================================================
// Terminal Embedding Interface
// 
// This provides a cross-platform abstraction for embedding terminal windows
// inside our ImGui application. On Windows, we use Windows Terminal or ConPTY.
// On Linux, we embed using X11 reparenting. On macOS, we use NSView embedding.
//=============================================================================

class TerminalEmbed {
public:
    TerminalEmbed();
    ~TerminalEmbed();
    
    // Initialize the terminal embedding system
    static bool Initialize();
    static void Shutdown();
    
    // Launch a terminal for an agent and embed it
    // Returns true if successful, sets agent's native_terminal_handle
    bool LaunchAndEmbed(Agent& agent, void* parent_window_handle);
    
    // Detach terminal from embedding (but keep it running)
    void Detach(Agent& agent);
    
    // Terminate the terminal process
    void Terminate(Agent& agent);
    
    // Resize the embedded terminal to fit a new region
    void Resize(Agent& agent, int x, int y, int width, int height);
    
    // Focus the terminal (bring to front within our window)
    void Focus(Agent& agent);
    
    // Check if terminal process is still running
    bool IsRunning(const Agent& agent) const;
    
    // Get the native window handle for the main application window
    static void* GetNativeWindowHandle(void* glfw_window);
    
private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

//=============================================================================
// Platform-specific terminal launch commands
//=============================================================================

struct TerminalLaunchConfig {
    std::string terminal_executable;
    std::vector<std::string> base_args;
    std::string working_dir_flag;
    std::string command_flag;
    
    static TerminalLaunchConfig GetDefault();
    static TerminalLaunchConfig ForWindowsTerminal();
    static TerminalLaunchConfig ForGnomeTerminal();
    static TerminalLaunchConfig ForiTerm();
    static TerminalLaunchConfig ForXterm();
};

} // namespace AgentSmith

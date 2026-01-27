#include "terminal_embed.h"
#include <iostream>
#include <cstdlib>
#include <sstream>

// GLFW must be included before glfw3native.h
#include <GLFW/glfw3.h>

#ifdef PLATFORM_WINDOWS
#include <windows.h>
#include <dwmapi.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#elif defined(PLATFORM_LINUX)
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#define GLFW_EXPOSE_NATIVE_X11
#include <GLFW/glfw3native.h>
#elif defined(PLATFORM_MACOS)
#define GLFW_EXPOSE_NATIVE_COCOA
#include <GLFW/glfw3native.h>
#endif

namespace AgentSmith {

//=============================================================================
// Platform-specific implementation details
//=============================================================================

struct TerminalEmbed::Impl {
#ifdef PLATFORM_WINDOWS
    // Windows-specific data
#elif defined(PLATFORM_LINUX)
    Display* display = nullptr;
#elif defined(PLATFORM_MACOS)
    // macOS-specific data
#endif
};

//=============================================================================
// Static initialization
//=============================================================================

static bool s_initialized = false;

#ifdef PLATFORM_LINUX
static Display* s_display = nullptr;
#endif

bool TerminalEmbed::Initialize() {
    if (s_initialized) return true;
    
#ifdef PLATFORM_LINUX
    s_display = XOpenDisplay(nullptr);
    if (!s_display) {
        std::cerr << "Failed to open X11 display for terminal embedding" << std::endl;
        return false;
    }
#endif
    
    s_initialized = true;
    return true;
}

void TerminalEmbed::Shutdown() {
#ifdef PLATFORM_LINUX
    if (s_display) {
        XCloseDisplay(s_display);
        s_display = nullptr;
    }
#endif
    s_initialized = false;
}

//=============================================================================
// TerminalEmbed implementation
//=============================================================================

TerminalEmbed::TerminalEmbed() : m_impl(std::make_unique<Impl>()) {}
TerminalEmbed::~TerminalEmbed() = default;

void* TerminalEmbed::GetNativeWindowHandle(void* glfw_window) {
    GLFWwindow* window = static_cast<GLFWwindow*>(glfw_window);
    
#ifdef PLATFORM_WINDOWS
    return glfwGetWin32Window(window);
#elif defined(PLATFORM_LINUX)
    return (void*)glfwGetX11Window(window);
#elif defined(PLATFORM_MACOS)
    return glfwGetCocoaWindow(window);
#else
    return nullptr;
#endif
}

bool TerminalEmbed::LaunchAndEmbed(Agent& agent, void* parent_window_handle) {
    if (agent.working_directory.empty()) {
        return false;
    }
    
#ifdef PLATFORM_WINDOWS
    // Windows: Launch Windows Terminal and embed it
    std::wstring cmd = L"wt.exe";
    std::wstring args = L"-d \"" + std::wstring(agent.working_directory.begin(), agent.working_directory.end()) + L"\"";
    
    if (!agent.command.empty()) {
        args += L" cmd /k " + std::wstring(agent.command.begin(), agent.command.end());
        for (const auto& arg : agent.args) {
            args += L" " + std::wstring(arg.begin(), arg.end());
        }
    }
    
    STARTUPINFOW si = { sizeof(si) };
    PROCESS_INFORMATION pi;
    
    std::wstring full_cmd = cmd + L" " + args;
    
    if (CreateProcessW(nullptr, &full_cmd[0], nullptr, nullptr, FALSE,
                       CREATE_NEW_CONSOLE, nullptr, nullptr, &si, &pi)) {
        agent.pid = pi.dwProcessId;
        agent.native_terminal_handle = pi.hProcess;
        agent.terminal_embedded = true;
        
        CloseHandle(pi.hThread);
        
        // Wait a bit for the window to be created
        Sleep(500);
        
        // Find and embed the terminal window
        // Note: Full Windows Terminal embedding requires ConPTY or similar
        // This is a simplified version that just tracks the process
        
        return true;
    }
    
    return false;
    
#elif defined(PLATFORM_LINUX)
    // Linux: Launch terminal and attempt X11 reparenting
    pid_t pid = fork();
    
    if (pid == 0) {
        // Child process
        chdir(agent.working_directory.c_str());
        
        // Build command
        std::string term_cmd;
        
        if (!agent.command.empty()) {
            term_cmd = agent.command;
            for (const auto& arg : agent.args) {
                term_cmd += " " + arg;
            }
        }
        
        // Try different terminals
        if (term_cmd.empty()) {
            execlp("x-terminal-emulator", "x-terminal-emulator", nullptr);
            execlp("gnome-terminal", "gnome-terminal", nullptr);
            execlp("xterm", "xterm", nullptr);
        } else {
            execlp("x-terminal-emulator", "x-terminal-emulator", "-e", term_cmd.c_str(), nullptr);
            execlp("gnome-terminal", "gnome-terminal", "--", "sh", "-c", term_cmd.c_str(), nullptr);
            execlp("xterm", "xterm", "-e", term_cmd.c_str(), nullptr);
        }
        
        _exit(1);
    } else if (pid > 0) {
        agent.pid = pid;
        agent.terminal_embedded = true;
        
        // Note: Full X11 embedding would require:
        // 1. Wait for child to create window
        // 2. Find the window using _NET_WM_PID
        // 3. Reparent it into our window
        // This is a simplified version
        
        return true;
    }
    
    return false;
    
#elif defined(PLATFORM_MACOS)
    // macOS: Launch Terminal.app
    std::string script = "tell application \"Terminal\" to do script \"cd '" + 
                         agent.working_directory + "'";
    
    if (!agent.command.empty()) {
        script += " && " + agent.command;
        for (const auto& arg : agent.args) {
            script += " " + arg;
        }
    }
    
    script += "\"";
    
    std::string cmd = "osascript -e '" + script + "'";
    int result = system(cmd.c_str());
    
    if (result == 0) {
        agent.terminal_embedded = true;
        return true;
    }
    
    return false;
    
#else
    return false;
#endif
}

void TerminalEmbed::Detach(Agent& agent) {
    agent.terminal_embedded = false;
    agent.native_terminal_handle = nullptr;
}

void TerminalEmbed::Terminate(Agent& agent) {
    if (agent.pid <= 0) return;
    
#ifdef PLATFORM_WINDOWS
    if (agent.native_terminal_handle) {
        TerminateProcess((HANDLE)agent.native_terminal_handle, 0);
        CloseHandle((HANDLE)agent.native_terminal_handle);
    }
#elif defined(PLATFORM_LINUX)
    kill(agent.pid, SIGTERM);
    waitpid(agent.pid, nullptr, WNOHANG);
#elif defined(PLATFORM_MACOS)
    kill(agent.pid, SIGTERM);
#endif
    
    agent.pid = -1;
    agent.native_terminal_handle = nullptr;
    agent.terminal_embedded = false;
}

void TerminalEmbed::Resize(Agent& agent, int x, int y, int width, int height) {
    if (!agent.terminal_embedded || !agent.native_terminal_handle) return;
    
#ifdef PLATFORM_WINDOWS
    // For true embedding, we'd use SetWindowPos on the terminal window
    // This requires finding the terminal's HWND first
#elif defined(PLATFORM_LINUX)
    // For X11 embedding, we'd use XMoveResizeWindow
    // This requires the terminal's X11 Window ID
#endif
    
    // Note: Full implementation requires platform-specific window management
    // This is a placeholder for the embedding logic
}

void TerminalEmbed::Focus(Agent& agent) {
    if (!agent.terminal_embedded) return;
    
#ifdef PLATFORM_WINDOWS
    // SetForegroundWindow for the terminal
#elif defined(PLATFORM_LINUX)
    // XRaiseWindow for X11
#endif
}

bool TerminalEmbed::IsRunning(const Agent& agent) const {
    if (agent.pid <= 0) return false;
    
#ifdef PLATFORM_WINDOWS
    if (agent.native_terminal_handle) {
        DWORD exit_code;
        if (GetExitCodeProcess((HANDLE)agent.native_terminal_handle, &exit_code)) {
            return exit_code == STILL_ACTIVE;
        }
    }
    return false;
#elif defined(PLATFORM_LINUX)
    return kill(agent.pid, 0) == 0;
#elif defined(PLATFORM_MACOS)
    return kill(agent.pid, 0) == 0;
#else
    return false;
#endif
}

//=============================================================================
// Terminal Launch Configuration
//=============================================================================

TerminalLaunchConfig TerminalLaunchConfig::GetDefault() {
#ifdef PLATFORM_WINDOWS
    return ForWindowsTerminal();
#elif defined(PLATFORM_LINUX)
    return ForGnomeTerminal();
#elif defined(PLATFORM_MACOS)
    return ForiTerm();
#else
    return {};
#endif
}

TerminalLaunchConfig TerminalLaunchConfig::ForWindowsTerminal() {
    return {
        "wt.exe",
        {},
        "-d",
        "cmd /k"
    };
}

TerminalLaunchConfig TerminalLaunchConfig::ForGnomeTerminal() {
    return {
        "gnome-terminal",
        {},
        "--working-directory=",
        "--"
    };
}

TerminalLaunchConfig TerminalLaunchConfig::ForiTerm() {
    return {
        "open",
        {"-a", "iTerm"},
        "",
        ""
    };
}

TerminalLaunchConfig TerminalLaunchConfig::ForXterm() {
    return {
        "xterm",
        {},
        "-cd",
        "-e"
    };
}

} // namespace AgentSmith

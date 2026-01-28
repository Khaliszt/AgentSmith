// C:\FarfadetsCorp\AgentSmith\include\terminal\conpty_terminal.h

#pragma once

#include "terminal_interface.h"
#include "terminal_theme.h"
#include <string>
#include <vector>
#include <memory>
#include <thread>
#include <atomic>
#include <mutex>
#include <functional>

#ifdef _WIN32
#include <windows.h>
#endif

namespace smith::terminal {

/**
 * @brief Windows Pseudo Console (ConPTY) Terminal Implementation
 *
 * This terminal implementation uses Windows ConPTY API to manage process I/O.
 * It creates a PTY, launches a child process, and handles I/O via background threads.
 *
 * ConPTY manages the process lifecycle and I/O piping. It is designed to work
 * with WebViewTerminal for rendering (xterm.js displays the output), but can
 * also be used standalone or with other rendering backends.
 *
 * Requirements:
 * - Windows 10 version 1809 (October 2018 Update) or later
 * - ConPTY API is part of Windows SDK
 */
class ConPTYTerminal : public ITerminal {
public:
    ConPTYTerminal();
    ~ConPTYTerminal() override;

    // Disable copy
    ConPTYTerminal(const ConPTYTerminal&) = delete;
    ConPTYTerminal& operator=(const ConPTYTerminal&) = delete;

    // Move support
    ConPTYTerminal(ConPTYTerminal&& other) noexcept;
    ConPTYTerminal& operator=(ConPTYTerminal&& other) noexcept;

    // === ITerminal Interface - Process Lifecycle ===

    bool Launch(const std::string& command,
               const std::vector<std::string>& args,
               const std::string& workingDir,
               int cols, int rows) override;

    void Terminate() override;
    bool IsRunning() const override;
    int GetExitCode() const override;
    int GetProcessId() const override;

    // === ITerminal Interface - I/O ===

    void Write(const std::string& text) override;
    void Write(const char* data, size_t length) override;
    void SetOutputCallback(OutputCallback callback) override;
    void SetExitCallback(ExitCallback callback) override;
    void SetErrorCallback(ErrorCallback callback) override;

    // === ITerminal Interface - Display Size ===

    void Resize(int cols, int rows) override;
    int GetCols() const override { return m_cols; }
    int GetRows() const override { return m_rows; }
    void Clear() override;

    // === ITerminal Interface - Theme ===

    void SetTheme(const TerminalTheme& theme) override;
    const TerminalTheme& GetTheme() const override { return m_theme; }

    // === ITerminal Interface - Capabilities ===

    bool HasNativeRendering() const override { return false; }
    bool SupportsSelection() const override { return false; }

private:
#ifdef _WIN32
    // Windows-specific handles
    HANDLE m_hPC = INVALID_HANDLE_VALUE;         // Pseudo console handle
    HANDLE m_hPipeIn = INVALID_HANDLE_VALUE;     // Write to this pipe -> PTY input
    HANDLE m_hPipeOut = INVALID_HANDLE_VALUE;    // Read from this pipe <- PTY output
    HANDLE m_hProcess = INVALID_HANDLE_VALUE;    // Child process handle
    HANDLE m_hThread = INVALID_HANDLE_VALUE;     // Child process main thread
    DWORD m_processId = 0;
    DWORD m_exitCode = 0;
#endif

    // Reader thread
    std::unique_ptr<std::thread> m_readThread;
    std::atomic<bool> m_running{false};

    // Callbacks
    OutputCallback m_outputCallback;
    ExitCallback m_exitCallback;
    ErrorCallback m_errorCallback;
    mutable std::mutex m_callbackMutex;

    // Terminal size
    int m_cols = 120;
    int m_rows = 30;

    // Theme
    TerminalTheme m_theme;

    // Internal methods
    void ReadThreadFunc();
    void CleanupHandles();
    void InvokeOutputCallback(const char* data, size_t length);
    void InvokeExitCallback(int exitCode);
    void InvokeErrorCallback(const std::string& error);
};

} // namespace smith::terminal

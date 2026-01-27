#pragma once

#include "types.h"
#include <string>
#include <vector>
#include <memory>
#include <thread>
#include <atomic>
#include <mutex>
#include <functional>

#ifdef PLATFORM_WINDOWS
#include <windows.h>
#endif

namespace AgentSmith {

//=============================================================================
// ConPTY Terminal
//
// Windows Pseudo Console (ConPTY) management for embedded terminals.
// Creates a PTY, launches child process, handles I/O via background threads.
//=============================================================================

class ConPTYTerminal {
public:
    ConPTYTerminal();
    ~ConPTYTerminal();

    // Disable copy
    ConPTYTerminal(const ConPTYTerminal&) = delete;
    ConPTYTerminal& operator=(const ConPTYTerminal&) = delete;

    // Move support
    ConPTYTerminal(ConPTYTerminal&& other) noexcept;
    ConPTYTerminal& operator=(ConPTYTerminal&& other) noexcept;

    // Launch a process in the PTY
    // command: executable path (e.g., "claude", "cmd.exe")
    // args: command line arguments
    // working_directory: starting directory for the process
    // cols/rows: initial terminal size in characters
    bool Launch(const std::string& command,
                const std::vector<std::string>& args,
                const std::string& working_directory,
                int cols = 120,
                int rows = 30);

    // Terminate the child process
    void Terminate();

    // Check if process is still running
    bool IsRunning() const;

    // Write input to the terminal (thread-safe)
    void Write(const std::string& data);
    void Write(const char* data, size_t length);

    // Resize the terminal
    void Resize(int cols, int rows);

    // Set callback for output data (called from read thread)
    using OutputCallback = std::function<void(const char* data, size_t length)>;
    void SetOutputCallback(OutputCallback callback);

    // Get process ID
    int GetProcessId() const;

    // Get terminal size
    int GetCols() const { return m_cols; }
    int GetRows() const { return m_rows; }

private:
    // Platform-specific implementation
#ifdef PLATFORM_WINDOWS
    HANDLE m_hPC = INVALID_HANDLE_VALUE;         // Pseudo console handle
    HANDLE m_hPipeIn = INVALID_HANDLE_VALUE;     // Write to this pipe -> PTY input
    HANDLE m_hPipeOut = INVALID_HANDLE_VALUE;    // Read from this pipe <- PTY output
    HANDLE m_hProcess = INVALID_HANDLE_VALUE;    // Child process handle
    HANDLE m_hThread = INVALID_HANDLE_VALUE;     // Child process main thread
    DWORD m_processId = 0;
#endif

    // Reader thread
    std::unique_ptr<std::thread> m_readThread;
    std::atomic<bool> m_running{false};

    // Output callback
    OutputCallback m_outputCallback;
    std::mutex m_callbackMutex;

    // Terminal size
    int m_cols = 120;
    int m_rows = 30;

    // Internal methods
    void ReadThreadFunc();
    void CleanupHandles();
};

} // namespace AgentSmith

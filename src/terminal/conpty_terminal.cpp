// C:\FarfadetsCorp\AgentSmith\src\terminal\conpty_terminal.cpp

#include "terminal/conpty_terminal.h"
#include <iostream>
#include <sstream>

#ifdef _WIN32
#include <windows.h>
#include <process.h>

// ConPTY API requires Windows 10 1809+
#ifndef PROC_THREAD_ATTRIBUTE_PSEUDOCONSOLE
#define PROC_THREAD_ATTRIBUTE_PSEUDOCONSOLE 0x00020016
#endif
#endif

namespace smith::terminal {

ConPTYTerminal::ConPTYTerminal() {
    // Initialize with default theme
    m_theme = TerminalTheme::GetDefault();
}

ConPTYTerminal::~ConPTYTerminal() {
    Terminate();
}

ConPTYTerminal::ConPTYTerminal(ConPTYTerminal&& other) noexcept {
    // CRITICAL: Stop source terminal's read thread first to prevent race condition
    // The read thread may be using callbacks while we're moving them
    other.m_running = false;

#ifdef _WIN32
    // Close pipe to unblock ReadFile in the read thread
    if (other.m_hPipeOut != INVALID_HANDLE_VALUE) {
        CloseHandle(other.m_hPipeOut);
        other.m_hPipeOut = INVALID_HANDLE_VALUE;
    }
#endif

    // Now safe to join the read thread
    if (other.m_readThread && other.m_readThread->joinable()) {
        other.m_readThread->join();
    }
    other.m_readThread.reset();

#ifdef _WIN32
    // Now safe to move handles
    m_hPC = other.m_hPC;
    m_hPipeIn = other.m_hPipeIn;
    m_hPipeOut = INVALID_HANDLE_VALUE;  // Already closed above
    m_hProcess = other.m_hProcess;
    m_hThread = other.m_hThread;
    m_processId = other.m_processId;
    m_exitCode = other.m_exitCode;

    other.m_hPC = INVALID_HANDLE_VALUE;
    other.m_hPipeIn = INVALID_HANDLE_VALUE;
    other.m_hProcess = INVALID_HANDLE_VALUE;
    other.m_hThread = INVALID_HANDLE_VALUE;
    other.m_processId = 0;
    other.m_exitCode = 0;
#endif

    m_running.store(false);  // Terminal is stopped after move

    // Safe to move callbacks now that read thread is stopped
    {
        std::lock_guard<std::mutex> lock(other.m_callbackMutex);
        m_outputCallback = std::move(other.m_outputCallback);
        m_exitCallback = std::move(other.m_exitCallback);
        m_errorCallback = std::move(other.m_errorCallback);
    }
    m_cols = other.m_cols;
    m_rows = other.m_rows;
    m_theme = other.m_theme;
}

ConPTYTerminal& ConPTYTerminal::operator=(ConPTYTerminal&& other) noexcept {
    if (this != &other) {
        // First terminate our own terminal
        Terminate();

        // CRITICAL: Stop source terminal's read thread first
        other.m_running = false;

#ifdef _WIN32
        // Close pipe to unblock ReadFile
        if (other.m_hPipeOut != INVALID_HANDLE_VALUE) {
            CloseHandle(other.m_hPipeOut);
            other.m_hPipeOut = INVALID_HANDLE_VALUE;
        }
#endif

        // Join read thread
        if (other.m_readThread && other.m_readThread->joinable()) {
            other.m_readThread->join();
        }
        other.m_readThread.reset();

#ifdef _WIN32
        // Now safe to move handles
        m_hPC = other.m_hPC;
        m_hPipeIn = other.m_hPipeIn;
        m_hPipeOut = INVALID_HANDLE_VALUE;
        m_hProcess = other.m_hProcess;
        m_hThread = other.m_hThread;
        m_processId = other.m_processId;
        m_exitCode = other.m_exitCode;

        other.m_hPC = INVALID_HANDLE_VALUE;
        other.m_hPipeIn = INVALID_HANDLE_VALUE;
        other.m_hProcess = INVALID_HANDLE_VALUE;
        other.m_hThread = INVALID_HANDLE_VALUE;
        other.m_processId = 0;
        other.m_exitCode = 0;
#endif

        m_running.store(false);

        // Safe to move callbacks
        {
            std::lock_guard<std::mutex> lock(other.m_callbackMutex);
            m_outputCallback = std::move(other.m_outputCallback);
            m_exitCallback = std::move(other.m_exitCallback);
            m_errorCallback = std::move(other.m_errorCallback);
        }
        m_cols = other.m_cols;
        m_rows = other.m_rows;
        m_theme = other.m_theme;
    }
    return *this;
}

#ifdef _WIN32

bool ConPTYTerminal::Launch(const std::string& command,
                            const std::vector<std::string>& args,
                            const std::string& workingDir,
                            int cols,
                            int rows) {
    if (m_running) {
        InvokeErrorCallback("Launch called but terminal already running");
        return false;
    }

    m_cols = cols;
    m_rows = rows;

    // Create pipes for PTY I/O
    HANDLE hPipeInRead = INVALID_HANDLE_VALUE;
    HANDLE hPipeInWrite = INVALID_HANDLE_VALUE;
    HANDLE hPipeOutRead = INVALID_HANDLE_VALUE;
    HANDLE hPipeOutWrite = INVALID_HANDLE_VALUE;

    SECURITY_ATTRIBUTES sa = {};
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = FALSE;  // Pipes should NOT be inherited

    // Create input pipe (we write to hPipeInWrite, PTY reads from hPipeInRead)
    if (!CreatePipe(&hPipeInRead, &hPipeInWrite, &sa, 0)) {
        InvokeErrorCallback("Failed to create input pipe");
        return false;
    }

    // Create output pipe (PTY writes to hPipeOutWrite, we read from hPipeOutRead)
    if (!CreatePipe(&hPipeOutRead, &hPipeOutWrite, &sa, 0)) {
        CloseHandle(hPipeInRead);
        CloseHandle(hPipeInWrite);
        InvokeErrorCallback("Failed to create output pipe");
        return false;
    }

    // Create the Pseudo Console
    COORD size = { static_cast<SHORT>(cols), static_cast<SHORT>(rows) };
    HRESULT hr = CreatePseudoConsole(size, hPipeInRead, hPipeOutWrite, 0, &m_hPC);

    if (FAILED(hr)) {
        CloseHandle(hPipeInRead);
        CloseHandle(hPipeInWrite);
        CloseHandle(hPipeOutRead);
        CloseHandle(hPipeOutWrite);
        InvokeErrorCallback("Failed to create pseudo console: 0x" + std::to_string(hr));
        return false;
    }

    // Close the pipe ends that the PTY now owns
    CloseHandle(hPipeInRead);
    CloseHandle(hPipeOutWrite);

    // Keep the ends we need
    m_hPipeIn = hPipeInWrite;   // We write to this
    m_hPipeOut = hPipeOutRead;  // We read from this

    // Build command line string
    std::wstring cmdLine;

    // Convert command to wide string
    int wideLen = MultiByteToWideChar(CP_UTF8, 0, command.c_str(), -1, nullptr, 0);
    std::wstring wideCommand(wideLen, 0);
    MultiByteToWideChar(CP_UTF8, 0, command.c_str(), -1, &wideCommand[0], wideLen);
    wideCommand.resize(wideLen - 1);  // Remove null terminator

    cmdLine = wideCommand;

    // Append arguments
    for (const auto& arg : args) {
        cmdLine += L" ";
        wideLen = MultiByteToWideChar(CP_UTF8, 0, arg.c_str(), -1, nullptr, 0);
        std::wstring wideArg(wideLen, 0);
        MultiByteToWideChar(CP_UTF8, 0, arg.c_str(), -1, &wideArg[0], wideLen);
        wideArg.resize(wideLen - 1);

        // Quote if contains spaces
        if (arg.find(' ') != std::string::npos) {
            cmdLine += L"\"" + wideArg + L"\"";
        } else {
            cmdLine += wideArg;
        }
    }

    // Convert working directory
    std::wstring wideWorkDir;
    if (!workingDir.empty()) {
        wideLen = MultiByteToWideChar(CP_UTF8, 0, workingDir.c_str(), -1, nullptr, 0);
        wideWorkDir.resize(wideLen);
        MultiByteToWideChar(CP_UTF8, 0, workingDir.c_str(), -1, &wideWorkDir[0], wideLen);
        wideWorkDir.resize(wideLen - 1);
    }

    // Initialize startup info with pseudo console
    STARTUPINFOEXW siEx = {};
    siEx.StartupInfo.cb = sizeof(STARTUPINFOEXW);

    // Allocate attribute list for pseudo console
    SIZE_T attrListSize = 0;
    InitializeProcThreadAttributeList(nullptr, 1, 0, &attrListSize);

    siEx.lpAttributeList = reinterpret_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(
        HeapAlloc(GetProcessHeap(), 0, attrListSize));

    if (!siEx.lpAttributeList) {
        CloseHandle(m_hPipeIn);
        CloseHandle(m_hPipeOut);
        ClosePseudoConsole(m_hPC);
        m_hPipeIn = INVALID_HANDLE_VALUE;
        m_hPipeOut = INVALID_HANDLE_VALUE;
        m_hPC = INVALID_HANDLE_VALUE;
        InvokeErrorCallback("Failed to allocate attribute list");
        return false;
    }

    if (!InitializeProcThreadAttributeList(siEx.lpAttributeList, 1, 0, &attrListSize)) {
        HeapFree(GetProcessHeap(), 0, siEx.lpAttributeList);
        CloseHandle(m_hPipeIn);
        CloseHandle(m_hPipeOut);
        ClosePseudoConsole(m_hPC);
        m_hPipeIn = INVALID_HANDLE_VALUE;
        m_hPipeOut = INVALID_HANDLE_VALUE;
        m_hPC = INVALID_HANDLE_VALUE;
        InvokeErrorCallback("Failed to initialize attribute list");
        return false;
    }

    // Set the pseudo console attribute
    if (!UpdateProcThreadAttribute(siEx.lpAttributeList, 0,
                                   PROC_THREAD_ATTRIBUTE_PSEUDOCONSOLE,
                                   m_hPC, sizeof(m_hPC), nullptr, nullptr)) {
        DeleteProcThreadAttributeList(siEx.lpAttributeList);
        HeapFree(GetProcessHeap(), 0, siEx.lpAttributeList);
        CloseHandle(m_hPipeIn);
        CloseHandle(m_hPipeOut);
        ClosePseudoConsole(m_hPC);
        m_hPipeIn = INVALID_HANDLE_VALUE;
        m_hPipeOut = INVALID_HANDLE_VALUE;
        m_hPC = INVALID_HANDLE_VALUE;
        InvokeErrorCallback("Failed to update attribute");
        return false;
    }

    // Create the process
    PROCESS_INFORMATION pi = {};
    std::vector<wchar_t> cmdLineBuf(cmdLine.begin(), cmdLine.end());
    cmdLineBuf.push_back(0);

    BOOL success = CreateProcessW(
        nullptr,                          // Application name (use command line)
        cmdLineBuf.data(),                // Command line
        nullptr,                          // Process security attributes
        nullptr,                          // Thread security attributes
        FALSE,                            // Inherit handles
        EXTENDED_STARTUPINFO_PRESENT,     // Creation flags
        nullptr,                          // Environment (inherit)
        wideWorkDir.empty() ? nullptr : wideWorkDir.c_str(),  // Working directory
        &siEx.StartupInfo,                // Startup info
        &pi                               // Process info output
    );

    // Cleanup attribute list
    DeleteProcThreadAttributeList(siEx.lpAttributeList);
    HeapFree(GetProcessHeap(), 0, siEx.lpAttributeList);

    if (!success) {
        DWORD err = GetLastError();
        CloseHandle(m_hPipeIn);
        CloseHandle(m_hPipeOut);
        ClosePseudoConsole(m_hPC);
        m_hPipeIn = INVALID_HANDLE_VALUE;
        m_hPipeOut = INVALID_HANDLE_VALUE;
        m_hPC = INVALID_HANDLE_VALUE;
        InvokeErrorCallback("Failed to create process, error: " + std::to_string(err));
        return false;
    }

    m_hProcess = pi.hProcess;
    m_hThread = pi.hThread;
    m_processId = pi.dwProcessId;
    m_exitCode = 0;
    m_running = true;

    // Start the read thread
    m_readThread = std::make_unique<std::thread>(&ConPTYTerminal::ReadThreadFunc, this);

    return true;
}

void ConPTYTerminal::Terminate() {
    if (!m_running) {
        return;
    }

    m_running = false;

    // Close the pseudo console first - this will cause the child process to exit
    if (m_hPC != INVALID_HANDLE_VALUE) {
        ClosePseudoConsole(m_hPC);
        m_hPC = INVALID_HANDLE_VALUE;
    }

    // Wait for read thread to finish
    if (m_readThread && m_readThread->joinable()) {
        m_readThread->join();
    }
    m_readThread.reset();

    // Terminate process if still running
    if (m_hProcess != INVALID_HANDLE_VALUE) {
        // Give it a moment to exit gracefully
        if (WaitForSingleObject(m_hProcess, 500) == WAIT_TIMEOUT) {
            TerminateProcess(m_hProcess, 1);
        }

        // Get exit code
        GetExitCodeProcess(m_hProcess, &m_exitCode);
    }

    CleanupHandles();
}

bool ConPTYTerminal::IsRunning() const {
    if (!m_running || m_hProcess == INVALID_HANDLE_VALUE) {
        return false;
    }

    DWORD exitCode;
    if (GetExitCodeProcess(m_hProcess, &exitCode)) {
        return exitCode == STILL_ACTIVE;
    }
    return false;
}

int ConPTYTerminal::GetExitCode() const {
    if (IsRunning()) {
        return -1;  // Still running
    }
    // Return -1 if never launched (m_processId == 0)
    if (m_processId == 0) {
        return -1;
    }
    return static_cast<int>(m_exitCode);
}

int ConPTYTerminal::GetProcessId() const {
    // Return -1 if not running or never launched
    if (m_processId == 0) {
        return -1;
    }
    return static_cast<int>(m_processId);
}

void ConPTYTerminal::Write(const std::string& text) {
    Write(text.c_str(), text.length());
}

void ConPTYTerminal::Write(const char* data, size_t length) {
    if (!m_running || m_hPipeIn == INVALID_HANDLE_VALUE) {
        return;
    }

    // Guard against size overflow on 64-bit systems
    if (length > MAXDWORD) {
        InvokeErrorCallback("Write data too large");
        return;
    }

    DWORD written = 0;
    if (!WriteFile(m_hPipeIn, data, static_cast<DWORD>(length), &written, nullptr)) {
        DWORD err = GetLastError();
        InvokeErrorCallback("Write failed with error: " + std::to_string(err));
    }
}

void ConPTYTerminal::SetOutputCallback(OutputCallback callback) {
    std::lock_guard<std::mutex> lock(m_callbackMutex);
    m_outputCallback = std::move(callback);
}

void ConPTYTerminal::SetExitCallback(ExitCallback callback) {
    std::lock_guard<std::mutex> lock(m_callbackMutex);
    m_exitCallback = std::move(callback);
}

void ConPTYTerminal::SetErrorCallback(ErrorCallback callback) {
    std::lock_guard<std::mutex> lock(m_callbackMutex);
    m_errorCallback = std::move(callback);
}

void ConPTYTerminal::Resize(int cols, int rows) {
    m_cols = cols;
    m_rows = rows;

    // Only resize the PTY if it's running
    if (m_running && m_hPC != INVALID_HANDLE_VALUE) {
        COORD size = { static_cast<SHORT>(cols), static_cast<SHORT>(rows) };
        ResizePseudoConsole(m_hPC, size);
    }
}

void ConPTYTerminal::Clear() {
    // Send ANSI clear screen sequence
    Write("\x1b[2J\x1b[H");
}

void ConPTYTerminal::SetTheme(const TerminalTheme& theme) {
    m_theme = theme;
    // Note: ConPTY doesn't apply themes directly - that's done by the rendering frontend
    // (WebViewTerminal or ImGuiTerminal). We just store the theme for retrieval.
}

void ConPTYTerminal::ReadThreadFunc() {
    char buffer[4096];

    while (m_running && m_hPipeOut != INVALID_HANDLE_VALUE) {
        DWORD bytesRead = 0;
        BOOL success = ReadFile(m_hPipeOut, buffer, sizeof(buffer), &bytesRead, nullptr);

        if (!success || bytesRead == 0) {
            // Pipe closed or error
            break;
        }

        // Invoke callback with received data
        InvokeOutputCallback(buffer, bytesRead);
    }

    // Process has exited - get exit code and invoke exit callback
    if (m_hProcess != INVALID_HANDLE_VALUE) {
        GetExitCodeProcess(m_hProcess, &m_exitCode);
        InvokeExitCallback(static_cast<int>(m_exitCode));
    }

    m_running = false;
}

void ConPTYTerminal::CleanupHandles() {
    // Close pseudo console handle if not already closed
    if (m_hPC != INVALID_HANDLE_VALUE) {
        ClosePseudoConsole(m_hPC);
        m_hPC = INVALID_HANDLE_VALUE;
    }
    if (m_hPipeIn != INVALID_HANDLE_VALUE) {
        CloseHandle(m_hPipeIn);
        m_hPipeIn = INVALID_HANDLE_VALUE;
    }
    if (m_hPipeOut != INVALID_HANDLE_VALUE) {
        CloseHandle(m_hPipeOut);
        m_hPipeOut = INVALID_HANDLE_VALUE;
    }
    if (m_hProcess != INVALID_HANDLE_VALUE) {
        CloseHandle(m_hProcess);
        m_hProcess = INVALID_HANDLE_VALUE;
    }
    if (m_hThread != INVALID_HANDLE_VALUE) {
        CloseHandle(m_hThread);
        m_hThread = INVALID_HANDLE_VALUE;
    }
    m_processId = 0;
}

void ConPTYTerminal::InvokeOutputCallback(const char* data, size_t length) {
    // Copy callback under lock, invoke outside to prevent deadlock
    OutputCallback callback;
    {
        std::lock_guard<std::mutex> lock(m_callbackMutex);
        callback = m_outputCallback;
    }
    if (callback) {
        std::string text(data, length);
        callback(text);
    }
}

void ConPTYTerminal::InvokeExitCallback(int exitCode) {
    // Copy callback under lock, invoke outside to prevent deadlock
    ExitCallback callback;
    {
        std::lock_guard<std::mutex> lock(m_callbackMutex);
        callback = m_exitCallback;
    }
    if (callback) {
        callback(exitCode);
    }
}

void ConPTYTerminal::InvokeErrorCallback(const std::string& error) {
    // Copy callback under lock, invoke outside to prevent deadlock
    ErrorCallback callback;
    {
        std::lock_guard<std::mutex> lock(m_callbackMutex);
        callback = m_errorCallback;
    }
    if (callback) {
        callback(error);
    }
}

#else
// Non-Windows stub implementations

bool ConPTYTerminal::Launch(const std::string& command,
                            const std::vector<std::string>& args,
                            const std::string& workingDir,
                            int cols,
                            int rows) {
    (void)command; (void)args; (void)workingDir; (void)cols; (void)rows;
    InvokeErrorCallback("ConPTY: Not implemented on this platform");
    return false;
}

void ConPTYTerminal::Terminate() {}
bool ConPTYTerminal::IsRunning() const { return false; }
int ConPTYTerminal::GetExitCode() const { return -1; }
int ConPTYTerminal::GetProcessId() const { return -1; }
void ConPTYTerminal::Write(const std::string& text) { (void)text; }
void ConPTYTerminal::Write(const char* data, size_t length) { (void)data; (void)length; }
void ConPTYTerminal::SetOutputCallback(OutputCallback callback) { (void)callback; }
void ConPTYTerminal::SetExitCallback(ExitCallback callback) { (void)callback; }
void ConPTYTerminal::SetErrorCallback(ErrorCallback callback) { (void)callback; }
void ConPTYTerminal::Resize(int cols, int rows) { (void)cols; (void)rows; }
void ConPTYTerminal::Clear() {}
void ConPTYTerminal::SetTheme(const TerminalTheme& theme) { (void)theme; }
void ConPTYTerminal::ReadThreadFunc() {}
void ConPTYTerminal::CleanupHandles() {}
void ConPTYTerminal::InvokeOutputCallback(const char* data, size_t length) { (void)data; (void)length; }
void ConPTYTerminal::InvokeExitCallback(int exitCode) { (void)exitCode; }
void ConPTYTerminal::InvokeErrorCallback(const std::string& error) { (void)error; }

#endif

} // namespace smith::terminal

#include "conpty_terminal.h"
#include "output_log.h"
#include <iostream>
#include <sstream>

#ifdef PLATFORM_WINDOWS
#include <windows.h>
#include <process.h>

// ConPTY API requires Windows 10 1809+
#ifndef PROC_THREAD_ATTRIBUTE_PSEUDOCONSOLE
#define PROC_THREAD_ATTRIBUTE_PSEUDOCONSOLE 0x00020016
#endif
#endif

namespace AgentSmith {

ConPTYTerminal::ConPTYTerminal() = default;

ConPTYTerminal::~ConPTYTerminal() {
    Terminate();
}

ConPTYTerminal::ConPTYTerminal(ConPTYTerminal&& other) noexcept {
#ifdef PLATFORM_WINDOWS
    m_hPC = other.m_hPC;
    m_hPipeIn = other.m_hPipeIn;
    m_hPipeOut = other.m_hPipeOut;
    m_hProcess = other.m_hProcess;
    m_hThread = other.m_hThread;
    m_processId = other.m_processId;

    other.m_hPC = INVALID_HANDLE_VALUE;
    other.m_hPipeIn = INVALID_HANDLE_VALUE;
    other.m_hPipeOut = INVALID_HANDLE_VALUE;
    other.m_hProcess = INVALID_HANDLE_VALUE;
    other.m_hThread = INVALID_HANDLE_VALUE;
    other.m_processId = 0;
#endif

    m_running.store(other.m_running.load());
    other.m_running = false;

    m_readThread = std::move(other.m_readThread);
    m_outputCallback = std::move(other.m_outputCallback);
    m_cols = other.m_cols;
    m_rows = other.m_rows;
}

ConPTYTerminal& ConPTYTerminal::operator=(ConPTYTerminal&& other) noexcept {
    if (this != &other) {
        Terminate();

#ifdef PLATFORM_WINDOWS
        m_hPC = other.m_hPC;
        m_hPipeIn = other.m_hPipeIn;
        m_hPipeOut = other.m_hPipeOut;
        m_hProcess = other.m_hProcess;
        m_hThread = other.m_hThread;
        m_processId = other.m_processId;

        other.m_hPC = INVALID_HANDLE_VALUE;
        other.m_hPipeIn = INVALID_HANDLE_VALUE;
        other.m_hPipeOut = INVALID_HANDLE_VALUE;
        other.m_hProcess = INVALID_HANDLE_VALUE;
        other.m_hThread = INVALID_HANDLE_VALUE;
        other.m_processId = 0;
#endif

        m_running.store(other.m_running.load());
        other.m_running = false;

        m_readThread = std::move(other.m_readThread);
        m_outputCallback = std::move(other.m_outputCallback);
        m_cols = other.m_cols;
        m_rows = other.m_rows;
    }
    return *this;
}

#ifdef PLATFORM_WINDOWS

bool ConPTYTerminal::Launch(const std::string& command,
                            const std::vector<std::string>& args,
                            const std::string& working_directory,
                            int cols,
                            int rows) {
    if (m_running) {
        LOG_WARNING_SRC("Launch called but already running", "ConPTY");
        return false;  // Already running
    }

    LOG_DEBUG_SRC("Creating ConPTY terminal (" + std::to_string(cols) + "x" + std::to_string(rows) + ")", "ConPTY");

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
        LOG_ERROR_SRC("Failed to create input pipe", "ConPTY");
        return false;
    }

    // Create output pipe (PTY writes to hPipeOutWrite, we read from hPipeOutRead)
    if (!CreatePipe(&hPipeOutRead, &hPipeOutWrite, &sa, 0)) {
        CloseHandle(hPipeInRead);
        CloseHandle(hPipeInWrite);
        LOG_ERROR_SRC("Failed to create output pipe", "ConPTY");
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
        LOG_ERROR_SRC("Failed to create pseudo console: 0x" + std::to_string(hr), "ConPTY");
        return false;
    }

    LOG_DEBUG_SRC("Pseudo console created successfully", "ConPTY");

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
    if (!working_directory.empty()) {
        wideLen = MultiByteToWideChar(CP_UTF8, 0, working_directory.c_str(), -1, nullptr, 0);
        wideWorkDir.resize(wideLen);
        MultiByteToWideChar(CP_UTF8, 0, working_directory.c_str(), -1, &wideWorkDir[0], wideLen);
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
        LOG_ERROR_SRC("Failed to allocate attribute list", "ConPTY");
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
        LOG_ERROR_SRC("Failed to initialize attribute list", "ConPTY");
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
        LOG_ERROR_SRC("Failed to update attribute", "ConPTY");
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
        LOG_ERROR_SRC("Failed to create process, error: " + std::to_string(err), "ConPTY");
        return false;
    }

    m_hProcess = pi.hProcess;
    m_hThread = pi.hThread;
    m_processId = pi.dwProcessId;
    m_running = true;

    LOG_INFO_SRC("Process created successfully, PID: " + std::to_string(m_processId), "ConPTY");

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

void ConPTYTerminal::Write(const std::string& data) {
    Write(data.c_str(), data.length());
}

void ConPTYTerminal::Write(const char* data, size_t length) {
    if (!m_running || m_hPipeIn == INVALID_HANDLE_VALUE) {
        return;
    }

    DWORD written;
    WriteFile(m_hPipeIn, data, static_cast<DWORD>(length), &written, nullptr);
}

void ConPTYTerminal::Resize(int cols, int rows) {
    if (!m_running || m_hPC == INVALID_HANDLE_VALUE) {
        return;
    }

    m_cols = cols;
    m_rows = rows;

    COORD size = { static_cast<SHORT>(cols), static_cast<SHORT>(rows) };
    ResizePseudoConsole(m_hPC, size);
}

void ConPTYTerminal::SetOutputCallback(OutputCallback callback) {
    std::lock_guard<std::mutex> lock(m_callbackMutex);
    m_outputCallback = std::move(callback);
}

int ConPTYTerminal::GetProcessId() const {
    return static_cast<int>(m_processId);
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
        std::lock_guard<std::mutex> lock(m_callbackMutex);
        if (m_outputCallback) {
            m_outputCallback(buffer, bytesRead);
        }
    }

    m_running = false;
}

void ConPTYTerminal::CleanupHandles() {
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

#else
// Non-Windows stub implementations

bool ConPTYTerminal::Launch(const std::string& command,
                            const std::vector<std::string>& args,
                            const std::string& working_directory,
                            int cols,
                            int rows) {
    std::cerr << "ConPTY: Not implemented on this platform" << std::endl;
    return false;
}

void ConPTYTerminal::Terminate() {}
bool ConPTYTerminal::IsRunning() const { return false; }
void ConPTYTerminal::Write(const std::string& data) {}
void ConPTYTerminal::Write(const char* data, size_t length) {}
void ConPTYTerminal::Resize(int cols, int rows) {}
void ConPTYTerminal::SetOutputCallback(OutputCallback callback) {}
int ConPTYTerminal::GetProcessId() const { return -1; }
void ConPTYTerminal::ReadThreadFunc() {}
void ConPTYTerminal::CleanupHandles() {}

#endif

} // namespace AgentSmith

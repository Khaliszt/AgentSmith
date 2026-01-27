#include "git_utils.h"
#include <cstdio>
#include <array>
#include <memory>
#include <sstream>
#include <algorithm>

#ifdef PLATFORM_WINDOWS
#include <windows.h>
#endif

namespace AgentSmith {

#ifdef PLATFORM_WINDOWS
// Windows implementation using CreateProcess with no console window
std::string GitUtils::RunGitCommand(const std::string& directory, const std::string& args) {
    std::string result;

    // Build command line: cmd /c "cd /d "directory" && git args"
    std::string cmdLine = "cmd /c \"cd /d \"" + directory + "\" && git " + args + " 2>nul\"";

    // Create pipes for stdout
    SECURITY_ATTRIBUTES sa = {};
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = nullptr;

    HANDLE hStdOutRead = nullptr;
    HANDLE hStdOutWrite = nullptr;

    if (!CreatePipe(&hStdOutRead, &hStdOutWrite, &sa, 0)) {
        return "";
    }

    // Ensure the read handle is not inherited
    SetHandleInformation(hStdOutRead, HANDLE_FLAG_INHERIT, 0);

    STARTUPINFOA si = {};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    si.hStdOutput = hStdOutWrite;
    si.hStdError = hStdOutWrite;
    si.hStdInput = nullptr;
    si.wShowWindow = SW_HIDE;

    PROCESS_INFORMATION pi = {};

    // CREATE_NO_WINDOW prevents console window from appearing
    BOOL success = CreateProcessA(
        nullptr,
        const_cast<char*>(cmdLine.c_str()),
        nullptr,
        nullptr,
        TRUE,  // Inherit handles
        CREATE_NO_WINDOW,
        nullptr,
        nullptr,
        &si,
        &pi
    );

    // Close write end of pipe (we only read)
    CloseHandle(hStdOutWrite);

    if (!success) {
        CloseHandle(hStdOutRead);
        return "";
    }

    // Read output from pipe
    char buffer[256];
    DWORD bytesRead;
    while (ReadFile(hStdOutRead, buffer, sizeof(buffer) - 1, &bytesRead, nullptr) && bytesRead > 0) {
        buffer[bytesRead] = '\0';
        result += buffer;
    }

    // Wait for process to finish (with timeout to prevent hanging)
    WaitForSingleObject(pi.hProcess, 5000);

    // Cleanup
    CloseHandle(hStdOutRead);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    // Trim trailing newline
    while (!result.empty() && (result.back() == '\n' || result.back() == '\r')) {
        result.pop_back();
    }

    return result;
}

#else
// Unix implementation using popen
std::string GitUtils::RunGitCommand(const std::string& directory, const std::string& args) {
    std::string command = "cd \"" + directory + "\" && git " + args + " 2>/dev/null";

    std::array<char, 256> buffer;
    std::string result;

    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(command.c_str(), "r"), pclose);

    if (!pipe) {
        return "";
    }

    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }

    // Trim trailing newline
    while (!result.empty() && (result.back() == '\n' || result.back() == '\r')) {
        result.pop_back();
    }

    return result;
}
#endif

bool GitUtils::IsGitRepository(const std::string& directory) {
    std::string result = RunGitCommand(directory, "rev-parse --is-inside-work-tree");
    return result == "true";
}

std::string GitUtils::GetCurrentBranch(const std::string& directory) {
    return RunGitCommand(directory, "branch --show-current");
}

std::string GitUtils::GetRepoName(const std::string& directory) {
    std::string result = RunGitCommand(directory, "rev-parse --show-toplevel");
    
    // Extract just the directory name
    size_t last_slash = result.find_last_of("/\\");
    if (last_slash != std::string::npos) {
        return result.substr(last_slash + 1);
    }
    return result;
}

std::string GitUtils::GetLastCommitHash(const std::string& directory, bool short_hash) {
    if (short_hash) {
        return RunGitCommand(directory, "rev-parse --short HEAD");
    }
    return RunGitCommand(directory, "rev-parse HEAD");
}

std::string GitUtils::GetLastCommitMessage(const std::string& directory) {
    return RunGitCommand(directory, "log -1 --pretty=%s");
}

int GitUtils::GetUncommittedChangesCount(const std::string& directory) {
    std::string result = RunGitCommand(directory, "status --porcelain");
    
    if (result.empty()) return 0;
    
    // Count lines
    int count = 0;
    for (char c : result) {
        if (c == '\n') count++;
    }
    
    // Add 1 if doesn't end with newline
    if (!result.empty() && result.back() != '\n') count++;
    
    return count;
}

void GitUtils::GetAheadBehind(const std::string& directory, int& ahead, int& behind) {
    ahead = 0;
    behind = 0;
    
    std::string result = RunGitCommand(directory, "rev-list --left-right --count @{upstream}...HEAD");
    
    if (result.empty()) return;
    
    std::istringstream iss(result);
    iss >> behind >> ahead;
}

void GitUtils::UpdateGitInfo(Agent& agent) {
    if (agent.working_directory.empty()) {
        agent.git_info.is_git_repo = false;
        return;
    }
    
    agent.git_info.is_git_repo = IsGitRepository(agent.working_directory);
    
    if (!agent.git_info.is_git_repo) {
        return;
    }
    
    agent.git_info.branch = GetCurrentBranch(agent.working_directory);
    agent.git_info.repo_name = GetRepoName(agent.working_directory);
    agent.git_info.last_commit_hash = GetLastCommitHash(agent.working_directory, true);
    agent.git_info.last_commit_message = GetLastCommitMessage(agent.working_directory);
    agent.git_info.uncommitted_changes = GetUncommittedChangesCount(agent.working_directory);
    GetAheadBehind(agent.working_directory, agent.git_info.ahead, agent.git_info.behind);
    
    agent.git_info.last_updated = std::chrono::system_clock::now();
}

std::string GitUtils::FormatBranchDisplay(const GitInfo& info) {
    if (!info.is_git_repo || info.branch.empty()) {
        return "";
    }
    
    std::string display = info.branch;
    
    // Add ahead/behind indicators
    if (info.ahead > 0 || info.behind > 0) {
        display += " ";
        if (info.ahead > 0) {
            display += "↑" + std::to_string(info.ahead);
        }
        if (info.behind > 0) {
            display += "↓" + std::to_string(info.behind);
        }
    }
    
    // Add uncommitted changes indicator
    if (info.uncommitted_changes > 0) {
        display += " *" + std::to_string(info.uncommitted_changes);
    }
    
    return display;
}

} // namespace AgentSmith

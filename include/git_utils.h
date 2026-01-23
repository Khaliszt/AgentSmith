#pragma once

#include "types.h"
#include <string>

namespace AgentSmith {

//=============================================================================
// Git Utilities
//
// Functions to query git repository information for displaying in overlays.
// These run git commands and parse the output.
//=============================================================================

class GitUtils {
public:
    // Update all git info for an agent's working directory
    static void UpdateGitInfo(Agent& agent);
    
    // Individual queries (called by UpdateGitInfo)
    static std::string GetCurrentBranch(const std::string& directory);
    static std::string GetRepoName(const std::string& directory);
    static std::string GetLastCommitHash(const std::string& directory, bool short_hash = true);
    static std::string GetLastCommitMessage(const std::string& directory);
    static int GetUncommittedChangesCount(const std::string& directory);
    static void GetAheadBehind(const std::string& directory, int& ahead, int& behind);
    static bool IsGitRepository(const std::string& directory);
    
    // Format branch display string with status indicators
    static std::string FormatBranchDisplay(const GitInfo& info);
    
private:
    // Run a git command and return stdout
    static std::string RunGitCommand(const std::string& directory, const std::string& args);
};

} // namespace AgentSmith

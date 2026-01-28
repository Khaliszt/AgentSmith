// C:\FarfadetsCorp\AgentSmith\src\agent\agent_utils.cpp

#include "agent/agent_utils.h"
#include <sstream>
#include <iomanip>
#include <ctime>
#include <algorithm>
#include <cstdlib>

namespace smith::agent::utils {

std::string GenerateAgentId() {
    auto now = std::chrono::system_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();

    std::ostringstream oss;
    oss << "agent_" << std::hex << std::setw(12) << std::setfill('0') << ms;
    return oss.str();
}

std::string AgentTypeToString(AgentType type) {
    switch (type) {
        case AgentType::ClaudeCode: return "ClaudeCode";
        case AgentType::Grok:       return "Grok";
        case AgentType::ChatGPT:    return "ChatGPT";
        case AgentType::Cursor:     return "Cursor";
        case AgentType::Custom:     return "Custom";
        default:                    return "Unknown";
    }
}

std::optional<AgentType> StringToAgentType(const std::string& str) {
    if (str == "ClaudeCode") return AgentType::ClaudeCode;
    if (str == "Grok")       return AgentType::Grok;
    if (str == "ChatGPT")    return AgentType::ChatGPT;
    if (str == "Cursor")     return AgentType::Cursor;
    if (str == "Custom")     return AgentType::Custom;
    return std::nullopt;
}

std::string AgentStatusToString(AgentStatus status) {
    switch (status) {
        case AgentStatus::Idle:     return "Idle";
        case AgentStatus::Starting: return "Starting";
        case AgentStatus::Running:  return "Running";
        case AgentStatus::Waiting:  return "Waiting";
        case AgentStatus::Thinking: return "Thinking";
        case AgentStatus::Error:    return "Error";
        case AgentStatus::Stopped:  return "Stopped";
        default:                    return "Unknown";
    }
}

std::optional<AgentStatus> StringToAgentStatus(const std::string& str) {
    if (str == "Idle")      return AgentStatus::Idle;
    if (str == "Starting")  return AgentStatus::Starting;
    if (str == "Running")   return AgentStatus::Running;
    if (str == "Waiting")   return AgentStatus::Waiting;
    if (str == "Thinking")  return AgentStatus::Thinking;
    if (str == "Error")     return AgentStatus::Error;
    if (str == "Stopped")   return AgentStatus::Stopped;
    return std::nullopt;
}

std::string FormatDuration(std::chrono::seconds seconds) {
    auto total = seconds.count();

    if (total < 60) {
        return std::to_string(total) + "s";
    }

    auto hours = total / 3600;
    auto minutes = (total % 3600) / 60;
    auto secs = total % 60;

    std::ostringstream oss;
    if (hours > 0) {
        oss << hours << "h ";
        if (minutes > 0) {
            oss << minutes << "m";
        }
    } else {
        oss << minutes << "m";
        if (secs > 0) {
            oss << " " << secs << "s";
        }
    }

    return oss.str();
}

std::string FormatCost(double cost) {
    std::ostringstream oss;
    oss << "$" << std::fixed;

    if (cost < 0.01) {
        oss << std::setprecision(4) << cost;
    } else if (cost < 1.0) {
        oss << std::setprecision(3) << cost;
    } else {
        oss << std::setprecision(2) << cost;
    }

    return oss.str();
}

std::string FormatTokens(uint64_t tokens) {
    std::string str = std::to_string(tokens);
    std::string result;
    result.reserve(str.size() + str.size() / 3);

    int count = 0;
    for (auto it = str.rbegin(); it != str.rend(); ++it) {
        if (count > 0 && count % 3 == 0) {
            result.push_back(',');
        }
        result.push_back(*it);
        count++;
    }

    std::reverse(result.begin(), result.end());
    return result;
}

std::string GetApiKeyForType(AgentType type) {
    const char* envVar = nullptr;

    switch (type) {
        case AgentType::ClaudeCode:
            envVar = std::getenv("ANTHROPIC_API_KEY");
            if (!envVar) envVar = std::getenv("CLAUDE_API_KEY");
            break;
        case AgentType::Grok:
            envVar = std::getenv("XAI_API_KEY");
            if (!envVar) envVar = std::getenv("GROK_API_KEY");
            break;
        case AgentType::ChatGPT:
            envVar = std::getenv("OPENAI_API_KEY");
            break;
        case AgentType::Cursor:
            envVar = std::getenv("CURSOR_API_KEY");
            break;
        default:
            break;
    }

    return envVar ? std::string(envVar) : std::string();
}

std::string GetDefaultApiEndpoint(AgentType type) {
    switch (type) {
        case AgentType::Grok:
            return "https://api.x.ai/v1/chat/completions";
        case AgentType::ChatGPT:
            return "https://api.openai.com/v1/chat/completions";
        default:
            return "";
    }
}

std::string GetDefaultModelId(AgentType type) {
    switch (type) {
        case AgentType::ClaudeCode:
            return "claude-sonnet-4.5";
        case AgentType::Grok:
            return "grok-2-latest";
        case AgentType::ChatGPT:
            return "gpt-4-turbo";
        default:
            return "";
    }
}

std::string ValidateAgent(const Agent& agent) {
    // Check basic fields
    if (agent.name.empty()) {
        return "Agent name cannot be empty";
    }

    // Type-specific validation
    if (agent.RequiresTerminal()) {
        if (agent.command.empty()) {
            return "Terminal-based agent requires a command";
        }
        if (agent.workingDirectory.empty()) {
            return "Terminal-based agent requires a working directory";
        }
    }

    if (agent.UsesAPI()) {
        if (agent.apiEndpoint.empty()) {
            return "API-based agent requires an endpoint";
        }
        if (agent.apiKey.empty()) {
            // Check environment
            std::string envKey = GetApiKeyForType(agent.type);
            if (envKey.empty()) {
                return "API-based agent requires an API key (set in config or environment)";
            }
        }
    }

    return ""; // Valid
}

std::string GetMetricsSummary(const AgentMetrics& metrics) {
    std::ostringstream oss;

    uint64_t totalTokens = metrics.GetTotalTokens();
    if (totalTokens > 0) {
        if (totalTokens >= 1000) {
            oss << (totalTokens / 1000.0) << "K tokens";
        } else {
            oss << totalTokens << " tokens";
        }
    }

    double cost = metrics.GetEffectiveCost();
    if (cost > 0.0) {
        if (totalTokens > 0) oss << ", ";
        oss << FormatCost(cost);
    }

    if (metrics.toolCalls > 0) {
        if (totalTokens > 0 || cost > 0.0) oss << ", ";
        oss << metrics.toolCalls << " tool call";
        if (metrics.toolCalls > 1) oss << "s";
    }

    if (metrics.apiCalls > 0) {
        if (totalTokens > 0 || cost > 0.0 || metrics.toolCalls > 0) oss << ", ";
        oss << metrics.apiCalls << " API call";
        if (metrics.apiCalls > 1) oss << "s";
    }

    std::string result = oss.str();
    return result.empty() ? "No activity" : result;
}

} // namespace smith::agent::utils

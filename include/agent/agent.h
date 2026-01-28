// C:\FarfadetsCorp\AgentSmith\include\agent\agent.h

#pragma once

#include "agent_metrics.h"
#include "core/types.h"
#include <string>
#include <chrono>
#include <memory>
#include <nlohmann/json.hpp>

namespace smith::terminal {
    class ITerminal;
}

namespace smith::agent {

class IAgentProvider;

// AgentType and AgentStatus are defined in core/types.h

/**
 * @brief Main agent data structure
 *
 * Represents a running agent instance with all its state, metrics,
 * and associated resources (terminal, provider, etc.).
 */
struct Agent {
    // === Identification ===
    std::string id;                     // Unique agent identifier (UUID or timestamp-based)
    std::string name;                   // User-friendly name
    AgentType type = AgentType::ClaudeCode;
    AgentStatus status = AgentStatus::Idle;

    // === Process Information (Terminal-based agents) ===
    std::string workingDirectory;       // CWD for the agent process
    std::string command;                // Launch command (e.g., "claude-code")
    std::vector<std::string> args;      // Command arguments
    std::map<std::string, std::string> environment; // Environment variables

    // === API Configuration (API-based agents) ===
    std::string apiKey;                 // API key (or reference to env var)
    std::string apiEndpoint;            // API endpoint URL
    std::string modelId;                // Specific model ID
    std::map<std::string, std::string> apiParams; // Additional API parameters

    // === Runtime State ===
    std::shared_ptr<terminal::ITerminal> terminal; // Terminal instance (if applicable)
    std::shared_ptr<IAgentProvider> provider;      // Provider instance
    AgentMetrics metrics;                          // Token/cost tracking

    // === Timing ===
    std::chrono::system_clock::time_point createdTime;
    std::chrono::system_clock::time_point lastActivityTime;
    std::chrono::seconds uptime{0};

    // === Configuration ===
    bool autoRestart = false;           // Restart on crash
    int maxRestarts = 3;                // Max auto-restart attempts
    int restartCount = 0;               // Current restart count
    bool logOutput = true;              // Log agent output

    // === Factory Methods ===

    /**
     * @brief Creates a new agent with default settings
     */
    static Agent Create(AgentType type, const std::string& name, const std::string& workingDir);

    /**
     * @brief Creates an agent from JSON configuration
     */
    static Agent FromConfig(const nlohmann::json& config);

    /**
     * @brief Serializes agent to JSON configuration
     */
    nlohmann::json ToConfig() const;

    // === Helper Methods ===

    /**
     * @brief Checks if this agent type requires a terminal
     */
    bool RequiresTerminal() const {
        return type == AgentType::ClaudeCode || type == AgentType::Cursor;
    }

    /**
     * @brief Checks if this agent type uses API
     */
    bool UsesAPI() const {
        return type == AgentType::Grok || type == AgentType::ChatGPT;
    }

    /**
     * @brief Gets the display name for this agent
     */
    std::string GetDisplayName() const {
        return name.empty() ? id : name;
    }

    /**
     * @brief Updates uptime based on current time
     */
    void UpdateUptime() {
        auto now = std::chrono::system_clock::now();
        uptime = std::chrono::duration_cast<std::chrono::seconds>(now - createdTime);
    }

    /**
     * @brief Updates last activity time
     */
    void UpdateActivity() {
        lastActivityTime = std::chrono::system_clock::now();
    }

    /**
     * @brief Checks if agent is active (Running, Waiting, or Thinking)
     */
    bool IsActive() const {
        return status == AgentStatus::Running ||
               status == AgentStatus::Waiting ||
               status == AgentStatus::Thinking;
    }

    /**
     * @brief Checks if agent has encountered an error
     */
    bool HasError() const {
        return status == AgentStatus::Error;
    }
};

} // namespace smith::agent

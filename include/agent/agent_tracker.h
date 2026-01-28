// C:\FarfadetsCorp\AgentSmith\include\agent\agent_tracker.h

#pragma once

#include "agent.h"
#include "agent_provider.h"
#include "core/result.h"
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <mutex>
#include <nlohmann/json.hpp>

namespace smith::agent {

/**
 * @brief Callback for agent lifecycle events
 */
using AgentEventCallback = std::function<void(const Agent& agent)>;

/**
 * @brief Manages all agent instances and their lifecycle
 *
 * This class is responsible for:
 * - Creating and destroying agents
 * - Starting and stopping agents
 * - Tracking agent state and metrics
 * - Saving/loading agent configurations
 * - Coordinating with providers
 */
class AgentTracker {
public:
    AgentTracker();
    ~AgentTracker();

    // === Agent Lifecycle ===

    /**
     * @brief Creates a new agent
     *
     * @param type Agent type
     * @param name Agent name
     * @param workingDir Working directory
     * @return Result with agent ID or error
     */
    core::Result<std::string> CreateAgent(AgentType type, const std::string& name,
                                          const std::string& workingDir);

    /**
     * @brief Creates an agent from JSON configuration
     *
     * @param config JSON configuration
     * @return Result with agent ID or error
     */
    core::Result<std::string> CreateAgentFromConfig(const nlohmann::json& config);

    /**
     * @brief Starts an agent
     *
     * @param agentId Agent ID
     * @return Result indicating success or error
     */
    core::Result<void> StartAgent(const std::string& agentId);

    /**
     * @brief Stops an agent
     *
     * @param agentId Agent ID
     * @return Result indicating success or error
     */
    core::Result<void> StopAgent(const std::string& agentId);

    /**
     * @brief Restarts an agent
     *
     * @param agentId Agent ID
     * @return Result indicating success or error
     */
    core::Result<void> RestartAgent(const std::string& agentId);

    /**
     * @brief Removes an agent (stops if running, then deletes)
     *
     * @param agentId Agent ID
     * @return Result indicating success or error
     */
    core::Result<void> RemoveAgent(const std::string& agentId);

    /**
     * @brief Stops and removes all agents
     */
    void RemoveAllAgents();

    // === Agent Access ===

    /**
     * @brief Gets an agent by ID
     *
     * @param agentId Agent ID
     * @return Pointer to agent, or nullptr if not found
     */
    Agent* GetAgent(const std::string& agentId);
    const Agent* GetAgent(const std::string& agentId) const;

    /**
     * @brief Gets all agents
     *
     * @return Vector of agent pointers
     */
    std::vector<Agent*> GetAllAgents();
    std::vector<const Agent*> GetAllAgents() const;

    /**
     * @brief Gets agents by status
     *
     * @param status Status to filter by
     * @return Vector of matching agents
     */
    std::vector<Agent*> GetAgentsByStatus(AgentStatus status);

    /**
     * @brief Gets agents by type
     *
     * @param type Type to filter by
     * @return Vector of matching agents
     */
    std::vector<Agent*> GetAgentsByType(AgentType type);

    /**
     * @brief Gets count of agents
     *
     * @return Total agent count
     */
    size_t GetAgentCount() const;

    // === Agent Communication ===

    /**
     * @brief Sends input to an agent
     *
     * @param agentId Agent ID
     * @param input Input text
     * @return Result indicating success or error
     */
    core::Result<void> SendInput(const std::string& agentId, const std::string& input);

    /**
     * @brief Sends a command to an agent
     *
     * @param agentId Agent ID
     * @param command Command text
     * @return Result indicating success or error
     */
    core::Result<void> SendCommand(const std::string& agentId, const std::string& command);

    // === Configuration ===

    /**
     * @brief Loads agents from configuration file
     *
     * @param configPath Path to config file
     * @return Result indicating success or error
     */
    core::Result<void> LoadFromConfig(const std::string& configPath);

    /**
     * @brief Saves agents to configuration file
     *
     * @param configPath Path to config file
     * @return Result indicating success or error
     */
    core::Result<void> SaveToConfig(const std::string& configPath);

    /**
     * @brief Exports all agent configurations to JSON
     *
     * @return JSON array of agent configs
     */
    nlohmann::json ExportConfigs() const;

    // === Callbacks ===

    /**
     * @brief Sets callback for agent creation
     */
    void SetOnAgentCreated(AgentEventCallback callback) {
        m_onAgentCreated = std::move(callback);
    }

    /**
     * @brief Sets callback for agent removal
     */
    void SetOnAgentRemoved(AgentEventCallback callback) {
        m_onAgentRemoved = std::move(callback);
    }

    /**
     * @brief Sets callback for agent status change
     */
    void SetOnAgentStatusChanged(AgentEventCallback callback) {
        m_onAgentStatusChanged = std::move(callback);
    }

    // === Update ===

    /**
     * @brief Updates all agents (calls provider Update())
     *
     * Should be called from main loop.
     */
    void Update();

private:
    // === Internal Methods ===

    Agent* FindAgent(const std::string& agentId);
    const Agent* FindAgent(const std::string& agentId) const;

    void SetupProviderCallbacks(Agent& agent);

    // === State ===
    std::vector<std::unique_ptr<Agent>> m_agents;
    mutable std::recursive_mutex m_mutex;  // Recursive to allow callbacks during Update()

    // === Callbacks ===
    AgentEventCallback m_onAgentCreated;
    AgentEventCallback m_onAgentRemoved;
    AgentEventCallback m_onAgentStatusChanged;
};

} // namespace smith::agent

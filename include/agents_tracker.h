#pragma once

#include "types.h"
#include <vector>
#include <memory>
#include <functional>
#include <string>

namespace AgentSmith {

//=============================================================================
// Agents Tracker (Singleton)
//
// Central management of all agents. Replaces AgentManager with singleton
// pattern for easier access across the application.
//
// Responsibilities:
// - Load/save agents from config
// - Create/destroy agents
// - Track active/running agents
// - Observer pattern for state changes
//=============================================================================

class AgentsTracker {
public:
    // Singleton access
    static AgentsTracker& Instance();

    // Prevent copying
    AgentsTracker(const AgentsTracker&) = delete;
    AgentsTracker& operator=(const AgentsTracker&) = delete;

    // Agent lifecycle
    Agent& CreateAgent(const std::string& name,
                       const std::string& workingDir,
                       AgentType type = AgentType::ClaudeCode);
    void RemoveAgent(const std::string& agentId);
    void RemoveAgentByIndex(size_t index);

    // Access agents
    std::vector<Agent>& GetAgents() { return m_agents; }
    const std::vector<Agent>& GetAgents() const { return m_agents; }
    Agent* GetAgent(const std::string& id);
    Agent* GetAgentByIndex(size_t index);
    size_t GetAgentCount() const { return m_agents.size(); }

    // Find agents
    std::vector<Agent*> GetRunningAgents();
    std::vector<Agent*> GetAgentsNeedingAttention();

    // Batch operations
    void StartAllAgents();
    void StopAllAgents();
    void RestartAllAgents();

    // Update (call periodically)
    void Update();

    // Git info update
    void UpdateAllGitInfo();

    // Persistence
    void LoadAgents(const std::vector<Agent>& agents);
    std::vector<Agent> SaveAgents() const;

    // Observer pattern for state changes
    using AgentAddedCallback = std::function<void(Agent& agent)>;
    using AgentRemovedCallback = std::function<void(const std::string& agentId)>;
    using AgentStatusCallback = std::function<void(Agent& agent, AgentStatus oldStatus, AgentStatus newStatus)>;

    void SetAgentAddedCallback(AgentAddedCallback callback);
    void SetAgentRemovedCallback(AgentRemovedCallback callback);
    void SetStatusChangeCallback(AgentStatusCallback callback);

    // Clear all attention flags
    void ClearAttentionFlags();

private:
    AgentsTracker();
    ~AgentsTracker();

    // Assign default command based on agent type
    void AssignDefaultCommand(Agent& agent);

    std::vector<Agent> m_agents;

    // Callbacks
    AgentAddedCallback m_addedCallback;
    AgentRemovedCallback m_removedCallback;
    AgentStatusCallback m_statusCallback;
};

} // namespace AgentSmith

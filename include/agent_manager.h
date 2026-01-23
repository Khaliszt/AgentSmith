#pragma once

#include "types.h"
#include <vector>
#include <memory>
#include <functional>

namespace AgentSmith {

//=============================================================================
// Agent Manager
//
// Manages the collection of agents, handles creation/deletion, and provides
// persistence. Also monitors agent processes for status changes.
//=============================================================================

class AgentManager {
public:
    AgentManager();
    ~AgentManager();
    
    // Agent lifecycle
    Agent& CreateAgent(const std::string& name, const std::string& working_dir, AgentType type = AgentType::ClaudeCode);
    void RemoveAgent(const std::string& agent_id);
    void RemoveAgent(size_t index);
    
    // Access agents
    std::vector<Agent>& GetAgents() { return m_agents; }
    const std::vector<Agent>& GetAgents() const { return m_agents; }
    Agent* GetAgent(const std::string& id);
    Agent* GetAgentByIndex(size_t index);
    size_t GetAgentCount() const { return m_agents.size(); }
    
    // Batch operations
    void StartAllAgents();
    void StopAllAgents();
    void RestartAllAgents();
    
    // Update agent statuses (call periodically)
    void Update();
    
    // Update git info for all agents (call less frequently)
    void UpdateAllGitInfo();
    
    // Persistence
    void LoadAgents(const std::string& filepath);
    void SaveAgents(const std::string& filepath);
    
    // Event callbacks
    using AgentStatusCallback = std::function<void(Agent&, AgentStatus old_status, AgentStatus new_status)>;
    void SetStatusChangeCallback(AgentStatusCallback callback) { m_status_callback = callback; }
    
private:
    std::vector<Agent> m_agents;
    AgentStatusCallback m_status_callback;
    
    // Assign default command based on agent type
    void AssignDefaultCommand(Agent& agent);
};

} // namespace AgentSmith

#include "agents_tracker.h"
#include "git_utils.h"
#include <algorithm>

namespace AgentSmith {

AgentsTracker& AgentsTracker::Instance() {
    static AgentsTracker instance;
    return instance;
}

AgentsTracker::AgentsTracker() = default;
AgentsTracker::~AgentsTracker() = default;

Agent& AgentsTracker::CreateAgent(const std::string& name,
                                   const std::string& workingDir,
                                   AgentType type) {
    auto agent = std::make_unique<Agent>(name, workingDir, type);
    AssignDefaultCommand(*agent);

    // Initial git info fetch
    GitUtils::UpdateGitInfo(*agent);

    m_agents.push_back(std::move(agent));

    Agent& newAgent = *m_agents.back();

    // Notify observers
    if (m_addedCallback) {
        m_addedCallback(newAgent);
    }

    return newAgent;
}

void AgentsTracker::RemoveAgent(const std::string& agentId) {
    auto it = std::find_if(m_agents.begin(), m_agents.end(),
        [&agentId](const std::unique_ptr<Agent>& a) { return a->id == agentId; });

    if (it != m_agents.end()) {
        std::string id = (*it)->id;
        m_agents.erase(it);

        // Notify observers
        if (m_removedCallback) {
            m_removedCallback(id);
        }
    }
}

void AgentsTracker::RemoveAgentByIndex(size_t index) {
    if (index < m_agents.size()) {
        std::string id = m_agents[index]->id;
        m_agents.erase(m_agents.begin() + static_cast<ptrdiff_t>(index));

        // Notify observers
        if (m_removedCallback) {
            m_removedCallback(id);
        }
    }
}

std::vector<Agent*> AgentsTracker::GetAgents() {
    std::vector<Agent*> result;
    result.reserve(m_agents.size());
    for (auto& agent : m_agents) {
        result.push_back(agent.get());
    }
    return result;
}

std::vector<const Agent*> AgentsTracker::GetAgents() const {
    std::vector<const Agent*> result;
    result.reserve(m_agents.size());
    for (const auto& agent : m_agents) {
        result.push_back(agent.get());
    }
    return result;
}

Agent* AgentsTracker::GetAgent(const std::string& id) {
    auto it = std::find_if(m_agents.begin(), m_agents.end(),
        [&id](const std::unique_ptr<Agent>& a) { return a->id == id; });
    return (it != m_agents.end()) ? it->get() : nullptr;
}

Agent* AgentsTracker::GetAgentByIndex(size_t index) {
    return (index < m_agents.size()) ? m_agents[index].get() : nullptr;
}

std::vector<Agent*> AgentsTracker::GetRunningAgents() {
    std::vector<Agent*> running;
    for (auto& agent : m_agents) {
        if (agent->status == AgentStatus::Running) {
            running.push_back(agent.get());
        }
    }
    return running;
}

std::vector<Agent*> AgentsTracker::GetAgentsNeedingAttention() {
    std::vector<Agent*> attention;
    for (auto& agent : m_agents) {
        if (agent->needs_attention) {
            attention.push_back(agent.get());
        }
    }
    return attention;
}

void AgentsTracker::StartAllAgents() {
    for (auto& agent : m_agents) {
        if (agent->status == AgentStatus::Idle || agent->status == AgentStatus::Stopped) {
            AgentStatus oldStatus = agent->status;
            agent->status = AgentStatus::Running;
            agent->started_at = std::chrono::system_clock::now();

            if (m_statusCallback) {
                m_statusCallback(*agent, oldStatus, agent->status);
            }
        }
    }
}

void AgentsTracker::StopAllAgents() {
    for (auto& agent : m_agents) {
        if (agent->status == AgentStatus::Running || agent->status == AgentStatus::Waiting) {
            AgentStatus oldStatus = agent->status;
            agent->status = AgentStatus::Stopped;

            if (m_statusCallback) {
                m_statusCallback(*agent, oldStatus, agent->status);
            }
        }
    }
}

void AgentsTracker::RestartAllAgents() {
    StopAllAgents();
    StartAllAgents();
}

void AgentsTracker::Update() {
    // Check for process status changes
    for (auto& agent : m_agents) {
        if (agent->pid > 0 && agent->status == AgentStatus::Running) {
            agent->last_activity = std::chrono::system_clock::now();
        }
    }
}

void AgentsTracker::UpdateAllGitInfo() {
    for (auto& agent : m_agents) {
        GitUtils::UpdateGitInfo(*agent);
    }
}

void AgentsTracker::LoadAgents(const std::vector<Agent>& agents) {
    m_agents.clear();
    m_agents.reserve(agents.size());

    for (const auto& agent : agents) {
        auto newAgent = std::make_unique<Agent>(agent);  // Copy construct
        GitUtils::UpdateGitInfo(*newAgent);

        // Notify observers
        if (m_addedCallback) {
            m_addedCallback(*newAgent);
        }

        m_agents.push_back(std::move(newAgent));
    }
}

std::vector<Agent> AgentsTracker::SaveAgents() const {
    std::vector<Agent> result;
    result.reserve(m_agents.size());
    for (const auto& agent : m_agents) {
        result.push_back(*agent);  // Copy
    }
    return result;
}

void AgentsTracker::SetAgentAddedCallback(AgentAddedCallback callback) {
    m_addedCallback = std::move(callback);
}

void AgentsTracker::SetAgentRemovedCallback(AgentRemovedCallback callback) {
    m_removedCallback = std::move(callback);
}

void AgentsTracker::SetStatusChangeCallback(AgentStatusCallback callback) {
    m_statusCallback = std::move(callback);
}

void AgentsTracker::ClearAttentionFlags() {
    for (auto& agent : m_agents) {
        agent->needs_attention = false;
    }
}

void AgentsTracker::AssignDefaultCommand(Agent& agent) {
    switch (agent.type) {
        case AgentType::ClaudeCode:
            agent.command = "claude";
            agent.args = {};
            break;
        case AgentType::Aider:
            agent.command = "aider";
            agent.args = {};
            break;
        case AgentType::Cursor:
            agent.command = "cursor";
            agent.args = {"--folder", agent.working_directory};
            break;
        case AgentType::Custom:
        default:
            agent.command = "";
            agent.args = {};
            break;
    }
}

} // namespace AgentSmith

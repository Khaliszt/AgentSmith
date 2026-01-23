#include "agent_manager.h"
#include "git_utils.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <algorithm>

using json = nlohmann::json;

namespace AgentSmith {

AgentManager::AgentManager() = default;
AgentManager::~AgentManager() = default;

Agent& AgentManager::CreateAgent(const std::string& name, const std::string& working_dir, AgentType type) {
    Agent agent(name, working_dir, type);
    AssignDefaultCommand(agent);
    
    // Initial git info fetch
    GitUtils::UpdateGitInfo(agent);
    
    m_agents.push_back(std::move(agent));
    return m_agents.back();
}

void AgentManager::RemoveAgent(const std::string& agent_id) {
    auto it = std::find_if(m_agents.begin(), m_agents.end(),
        [&agent_id](const Agent& a) { return a.id == agent_id; });
    
    if (it != m_agents.end()) {
        m_agents.erase(it);
    }
}

void AgentManager::RemoveAgent(size_t index) {
    if (index < m_agents.size()) {
        m_agents.erase(m_agents.begin() + index);
    }
}

Agent* AgentManager::GetAgent(const std::string& id) {
    auto it = std::find_if(m_agents.begin(), m_agents.end(),
        [&id](const Agent& a) { return a.id == id; });
    return (it != m_agents.end()) ? &(*it) : nullptr;
}

Agent* AgentManager::GetAgentByIndex(size_t index) {
    return (index < m_agents.size()) ? &m_agents[index] : nullptr;
}

void AgentManager::StartAllAgents() {
    for (auto& agent : m_agents) {
        if (agent.status == AgentStatus::Idle || agent.status == AgentStatus::Stopped) {
            agent.status = AgentStatus::Running;
            agent.started_at = std::chrono::system_clock::now();
        }
    }
}

void AgentManager::StopAllAgents() {
    for (auto& agent : m_agents) {
        if (agent.status == AgentStatus::Running || agent.status == AgentStatus::Waiting) {
            agent.status = AgentStatus::Stopped;
        }
    }
}

void AgentManager::RestartAllAgents() {
    StopAllAgents();
    StartAllAgents();
}

void AgentManager::Update() {
    for (auto& agent : m_agents) {
        // Check if process is still running
        if (agent.pid > 0 && agent.status == AgentStatus::Running) {
            // TODO: Platform-specific process check
            agent.last_activity = std::chrono::system_clock::now();
        }
    }
}

void AgentManager::UpdateAllGitInfo() {
    for (auto& agent : m_agents) {
        GitUtils::UpdateGitInfo(agent);
    }
}

void AgentManager::AssignDefaultCommand(Agent& agent) {
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

void AgentManager::LoadAgents(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) return;
    
    try {
        json j;
        file >> j;
        
        if (j.contains("agents") && j["agents"].is_array()) {
            m_agents.clear();
            
            for (const auto& agent_json : j["agents"]) {
                Agent agent;
                
                if (agent_json.contains("id")) agent.id = agent_json["id"];
                if (agent_json.contains("name")) agent.name = agent_json["name"];
                if (agent_json.contains("working_directory")) agent.working_directory = agent_json["working_directory"];
                if (agent_json.contains("command")) agent.command = agent_json["command"];
                if (agent_json.contains("type")) agent.type = static_cast<AgentType>(agent_json["type"].get<int>());
                if (agent_json.contains("grid_row")) agent.grid_row = agent_json["grid_row"];
                if (agent_json.contains("grid_col")) agent.grid_col = agent_json["grid_col"];
                
                if (agent_json.contains("args") && agent_json["args"].is_array()) {
                    for (const auto& arg : agent_json["args"]) {
                        agent.args.push_back(arg);
                    }
                }
                
                // Fetch initial git info
                GitUtils::UpdateGitInfo(agent);
                
                m_agents.push_back(std::move(agent));
            }
        }
    } catch (const json::exception& e) {
        // Failed to parse, keep empty
    }
}

void AgentManager::SaveAgents(const std::string& filepath) {
    json j;
    j["agents"] = json::array();
    
    for (const auto& agent : m_agents) {
        json agent_json;
        agent_json["id"] = agent.id;
        agent_json["name"] = agent.name;
        agent_json["working_directory"] = agent.working_directory;
        agent_json["command"] = agent.command;
        agent_json["type"] = static_cast<int>(agent.type);
        agent_json["grid_row"] = agent.grid_row;
        agent_json["grid_col"] = agent.grid_col;
        agent_json["args"] = agent.args;
        
        j["agents"].push_back(agent_json);
    }
    
    std::ofstream file(filepath);
    if (file.is_open()) {
        file << j.dump(2);
    }
}

} // namespace AgentSmith

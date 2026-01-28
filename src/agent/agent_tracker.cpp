// C:\FarfadetsCorp\AgentSmith\src\agent\agent_tracker.cpp

#include "agent/agent_tracker.h"
#include "agent/agent_registry.h"
#include "agent/agent_utils.h"
#include "logging/logger.h"
#include "logging/log_macros.h"
#include <fstream>
#include <algorithm>

namespace smith::agent {

AgentTracker::AgentTracker() {
    SMITH_INFO(logging::Category::Agent, "AgentTracker initialized");
}

AgentTracker::~AgentTracker() {
    SMITH_INFO(logging::Category::Agent, "AgentTracker shutting down");
    RemoveAllAgents();
}

// === Agent Lifecycle ===

core::Result<std::string> AgentTracker::CreateAgent(AgentType type, const std::string& name,
                                                     const std::string& workingDir) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    // Create agent
    auto agent = std::make_unique<Agent>(Agent::Create(type, name, workingDir));

    // Validate
    std::string validationError = utils::ValidateAgent(*agent);
    if (!validationError.empty()) {
        SMITH_ERROR(logging::Category::Agent, "Agent validation failed: {}", validationError);
        return core::Err<std::string>(core::Error::Code::InvalidArgument, validationError);
    }

    // Create provider
    auto provider = AgentProviderRegistry::Instance().CreateProvider(type);
    if (!provider) {
        SMITH_ERROR(logging::Category::Agent, "No provider available for agent type: {}",
                    utils::AgentTypeToString(type));
        return core::Result<std::string>(core::Error(core::Error::Code::NotFound,
                                                     "No provider registered for agent type"));
    }

    // Initialize provider
    auto initResult = provider->Initialize(*agent);
    if (!initResult) {
        SMITH_ERROR(logging::Category::Agent, "Provider initialization failed: {}",
                    initResult.GetError().ToString());
        return core::Result<std::string>(initResult.GetError());
    }

    agent->provider = std::move(provider);

    // Setup callbacks
    SetupProviderCallbacks(*agent);

    std::string agentId = agent->id;

    SMITH_INFO(logging::Category::Agent, "Created agent: {} ({})", name, agentId);

    m_agents.push_back(std::move(agent));

    // Notify callback
    if (m_onAgentCreated) {
        m_onAgentCreated(*m_agents.back());
    }

    return core::Result<std::string>(std::move(agentId));
}

core::Result<std::string> AgentTracker::CreateAgentFromConfig(const nlohmann::json& config) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    try {
        auto agent = std::make_unique<Agent>(Agent::FromConfig(config));

        // Validate
        std::string validationError = utils::ValidateAgent(*agent);
        if (!validationError.empty()) {
            return core::Result<std::string>(core::Error(core::Error::Code::InvalidArgument, validationError));
        }

        // Create provider
        auto provider = AgentProviderRegistry::Instance().CreateProvider(agent->type);
        if (!provider) {
            return core::Err<std::string>(core::Error::Code::NotFound,
                                          "No provider registered for agent type");
        }

        // Initialize provider
        auto initResult = provider->Initialize(*agent);
        if (!initResult) {
            return core::Result<std::string>(initResult.GetError());
        }

        agent->provider = std::move(provider);
        SetupProviderCallbacks(*agent);

        std::string agentId = agent->id;
        m_agents.push_back(std::move(agent));

        if (m_onAgentCreated) {
            m_onAgentCreated(*m_agents.back());
        }

        SMITH_INFO(logging::Category::Agent, "Created agent from config: {}", agentId);
        return core::Result<std::string>(std::move(agentId));

    } catch (const std::exception& e) {
        SMITH_ERROR(logging::Category::Agent, "Failed to create agent from config: {}", e.what());
        return core::Result<std::string>(core::Error(core::Error::Code::ParseError, e.what()));
    }
}

core::Result<void> AgentTracker::StartAgent(const std::string& agentId) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    Agent* agent = FindAgent(agentId);
    if (!agent) {
        return core::Result<void>(core::Error(core::Error::Code::NotFound, "Agent not found"));
    }

    if (!agent->provider) {
        return core::Result<void>(core::Error(core::Error::Code::InvalidArgument, "Agent has no provider"));
    }

    SMITH_INFO(logging::Category::Agent, "Starting agent: {}", agent->name);

    auto result = agent->provider->Start();
    if (result) {
        agent->lastActivityTime = std::chrono::system_clock::now();
    }

    return result;
}

core::Result<void> AgentTracker::StopAgent(const std::string& agentId) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    Agent* agent = FindAgent(agentId);
    if (!agent) {
        return core::Result<void>(core::Error(core::Error::Code::NotFound, "Agent not found"));
    }

    if (!agent->provider) {
        return core::Result<void>(core::Error(core::Error::Code::InvalidArgument, "Agent has no provider"));
    }

    SMITH_INFO(logging::Category::Agent, "Stopping agent: {}", agent->name);

    return agent->provider->Stop();
}

core::Result<void> AgentTracker::RestartAgent(const std::string& agentId) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    Agent* agent = FindAgent(agentId);
    if (!agent) {
        return core::Result<void>(core::Error(core::Error::Code::NotFound, "Agent not found"));
    }

    if (!agent->provider) {
        return core::Result<void>(core::Error(core::Error::Code::InvalidArgument, "Agent has no provider"));
    }

    SMITH_INFO(logging::Category::Agent, "Restarting agent: {}", agent->name);

    agent->restartCount++;
    return agent->provider->Restart();
}

core::Result<void> AgentTracker::RemoveAgent(const std::string& agentId) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    auto it = std::find_if(m_agents.begin(), m_agents.end(),
                           [&agentId](const auto& agent) { return agent->id == agentId; });

    if (it == m_agents.end()) {
        return core::Result<void>(core::Error(core::Error::Code::NotFound, "Agent not found"));
    }

    Agent* agent = it->get();

    // Stop if running
    if (agent->provider && agent->provider->IsRunning()) {
        agent->provider->Stop();
    }

    SMITH_INFO(logging::Category::Agent, "Removing agent: {}", agent->name);

    // Notify before removal
    if (m_onAgentRemoved) {
        m_onAgentRemoved(*agent);
    }

    m_agents.erase(it);

    return core::Ok();
}

void AgentTracker::RemoveAllAgents() {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    SMITH_INFO(logging::Category::Agent, "Removing all agents (count: {})", m_agents.size());

    for (auto& agent : m_agents) {
        if (agent->provider && agent->provider->IsRunning()) {
            agent->provider->Stop();
        }

        if (m_onAgentRemoved) {
            m_onAgentRemoved(*agent);
        }
    }

    m_agents.clear();
}

// === Agent Access ===

Agent* AgentTracker::GetAgent(const std::string& agentId) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    return FindAgent(agentId);
}

const Agent* AgentTracker::GetAgent(const std::string& agentId) const {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    return FindAgent(agentId);
}

std::vector<Agent*> AgentTracker::GetAllAgents() {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    std::vector<Agent*> result;
    result.reserve(m_agents.size());
    for (auto& agent : m_agents) {
        result.push_back(agent.get());
    }
    return result;
}

std::vector<const Agent*> AgentTracker::GetAllAgents() const {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    std::vector<const Agent*> result;
    result.reserve(m_agents.size());
    for (const auto& agent : m_agents) {
        result.push_back(agent.get());
    }
    return result;
}

std::vector<Agent*> AgentTracker::GetAgentsByStatus(AgentStatus status) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    std::vector<Agent*> result;
    for (auto& agent : m_agents) {
        if (agent->status == status) {
            result.push_back(agent.get());
        }
    }
    return result;
}

std::vector<Agent*> AgentTracker::GetAgentsByType(AgentType type) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    std::vector<Agent*> result;
    for (auto& agent : m_agents) {
        if (agent->type == type) {
            result.push_back(agent.get());
        }
    }
    return result;
}

size_t AgentTracker::GetAgentCount() const {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    return m_agents.size();
}

// === Agent Communication ===

core::Result<void> AgentTracker::SendInput(const std::string& agentId, const std::string& input) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    Agent* agent = FindAgent(agentId);
    if (!agent) {
        return core::Result<void>(core::Error(core::Error::Code::NotFound, "Agent not found"));
    }

    if (!agent->provider) {
        return core::Result<void>(core::Error(core::Error::Code::InvalidArgument, "Agent has no provider"));
    }

    return agent->provider->SendInput(input);
}

core::Result<void> AgentTracker::SendCommand(const std::string& agentId, const std::string& command) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    Agent* agent = FindAgent(agentId);
    if (!agent) {
        return core::Result<void>(core::Error(core::Error::Code::NotFound, "Agent not found"));
    }

    if (!agent->provider) {
        return core::Result<void>(core::Error(core::Error::Code::InvalidArgument, "Agent has no provider"));
    }

    return agent->provider->SendCommand(command);
}

// === Configuration ===

core::Result<void> AgentTracker::LoadFromConfig(const std::string& configPath) {
    std::ifstream file(configPath);
    if (!file.is_open()) {
        return core::Result<void>(core::Error(core::Error::Code::IoError, "Failed to open config file"));
    }

    try {
        nlohmann::json config;
        file >> config;

        if (config.contains("agents") && config["agents"].is_array()) {
            for (const auto& agentConfig : config["agents"]) {
                auto result = CreateAgentFromConfig(agentConfig);
                if (!result) {
                    SMITH_WARN(logging::Category::Agent, "Failed to load agent: {}",
                               result.GetError().ToString());
                }
            }
        }

        SMITH_INFO(logging::Category::Agent, "Loaded {} agents from config", GetAgentCount());
        return core::Ok();

    } catch (const std::exception& e) {
        SMITH_ERROR(logging::Category::Agent, "Failed to parse config: {}", e.what());
        return core::Result<void>(core::Error(core::Error::Code::ParseError, e.what()));
    }
}

core::Result<void> AgentTracker::SaveToConfig(const std::string& configPath) {
    try {
        nlohmann::json config;
        config["agents"] = ExportConfigs();

        std::ofstream file(configPath);
        if (!file.is_open()) {
            return core::Result<void>(core::Error(core::Error::Code::IoError, "Failed to open config file for writing"));
        }

        file << config.dump(2);
        file.close();

        SMITH_INFO(logging::Category::Agent, "Saved {} agents to config", GetAgentCount());
        return core::Ok();

    } catch (const std::exception& e) {
        SMITH_ERROR(logging::Category::Agent, "Failed to save config: {}", e.what());
        return core::Result<void>(core::Error(core::Error::Code::IoError, e.what()));
    }
}

nlohmann::json AgentTracker::ExportConfigs() const {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    nlohmann::json configs = nlohmann::json::array();
    for (const auto& agent : m_agents) {
        configs.push_back(agent->ToConfig());
    }

    return configs;
}

// === Update ===

void AgentTracker::Update() {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    for (auto& agent : m_agents) {
        if (agent->provider) {
            agent->provider->Update();
            agent->UpdateUptime();
        }
    }
}

// === Internal Methods ===

Agent* AgentTracker::FindAgent(const std::string& agentId) {
    auto it = std::find_if(m_agents.begin(), m_agents.end(),
                           [&agentId](const auto& agent) { return agent->id == agentId; });
    return it != m_agents.end() ? it->get() : nullptr;
}

const Agent* AgentTracker::FindAgent(const std::string& agentId) const {
    auto it = std::find_if(m_agents.begin(), m_agents.end(),
                           [&agentId](const auto& agent) { return agent->id == agentId; });
    return it != m_agents.end() ? it->get() : nullptr;
}

void AgentTracker::SetupProviderCallbacks(Agent& agent) {
    if (!agent.provider) return;

    // Output callback
    agent.provider->SetOutputCallback([this, agentId = agent.id](const std::string& text, bool isError) {
        Agent* agent = FindAgent(agentId);
        if (agent) {
            agent->UpdateActivity();
            // Additional handling could go here
        }
    });

    // Status callback
    agent.provider->SetStatusCallback([this, agentId = agent.id](AgentStatus oldStatus, AgentStatus newStatus) {
        Agent* agent = FindAgent(agentId);
        if (agent) {
            agent->status = newStatus;
            agent->UpdateActivity();

            if (m_onAgentStatusChanged) {
                m_onAgentStatusChanged(*agent);
            }

            SMITH_DEBUG(logging::Category::Agent, "Agent {} status changed: {} -> {}",
                        agent->name, utils::AgentStatusToString(oldStatus),
                        utils::AgentStatusToString(newStatus));
        }
    });

    // Metrics callback
    agent.provider->SetMetricsCallback([this, agentId = agent.id](const AgentMetrics& metrics) {
        Agent* agent = FindAgent(agentId);
        if (agent) {
            agent->metrics = metrics;
            agent->UpdateActivity();
        }
    });
}

} // namespace smith::agent

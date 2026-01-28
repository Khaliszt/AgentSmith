// C:\FarfadetsCorp\AgentSmith\src\agent\agent_registry.cpp

#include "agent/agent_registry.h"
#include "agent/agent.h"
#include "agent/providers/claude_provider.h"
#include "logging/logger.h"
#include "logging/log_macros.h"

namespace smith::agent {

AgentProviderRegistry& AgentProviderRegistry::Instance() {
    static AgentProviderRegistry instance;
    return instance;
}

void AgentProviderRegistry::RegisterProvider(AgentType type, ProviderFactory factory) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_factories[type] = std::move(factory);

    SMITH_DEBUG(logging::Category::Agent, "Registered provider for agent type: {}",
                static_cast<int>(type));
}

std::unique_ptr<IAgentProvider> AgentProviderRegistry::CreateProvider(AgentType type) {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_factories.find(type);
    if (it == m_factories.end()) {
        SMITH_WARN(logging::Category::Agent, "No provider registered for agent type: {}",
                   static_cast<int>(type));
        return nullptr;
    }

    SMITH_DEBUG(logging::Category::Agent, "Creating provider for agent type: {}",
                static_cast<int>(type));

    return it->second();
}

IAgentProvider* AgentProviderRegistry::GetProvider(AgentType type) {
    std::lock_guard<std::mutex> lock(m_mutex);

    // Check if instance already exists
    auto instanceIt = m_instances.find(type);
    if (instanceIt != m_instances.end()) {
        return instanceIt->second.get();
    }

    // Check if factory exists
    auto factoryIt = m_factories.find(type);
    if (factoryIt == m_factories.end()) {
        SMITH_WARN(logging::Category::Agent, "No provider registered for agent type: {}",
                   static_cast<int>(type));
        return nullptr;
    }

    // Create new instance
    SMITH_DEBUG(logging::Category::Agent, "Creating singleton provider for agent type: {}",
                static_cast<int>(type));

    auto provider = factoryIt->second();
    auto* rawPtr = provider.get();
    m_instances[type] = std::move(provider);

    return rawPtr;
}

std::vector<AgentType> AgentProviderRegistry::GetRegisteredTypes() const {
    std::lock_guard<std::mutex> lock(m_mutex);

    std::vector<AgentType> types;
    types.reserve(m_factories.size());

    for (const auto& [type, _] : m_factories) {
        types.push_back(type);
    }

    return types;
}

bool AgentProviderRegistry::IsRegistered(AgentType type) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_factories.find(type) != m_factories.end();
}

void AgentProviderRegistry::RegisterBuiltins() {
    SMITH_INFO(logging::Category::Agent, "Registering built-in agent providers");

    // ClaudeCode provider
    RegisterProvider(AgentType::ClaudeCode, []() -> std::unique_ptr<IAgentProvider> {
        return std::make_unique<ClaudeAgentProvider>();
    });

    // Grok provider (will be implemented in Phase 4)
    // RegisterProvider(AgentType::Grok, []() -> std::unique_ptr<IAgentProvider> {
    //     return std::make_unique<GrokAgentProvider>();
    // });

    // ChatGPT provider (will be implemented in Phase 4)
    // RegisterProvider(AgentType::ChatGPT, []() -> std::unique_ptr<IAgentProvider> {
    //     return std::make_unique<ChatGPTAgentProvider>();
    // });

    SMITH_INFO(logging::Category::Agent, "Registered {} built-in providers",
               GetRegisteredTypes().size());
}

void AgentProviderRegistry::ClearInstances() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_instances.clear();
    SMITH_DEBUG(logging::Category::Agent, "Cleared all provider instances");
}

} // namespace smith::agent

// C:\FarfadetsCorp\AgentSmith\include\agent\agent_registry.h

#pragma once

#include "agent_provider.h"
#include <memory>
#include <unordered_map>
#include <functional>
#include <vector>
#include <mutex>

namespace smith::agent {

/**
 * @brief Registry for agent provider factories
 *
 * This singleton manages provider factories and instances for different
 * agent types. It allows registering custom providers and provides
 * built-in providers for ClaudeCode, Grok, and ChatGPT.
 */
class AgentProviderRegistry {
public:
    /**
     * @brief Factory function type for creating providers
     */
    using ProviderFactory = std::function<std::unique_ptr<IAgentProvider>()>;

    /**
     * @brief Gets the singleton instance
     */
    static AgentProviderRegistry& Instance();

    /**
     * @brief Registers a provider factory for an agent type
     *
     * @param type Agent type
     * @param factory Factory function
     */
    void RegisterProvider(AgentType type, ProviderFactory factory);

    /**
     * @brief Creates a new provider instance for an agent type
     *
     * @param type Agent type
     * @return Unique pointer to provider, or nullptr if not registered
     */
    std::unique_ptr<IAgentProvider> CreateProvider(AgentType type);

    /**
     * @brief Gets or creates a singleton provider instance for an agent type
     *
     * Some providers (especially API-based) can be reused across multiple
     * agents. This method returns a shared instance.
     *
     * @param type Agent type
     * @return Raw pointer to provider (owned by registry), or nullptr if not registered
     */
    IAgentProvider* GetProvider(AgentType type);

    /**
     * @brief Gets list of all registered agent types
     *
     * @return Vector of registered types
     */
    std::vector<AgentType> GetRegisteredTypes() const;

    /**
     * @brief Checks if a provider is registered for a type
     *
     * @param type Agent type
     * @return True if registered
     */
    bool IsRegistered(AgentType type) const;

    /**
     * @brief Registers all built-in providers
     *
     * Registers: ClaudeCode, Grok, ChatGPT
     * Should be called during application initialization.
     */
    void RegisterBuiltins();

    /**
     * @brief Clears all provider instances (but keeps factories)
     *
     * Useful for cleanup or testing.
     */
    void ClearInstances();

private:
    AgentProviderRegistry() = default;
    ~AgentProviderRegistry() = default;
    AgentProviderRegistry(const AgentProviderRegistry&) = delete;
    AgentProviderRegistry& operator=(const AgentProviderRegistry&) = delete;

    mutable std::mutex m_mutex;
    std::unordered_map<AgentType, ProviderFactory> m_factories;
    std::unordered_map<AgentType, std::unique_ptr<IAgentProvider>> m_instances;
};

} // namespace smith::agent

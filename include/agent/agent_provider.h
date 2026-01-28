// C:\FarfadetsCorp\AgentSmith\include\agent\agent_provider.h

#pragma once

#include "agent.h"
#include "agent_metrics.h"
#include "core/result.h"
#include <string>
#include <functional>
#include <memory>

namespace smith::agent {

/**
 * @brief Callback for receiving agent output/responses
 *
 * @param text The output text from the agent
 * @param isError True if this is error output
 */
using OutputCallback = std::function<void(const std::string& text, bool isError)>;

/**
 * @brief Callback for status changes
 *
 * @param oldStatus Previous status
 * @param newStatus New status
 */
using StatusCallback = std::function<void(AgentStatus oldStatus, AgentStatus newStatus)>;

/**
 * @brief Callback for metrics updates
 *
 * @param metrics Updated metrics
 */
using MetricsCallback = std::function<void(const AgentMetrics& metrics)>;

/**
 * @brief Abstract interface for agent providers
 *
 * This interface defines the contract for all agent implementations,
 * whether they are terminal-based (Claude Code, Cursor) or API-based
 * (Grok, ChatGPT).
 *
 * Terminal-based providers manage a ConPTY process and parse output.
 * API-based providers make HTTP requests and handle responses.
 */
class IAgentProvider {
public:
    virtual ~IAgentProvider() = default;

    // === Lifecycle ===

    /**
     * @brief Initializes the provider with agent configuration
     *
     * @param agent Agent configuration
     * @return Result indicating success or error
     */
    virtual core::Result<void> Initialize(const Agent& agent) = 0;

    /**
     * @brief Starts the agent (launches process or establishes connection)
     *
     * @return Result indicating success or error
     */
    virtual core::Result<void> Start() = 0;

    /**
     * @brief Stops the agent gracefully
     *
     * @return Result indicating success or error
     */
    virtual core::Result<void> Stop() = 0;

    /**
     * @brief Restarts the agent (stop + start)
     *
     * @return Result indicating success or error
     */
    virtual core::Result<void> Restart() {
        auto stopResult = Stop();
        if (!stopResult) {
            return stopResult;
        }
        return Start();
    }

    // === Communication ===

    /**
     * @brief Sends input to the agent
     *
     * For terminal agents: writes to stdin
     * For API agents: sends as user message
     *
     * @param input Input text
     * @return Result indicating success or error
     */
    virtual core::Result<void> SendInput(const std::string& input) = 0;

    /**
     * @brief Sends a special command to the agent (e.g., /cost for Claude)
     *
     * @param command Command string
     * @return Result indicating success or error
     */
    virtual core::Result<void> SendCommand(const std::string& command) {
        // Default: treat as regular input
        return SendInput(command);
    }

    // === Status & Metrics ===

    /**
     * @brief Gets current agent status
     *
     * @return Current status
     */
    virtual AgentStatus GetStatus() const = 0;

    /**
     * @brief Checks if agent is running
     *
     * @return True if running
     */
    virtual bool IsRunning() const {
        AgentStatus status = GetStatus();
        return status == AgentStatus::Running ||
               status == AgentStatus::Waiting ||
               status == AgentStatus::Thinking;
    }

    /**
     * @brief Gets current metrics
     *
     * @return Current metrics
     */
    virtual AgentMetrics GetMetrics() const = 0;

    /**
     * @brief Updates metrics (e.g., by parsing output or API response)
     *
     * This is called periodically or when new data is available.
     *
     * @return Result with updated metrics or error
     */
    virtual core::Result<AgentMetrics> UpdateMetrics() = 0;

    // === Callbacks ===

    /**
     * @brief Sets callback for output
     *
     * @param callback Callback function
     */
    virtual void SetOutputCallback(OutputCallback callback) {
        m_outputCallback = std::move(callback);
    }

    /**
     * @brief Sets callback for status changes
     *
     * @param callback Callback function
     */
    virtual void SetStatusCallback(StatusCallback callback) {
        m_statusCallback = std::move(callback);
    }

    /**
     * @brief Sets callback for metrics updates
     *
     * @param callback Callback function
     */
    virtual void SetMetricsCallback(MetricsCallback callback) {
        m_metricsCallback = std::move(callback);
    }

    // === Provider Information ===

    /**
     * @brief Gets provider name (e.g., "ClaudeCodeProvider", "GrokProvider")
     *
     * @return Provider name
     */
    virtual const char* GetProviderName() const = 0;

    /**
     * @brief Gets supported agent type
     *
     * @return Agent type
     */
    virtual AgentType GetAgentType() const = 0;

    /**
     * @brief Checks if this provider requires a terminal
     *
     * @return True if terminal required
     */
    virtual bool RequiresTerminal() const = 0;

    /**
     * @brief Checks if this provider uses API
     *
     * @return True if API-based
     */
    virtual bool UsesAPI() const {
        return !RequiresTerminal();
    }

    // === Update/Poll ===

    /**
     * @brief Updates provider state (polls for output, checks status, etc.)
     *
     * Called from main loop. Providers should check for new data and
     * invoke callbacks as needed.
     */
    virtual void Update() = 0;

protected:
    // Callback storage
    OutputCallback m_outputCallback;
    StatusCallback m_statusCallback;
    MetricsCallback m_metricsCallback;

    /**
     * @brief Helper to invoke output callback safely
     */
    void InvokeOutputCallback(const std::string& text, bool isError = false) {
        if (m_outputCallback) {
            m_outputCallback(text, isError);
        }
    }

    /**
     * @brief Helper to invoke status callback safely
     */
    void InvokeStatusCallback(AgentStatus oldStatus, AgentStatus newStatus) {
        if (m_statusCallback) {
            m_statusCallback(oldStatus, newStatus);
        }
    }

    /**
     * @brief Helper to invoke metrics callback safely
     */
    void InvokeMetricsCallback(const AgentMetrics& metrics) {
        if (m_metricsCallback) {
            m_metricsCallback(metrics);
        }
    }
};

} // namespace smith::agent

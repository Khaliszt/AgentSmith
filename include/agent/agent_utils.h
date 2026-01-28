// C:\FarfadetsCorp\AgentSmith\include\agent\agent_utils.h

#pragma once

#include "agent.h"
#include "agent_metrics.h"
#include <string>
#include <optional>

namespace smith::agent {

/**
 * @brief Utility functions for agent management
 */
namespace utils {

/**
 * @brief Generates a unique agent ID
 *
 * Uses timestamp-based approach with hex encoding.
 *
 * @return Unique ID string
 */
std::string GenerateAgentId();

/**
 * @brief Converts AgentType to string
 *
 * @param type Agent type
 * @return String representation (e.g., "ClaudeCode", "Grok")
 */
std::string AgentTypeToString(AgentType type);

/**
 * @brief Converts string to AgentType
 *
 * @param str String representation
 * @return Agent type, or nullopt if invalid
 */
std::optional<AgentType> StringToAgentType(const std::string& str);

/**
 * @brief Converts AgentStatus to string
 *
 * @param status Agent status
 * @return String representation (e.g., "Running", "Thinking")
 */
std::string AgentStatusToString(AgentStatus status);

/**
 * @brief Converts string to AgentStatus
 *
 * @param str String representation
 * @return Agent status, or nullopt if invalid
 */
std::optional<AgentStatus> StringToAgentStatus(const std::string& str);

/**
 * @brief Formats duration in human-readable form
 *
 * Examples: "5s", "2m 30s", "1h 15m"
 *
 * @param seconds Duration in seconds
 * @return Formatted string
 */
std::string FormatDuration(std::chrono::seconds seconds);

/**
 * @brief Formats cost with appropriate precision
 *
 * Examples: "$0.0012", "$1.23", "$123.45"
 *
 * @param cost Cost in USD
 * @return Formatted string
 */
std::string FormatCost(double cost);

/**
 * @brief Formats token count with thousands separators
 *
 * Examples: "1,234", "12,345,678"
 *
 * @param tokens Token count
 * @return Formatted string
 */
std::string FormatTokens(uint64_t tokens);

/**
 * @brief Gets API key from environment variable
 *
 * Checks environment for keys like CLAUDE_API_KEY, OPENAI_API_KEY, etc.
 *
 * @param type Agent type
 * @return API key string, or empty if not found
 */
std::string GetApiKeyForType(AgentType type);

/**
 * @brief Gets default API endpoint for agent type
 *
 * @param type Agent type
 * @return API endpoint URL, or empty if not API-based
 */
std::string GetDefaultApiEndpoint(AgentType type);

/**
 * @brief Gets default model ID for agent type
 *
 * @param type Agent type
 * @return Model ID, or empty if not API-based
 */
std::string GetDefaultModelId(AgentType type);

/**
 * @brief Validates agent configuration
 *
 * Checks if required fields are set for the agent type.
 *
 * @param agent Agent to validate
 * @return Empty string if valid, error message otherwise
 */
std::string ValidateAgent(const Agent& agent);

/**
 * @brief Gets a human-readable summary of metrics
 *
 * Example: "1.2K tokens, $0.05, 3 tool calls"
 *
 * @param metrics Metrics to summarize
 * @return Summary string
 */
std::string GetMetricsSummary(const AgentMetrics& metrics);

} // namespace utils
} // namespace smith::agent

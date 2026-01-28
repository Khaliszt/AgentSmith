// C:\FarfadetsCorp\AgentSmith\include\agent\agent_metrics.h

#pragma once

#include <string>
#include <chrono>
#include <cstdint>

namespace smith::agent {

/**
 * @brief Tracks token usage and costs for an agent
 *
 * Supports both terminal-based agents (Claude Code - parsed from /cost output)
 * and API-based agents (Grok, ChatGPT - extracted from API responses).
 */
struct AgentMetrics {
    // === Token Counts ===
    uint64_t inputTokens = 0;           // Total input/prompt tokens
    uint64_t outputTokens = 0;          // Total output/completion tokens
    uint64_t cacheCreationTokens = 0;   // Tokens written to cache (Claude)
    uint64_t cacheReadTokens = 0;       // Tokens read from cache (Claude)

    // === Cost Tracking ===
    double sessionCost = 0.0;           // Total session cost in USD
    double estimatedCost = 0.0;         // Estimated cost (if not from provider)

    // === Model Information ===
    std::string modelName;              // e.g., "claude-sonnet-4.5", "grok-2", "gpt-4-turbo"

    // === Activity Metrics ===
    int toolCalls = 0;                  // Number of tool/function calls
    int apiCalls = 0;                   // Number of API requests made

    // === Timing ===
    std::chrono::system_clock::time_point lastUpdateTime;
    std::chrono::seconds totalThinkingTime{0};  // Time spent "thinking"

    /**
     * @brief Calculates total tokens (input + output)
     */
    uint64_t GetTotalTokens() const {
        return inputTokens + outputTokens;
    }

    /**
     * @brief Calculates cache tokens (creation + read)
     */
    uint64_t GetCacheTokens() const {
        return cacheCreationTokens + cacheReadTokens;
    }

    /**
     * @brief Gets the effective cost (session if available, otherwise estimated)
     */
    double GetEffectiveCost() const {
        return sessionCost > 0.0 ? sessionCost : estimatedCost;
    }

    /**
     * @brief Resets all metrics to zero
     */
    void Reset() {
        inputTokens = 0;
        outputTokens = 0;
        cacheCreationTokens = 0;
        cacheReadTokens = 0;
        sessionCost = 0.0;
        estimatedCost = 0.0;
        modelName.clear();
        toolCalls = 0;
        apiCalls = 0;
        totalThinkingTime = std::chrono::seconds{0};
    }

    /**
     * @brief Adds another metrics object to this one
     */
    AgentMetrics& operator+=(const AgentMetrics& other) {
        inputTokens += other.inputTokens;
        outputTokens += other.outputTokens;
        cacheCreationTokens += other.cacheCreationTokens;
        cacheReadTokens += other.cacheReadTokens;
        sessionCost += other.sessionCost;
        estimatedCost += other.estimatedCost;
        toolCalls += other.toolCalls;
        apiCalls += other.apiCalls;
        totalThinkingTime += other.totalThinkingTime;
        lastUpdateTime = std::max(lastUpdateTime, other.lastUpdateTime);
        return *this;
    }
};

} // namespace smith::agent

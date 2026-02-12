#pragma once

#include <chrono>
#include <map>
#include <mutex>
#include <string>
#include <vector>

namespace smith::network {

/**
 * @brief Thread-safe rate limiter using sliding window algorithm
 *
 * Tracks API request rates and token consumption to prevent
 * exceeding rate limits. Supports dynamic limit updates from
 * API response headers (OpenAI standard).
 */
class RateLimiter {
public:
    /**
     * @brief Construct a rate limiter with specified limits
     * @param requestsPerMinute Maximum requests allowed per minute
     * @param tokensPerMinute Maximum tokens allowed per minute
     */
    RateLimiter(int requestsPerMinute = 60, int tokensPerMinute = 100000);

    /**
     * @brief Check if a new request can be made without exceeding limits
     * @return true if request is allowed, false otherwise
     */
    bool CanMakeRequest() const;

    /**
     * @brief Check if specified token count can be used without exceeding limits
     * @param tokenCount Number of tokens to check
     * @return true if tokens are available, false otherwise
     */
    bool CanUseTokens(int tokenCount) const;

    /**
     * @brief Record a request and its token usage
     * @param tokenCount Number of tokens used (0 if not applicable)
     */
    void RecordRequest(int tokenCount = 0);

    /**
     * @brief Calculate time until next request is available
     * @return Duration until a request slot becomes available
     */
    std::chrono::seconds TimeUntilAvailable() const;

    /**
     * @brief Update rate limits from API response headers
     *
     * Parses OpenAI standard rate limit headers:
     * - x-ratelimit-limit-requests
     * - x-ratelimit-remaining-requests
     * - x-ratelimit-limit-tokens
     * - x-ratelimit-remaining-tokens
     * - x-ratelimit-reset-requests
     *
     * @param headers Map of HTTP response headers
     */
    void UpdateFromHeaders(const std::map<std::string, std::string>& headers);

    /**
     * @brief Reset all counters and recorded requests
     */
    void Reset();

    /**
     * @brief Get current request count in the window
     * @return Number of requests in the last minute
     */
    int GetCurrentRequestCount() const;

    /**
     * @brief Get current token count in the window
     * @return Number of tokens used in the last minute
     */
    int GetCurrentTokenCount() const;

    /**
     * @brief Get maximum requests per minute limit
     * @return Configured request limit
     */
    int GetRequestLimit() const { return m_requestsPerMinute; }

    /**
     * @brief Get maximum tokens per minute limit
     * @return Configured token limit
     */
    int GetTokenLimit() const { return m_tokensPerMinute; }

private:
    /**
     * @brief Structure to record a single request
     */
    struct RequestRecord {
        std::chrono::steady_clock::time_point timestamp;
        int tokenCount;
    };

    /**
     * @brief Remove requests older than 1 minute from the sliding window
     */
    void PruneOldEntries() const;

    /**
     * @brief Calculate time until oldest request expires
     * @return Duration until a request slot becomes available, or 0 if available now
     */
    std::chrono::seconds CalculateWaitTime() const;

    mutable std::mutex m_mutex;
    int m_requestsPerMinute;
    int m_tokensPerMinute;

    // Sliding window: stores timestamps and token counts of recent requests
    mutable std::vector<RequestRecord> m_requestHistory;

    // Optional: track remaining counts from API headers for more accurate limiting
    mutable int m_remainingRequests;
    mutable int m_remainingTokens;
    mutable std::chrono::steady_clock::time_point m_resetTime;
};

} // namespace smith::network

#include "network/rate_limiter.h"
#include "logging/log_macros.h"
#include "logging/logger.h"
#include <algorithm>
#include <sstream>

namespace smith::network {

namespace {
    // Duration for the sliding window
    constexpr auto WINDOW_DURATION = std::chrono::minutes(1);

    /**
     * @brief Parse integer from string, return default on error
     */
    int ParseInt(const std::string& str, int defaultValue = 0) {
        try {
            return std::stoi(str);
        } catch (...) {
            return defaultValue;
        }
    }

    /**
     * @brief Parse timestamp from string (seconds since epoch), return default on error
     */
    std::chrono::steady_clock::time_point ParseTimestamp(const std::string& str) {
        try {
            auto seconds = std::stoll(str);
            // Convert to steady_clock time point relative to now
            auto now = std::chrono::steady_clock::now();
            auto systemNow = std::chrono::system_clock::now();
            auto systemTarget = std::chrono::system_clock::time_point(std::chrono::seconds(seconds));
            auto duration = systemTarget - systemNow;
            return now + std::chrono::duration_cast<std::chrono::steady_clock::duration>(duration);
        } catch (...) {
            return std::chrono::steady_clock::now();
        }
    }

    /**
     * @brief Case-insensitive header lookup
     */
    std::string FindHeader(const std::map<std::string, std::string>& headers, const std::string& key) {
        // Try exact match first
        auto it = headers.find(key);
        if (it != headers.end()) {
            return it->second;
        }

        // Try case-insensitive search
        std::string lowerKey = key;
        std::transform(lowerKey.begin(), lowerKey.end(), lowerKey.begin(), ::tolower);

        for (const auto& [headerKey, headerValue] : headers) {
            std::string lowerHeaderKey = headerKey;
            std::transform(lowerHeaderKey.begin(), lowerHeaderKey.end(),
                         lowerHeaderKey.begin(), ::tolower);
            if (lowerHeaderKey == lowerKey) {
                return headerValue;
            }
        }

        return "";
    }
}

RateLimiter::RateLimiter(int requestsPerMinute, int tokensPerMinute)
    : m_requestsPerMinute(requestsPerMinute)
    , m_tokensPerMinute(tokensPerMinute)
    , m_remainingRequests(requestsPerMinute)
    , m_remainingTokens(tokensPerMinute)
    , m_resetTime(std::chrono::steady_clock::now() + WINDOW_DURATION)
{
    m_requestHistory.reserve(requestsPerMinute); // Pre-allocate for efficiency
    SMITH_LOG(Info, smith::logging::Category::Network,
              "RateLimiter initialized: {} requests/min, {} tokens/min",
              requestsPerMinute, tokensPerMinute);
}

bool RateLimiter::CanMakeRequest() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    PruneOldEntries();

    int currentRequests = static_cast<int>(m_requestHistory.size());
    bool allowed = currentRequests < m_requestsPerMinute;

    if (!allowed) {
        SMITH_LOG(Debug, smith::logging::Category::Network,
                  "Rate limit reached: {}/{} requests in window",
                  currentRequests, m_requestsPerMinute);
    }

    return allowed;
}

bool RateLimiter::CanUseTokens(int tokenCount) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    PruneOldEntries();

    // Calculate current token usage in the window
    int currentTokens = 0;
    for (const auto& record : m_requestHistory) {
        currentTokens += record.tokenCount;
    }

    bool allowed = (currentTokens + tokenCount) <= m_tokensPerMinute;

    if (!allowed) {
        SMITH_LOG(Debug, smith::logging::Category::Network,
                  "Token limit would be exceeded: {} + {} > {}",
                  currentTokens, tokenCount, m_tokensPerMinute);
    }

    return allowed;
}

void RateLimiter::RecordRequest(int tokenCount) {
    std::lock_guard<std::mutex> lock(m_mutex);
    PruneOldEntries();

    RequestRecord record;
    record.timestamp = std::chrono::steady_clock::now();
    record.tokenCount = tokenCount;

    m_requestHistory.push_back(record);

    // Update remaining counts
    if (m_remainingRequests > 0) {
        m_remainingRequests--;
    }
    if (tokenCount > 0 && m_remainingTokens > tokenCount) {
        m_remainingTokens -= tokenCount;
    }

    SMITH_LOG(Debug, smith::logging::Category::Network,
              "Request recorded: {} tokens, {}/{} requests in window",
              tokenCount, m_requestHistory.size(), m_requestsPerMinute);
}

std::chrono::seconds RateLimiter::TimeUntilAvailable() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return CalculateWaitTime();
}

void RateLimiter::UpdateFromHeaders(const std::map<std::string, std::string>& headers) {
    std::lock_guard<std::mutex> lock(m_mutex);

    // Parse request limits
    std::string limitRequests = FindHeader(headers, "x-ratelimit-limit-requests");
    if (!limitRequests.empty()) {
        int newLimit = ParseInt(limitRequests, m_requestsPerMinute);
        if (newLimit > 0 && newLimit != m_requestsPerMinute) {
            SMITH_LOG(Info, smith::logging::Category::Network,
                      "Updating request limit: {} -> {}",
                      m_requestsPerMinute, newLimit);
            m_requestsPerMinute = newLimit;
        }
    }

    // Parse remaining requests
    std::string remainingRequests = FindHeader(headers, "x-ratelimit-remaining-requests");
    if (!remainingRequests.empty()) {
        m_remainingRequests = ParseInt(remainingRequests, m_remainingRequests);
        SMITH_LOG(Debug, smith::logging::Category::Network,
                  "Remaining requests from header: {}", m_remainingRequests);
    }

    // Parse token limits
    std::string limitTokens = FindHeader(headers, "x-ratelimit-limit-tokens");
    if (!limitTokens.empty()) {
        int newLimit = ParseInt(limitTokens, m_tokensPerMinute);
        if (newLimit > 0 && newLimit != m_tokensPerMinute) {
            SMITH_LOG(Info, smith::logging::Category::Network,
                      "Updating token limit: {} -> {}",
                      m_tokensPerMinute, newLimit);
            m_tokensPerMinute = newLimit;
        }
    }

    // Parse remaining tokens
    std::string remainingTokens = FindHeader(headers, "x-ratelimit-remaining-tokens");
    if (!remainingTokens.empty()) {
        m_remainingTokens = ParseInt(remainingTokens, m_remainingTokens);
        SMITH_LOG(Debug, smith::logging::Category::Network,
                  "Remaining tokens from header: {}", m_remainingTokens);
    }

    // Parse reset time
    std::string resetRequests = FindHeader(headers, "x-ratelimit-reset-requests");
    if (!resetRequests.empty()) {
        m_resetTime = ParseTimestamp(resetRequests);
        auto waitTime = std::chrono::duration_cast<std::chrono::seconds>(
            m_resetTime - std::chrono::steady_clock::now()
        );
        SMITH_LOG(Debug, smith::logging::Category::Network,
                  "Rate limit reset in {} seconds", waitTime.count());
    }
}

void RateLimiter::Reset() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_requestHistory.clear();
    m_remainingRequests = m_requestsPerMinute;
    m_remainingTokens = m_tokensPerMinute;
    m_resetTime = std::chrono::steady_clock::now() + WINDOW_DURATION;
    SMITH_LOG(Info, smith::logging::Category::Network, "RateLimiter reset");
}

int RateLimiter::GetCurrentRequestCount() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    PruneOldEntries();
    return static_cast<int>(m_requestHistory.size());
}

int RateLimiter::GetCurrentTokenCount() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    PruneOldEntries();

    int total = 0;
    for (const auto& record : m_requestHistory) {
        total += record.tokenCount;
    }
    return total;
}

void RateLimiter::PruneOldEntries() const {
    auto now = std::chrono::steady_clock::now();
    auto cutoff = now - WINDOW_DURATION;

    // Remove entries older than the window
    auto it = std::remove_if(m_requestHistory.begin(), m_requestHistory.end(),
        [cutoff](const RequestRecord& record) {
            return record.timestamp < cutoff;
        });

    if (it != m_requestHistory.end()) {
        size_t removed = std::distance(it, m_requestHistory.end());
        m_requestHistory.erase(it, m_requestHistory.end());

        if (removed > 0) {
            SMITH_LOG(Verbose, smith::logging::Category::Network,
                      "Pruned {} old request records", removed);
        }
    }
}

std::chrono::seconds RateLimiter::CalculateWaitTime() const {
    PruneOldEntries();

    // If we're under the limit, no wait needed
    if (m_requestHistory.size() < static_cast<size_t>(m_requestsPerMinute)) {
        return std::chrono::seconds(0);
    }

    // Find the oldest request in the window
    auto now = std::chrono::steady_clock::now();
    auto oldestTimestamp = now;

    for (const auto& record : m_requestHistory) {
        if (record.timestamp < oldestTimestamp) {
            oldestTimestamp = record.timestamp;
        }
    }

    // Calculate when the oldest request will expire
    auto expiryTime = oldestTimestamp + WINDOW_DURATION;

    if (expiryTime <= now) {
        return std::chrono::seconds(0);
    }

    auto waitDuration = std::chrono::duration_cast<std::chrono::seconds>(expiryTime - now);
    return waitDuration + std::chrono::seconds(1); // Add 1 second buffer
}

} // namespace smith::network

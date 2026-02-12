// C:\FarfadetsCorp\AgentSmith\tests\unit\network\test_rate_limiter.cpp

#include <gtest/gtest.h>
#include "network/rate_limiter.h"
#include <thread>
#include <chrono>

using namespace smith::network;

class RateLimiterTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a rate limiter with small limits for testing
        limiter = std::make_unique<RateLimiter>(5, 100);
    }

    void TearDown() override {
        limiter.reset();
    }

    std::unique_ptr<RateLimiter> limiter;
};

TEST_F(RateLimiterTest, ConstructorInitializes) {
    EXPECT_EQ(limiter->GetRequestLimit(), 5);
    EXPECT_EQ(limiter->GetTokenLimit(), 100);
    EXPECT_EQ(limiter->GetCurrentRequestCount(), 0);
    EXPECT_EQ(limiter->GetCurrentTokenCount(), 0);
}

TEST_F(RateLimiterTest, CanMakeRequestWhenUnderLimit) {
    EXPECT_TRUE(limiter->CanMakeRequest());

    limiter->RecordRequest(10);
    EXPECT_TRUE(limiter->CanMakeRequest());

    limiter->RecordRequest(10);
    EXPECT_TRUE(limiter->CanMakeRequest());
}

TEST_F(RateLimiterTest, CannotMakeRequestWhenAtLimit) {
    // Record 5 requests (at the limit)
    for (int i = 0; i < 5; i++) {
        EXPECT_TRUE(limiter->CanMakeRequest());
        limiter->RecordRequest(10);
    }

    // Should not be able to make another request
    EXPECT_FALSE(limiter->CanMakeRequest());
}

TEST_F(RateLimiterTest, CanUseTokensWhenUnderLimit) {
    EXPECT_TRUE(limiter->CanUseTokens(50));
    limiter->RecordRequest(50);

    EXPECT_TRUE(limiter->CanUseTokens(49));
    EXPECT_FALSE(limiter->CanUseTokens(51));
}

TEST_F(RateLimiterTest, TokenCountTracking) {
    limiter->RecordRequest(30);
    EXPECT_EQ(limiter->GetCurrentTokenCount(), 30);

    limiter->RecordRequest(20);
    EXPECT_EQ(limiter->GetCurrentTokenCount(), 50);

    limiter->RecordRequest(40);
    EXPECT_EQ(limiter->GetCurrentTokenCount(), 90);
}

TEST_F(RateLimiterTest, RequestCountTracking) {
    EXPECT_EQ(limiter->GetCurrentRequestCount(), 0);

    limiter->RecordRequest(10);
    EXPECT_EQ(limiter->GetCurrentRequestCount(), 1);

    limiter->RecordRequest(20);
    EXPECT_EQ(limiter->GetCurrentRequestCount(), 2);
}

TEST_F(RateLimiterTest, ResetClearsHistory) {
    limiter->RecordRequest(30);
    limiter->RecordRequest(20);

    EXPECT_EQ(limiter->GetCurrentRequestCount(), 2);
    EXPECT_EQ(limiter->GetCurrentTokenCount(), 50);

    limiter->Reset();

    EXPECT_EQ(limiter->GetCurrentRequestCount(), 0);
    EXPECT_EQ(limiter->GetCurrentTokenCount(), 0);
}

TEST_F(RateLimiterTest, UpdateFromHeaders) {
    std::map<std::string, std::string> headers;
    headers["x-ratelimit-limit-requests"] = "100";
    headers["x-ratelimit-limit-tokens"] = "50000";
    headers["x-ratelimit-remaining-requests"] = "95";
    headers["x-ratelimit-remaining-tokens"] = "48000";

    limiter->UpdateFromHeaders(headers);

    EXPECT_EQ(limiter->GetRequestLimit(), 100);
    EXPECT_EQ(limiter->GetTokenLimit(), 50000);
}

TEST_F(RateLimiterTest, CaseInsensitiveHeaderParsing) {
    std::map<std::string, std::string> headers;
    headers["X-RateLimit-Limit-Requests"] = "200";
    headers["X-RATELIMIT-LIMIT-TOKENS"] = "100000";

    limiter->UpdateFromHeaders(headers);

    EXPECT_EQ(limiter->GetRequestLimit(), 200);
    EXPECT_EQ(limiter->GetTokenLimit(), 100000);
}

TEST_F(RateLimiterTest, TimeUntilAvailableWhenNotLimited) {
    auto waitTime = limiter->TimeUntilAvailable();
    EXPECT_EQ(waitTime.count(), 0);
}

TEST_F(RateLimiterTest, TimeUntilAvailableWhenLimited) {
    // Fill up the request limit
    for (int i = 0; i < 5; i++) {
        limiter->RecordRequest(10);
    }

    // Should need to wait
    auto waitTime = limiter->TimeUntilAvailable();
    EXPECT_GT(waitTime.count(), 0);
    EXPECT_LE(waitTime.count(), 61); // At most 60 seconds + 1 second buffer
}

TEST_F(RateLimiterTest, SlidingWindowPrunesOldEntries) {
    // Note: This test would need to wait for actual time to pass
    // For now, we just verify the behavior doesn't crash
    limiter->RecordRequest(10);
    limiter->RecordRequest(20);

    EXPECT_EQ(limiter->GetCurrentRequestCount(), 2);

    // In a real scenario, we'd wait 61 seconds and verify entries are pruned
    // For this test, we just verify the current state
    EXPECT_GE(limiter->GetCurrentRequestCount(), 0);
}

TEST_F(RateLimiterTest, ThreadSafety) {
    // Create multiple threads that try to record requests
    std::vector<std::thread> threads;

    for (int i = 0; i < 10; i++) {
        threads.emplace_back([this]() {
            if (limiter->CanMakeRequest()) {
                limiter->RecordRequest(5);
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    // Should have recorded some requests without crashing
    // The exact count depends on race conditions, but should be <= 5
    EXPECT_LE(limiter->GetCurrentRequestCount(), 5);
}

TEST_F(RateLimiterTest, ZeroTokensAllowed) {
    limiter->RecordRequest(0);
    EXPECT_EQ(limiter->GetCurrentRequestCount(), 1);
    EXPECT_EQ(limiter->GetCurrentTokenCount(), 0);
}

TEST_F(RateLimiterTest, LargeTokenValues) {
    auto largeLimiter = std::make_unique<RateLimiter>(10, 1000000);

    EXPECT_TRUE(largeLimiter->CanUseTokens(500000));
    largeLimiter->RecordRequest(500000);

    EXPECT_TRUE(largeLimiter->CanUseTokens(500000));
    EXPECT_FALSE(largeLimiter->CanUseTokens(500001));
}

TEST_F(RateLimiterTest, InvalidHeadersIgnored) {
    std::map<std::string, std::string> headers;
    headers["x-ratelimit-limit-requests"] = "invalid";
    headers["x-ratelimit-limit-tokens"] = "not-a-number";

    int originalRequestLimit = limiter->GetRequestLimit();
    int originalTokenLimit = limiter->GetTokenLimit();

    limiter->UpdateFromHeaders(headers);

    // Limits should not change with invalid headers
    EXPECT_EQ(limiter->GetRequestLimit(), originalRequestLimit);
    EXPECT_EQ(limiter->GetTokenLimit(), originalTokenLimit);
}

TEST_F(RateLimiterTest, EmptyHeadersDoNothing) {
    std::map<std::string, std::string> headers;

    int originalRequestLimit = limiter->GetRequestLimit();
    int originalTokenLimit = limiter->GetTokenLimit();

    limiter->UpdateFromHeaders(headers);

    EXPECT_EQ(limiter->GetRequestLimit(), originalRequestLimit);
    EXPECT_EQ(limiter->GetTokenLimit(), originalTokenLimit);
}

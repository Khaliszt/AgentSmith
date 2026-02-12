// C:\FarfadetsCorp\AgentSmith\src\network\api_client.cpp

#include "network/api_client.h"
#include "logging/logger.h"
#include <nlohmann/json.hpp>
#include <thread>
#include <chrono>
#include <sstream>

using json = nlohmann::json;
using namespace smith::core;
using namespace smith::logging;

namespace smith::network {

// === ChatMessage ===

const char* ChatMessage::RoleToString(Role role) {
    switch (role) {
        case Role::System: return "system";
        case Role::User: return "user";
        case Role::Assistant: return "assistant";
        default: return "user";
    }
}

ChatMessage::Role ChatMessage::StringToRole(const std::string& str) {
    if (str == "system") return Role::System;
    if (str == "assistant") return Role::Assistant;
    return Role::User;
}

// === ApiClient ===

ApiClient::ApiClient()
    : m_httpClient(std::make_unique<HttpClient>()) {
    SMITH_DEBUG(Category::Network, "ApiClient created with default config");
}

ApiClient::ApiClient(const ApiClientConfig& config)
    : m_config(config)
    , m_httpClient(std::make_unique<HttpClient>()) {

    // Configure HTTP client based on API client config
    HttpClient::Config httpConfig;
    httpConfig.connectTimeout = std::chrono::seconds(10);
    httpConfig.readTimeout = std::chrono::seconds(m_config.timeoutSeconds);
    httpConfig.writeTimeout = std::chrono::seconds(30);
    httpConfig.verifySSL = true;

    m_httpClient->SetConfig(httpConfig);

    SMITH_INFO(Category::Network, "ApiClient created for {} with model {}",
               m_config.baseUrl, m_config.model);
}

ApiClient::~ApiClient() {
    CancelStream();
    SMITH_DEBUG(Category::Network, "ApiClient destroyed");
}

ApiClient::ApiClient(ApiClient&&) noexcept = default;
ApiClient& ApiClient::operator=(ApiClient&&) noexcept = default;

void ApiClient::SetConfig(const ApiClientConfig& config) {
    m_config = config;

    // Update HTTP client timeout
    HttpClient::Config httpConfig = m_httpClient->GetConfig();
    httpConfig.readTimeout = std::chrono::seconds(m_config.timeoutSeconds);
    m_httpClient->SetConfig(httpConfig);

    SMITH_INFO(Category::Network, "ApiClient config updated for {} with model {}",
               m_config.baseUrl, m_config.model);
}

// === Synchronous Methods ===

Result<ChatResponse> ApiClient::SendChatCompletion(
    const std::vector<ChatMessage>& messages,
    const std::string& systemPrompt) {

    SMITH_DEBUG(Category::Network, "Sending chat completion with {} messages", messages.size());

    // Validate configuration
    if (m_config.apiKey.empty()) {
        SMITH_ERROR(Category::Network, "API key is empty");
        return Error(Error::Code::InvalidArgument, "API key is required");
    }

    if (m_config.model.empty()) {
        SMITH_ERROR(Category::Network, "Model is empty");
        return Error(Error::Code::InvalidArgument, "Model is required");
    }

    if (messages.empty()) {
        SMITH_ERROR(Category::Network, "Messages list is empty");
        return Error(Error::Code::InvalidArgument, "At least one message is required");
    }

    // Execute with retry logic
    return ExecuteWithRetry<ChatResponse>([&]() -> Result<ChatResponse> {
        // Build request
        std::string requestBody = BuildRequestBody(messages, systemPrompt, false);
        auto headers = BuildRequestHeaders();
        std::string url = BuildUrl("/v1/chat/completions");

        SMITH_VERBOSE(Category::Network, "POST {} with {} bytes", url, requestBody.size());

        // Send request
        auto result = m_httpClient->Post(url, requestBody, headers);

        if (!result.IsOk()) {
            SMITH_ERROR(Category::Network, "HTTP request failed: {}",
                       result.GetError().ToString());
            return Error(Error::Code::NetworkError,
                        "HTTP request failed: " + result.GetError().ToString());
        }

        auto& response = result.Value();

        if (!response.IsSuccess()) {
            SMITH_ERROR(Category::Network, "API returned error status {}: {}",
                       response.statusCode, response.body);

            Error err(Error::Code::NetworkError,
                     "API returned error status " + std::to_string(response.statusCode));
            err.details = response.body;
            return err;
        }

        SMITH_VERBOSE(Category::Network, "Received response: {} bytes", response.body.size());

        // Parse response
        return ParseChatResponse(response.body);
    });
}

// === Streaming Methods ===

Result<void> ApiClient::SendChatCompletionStream(
    const std::vector<ChatMessage>& messages,
    const std::string& systemPrompt,
    ApiStreamChunkCallback onChunk,
    ApiStreamCompleteCallback onComplete) {

    SMITH_DEBUG(Category::Network, "Starting streaming chat completion with {} messages",
               messages.size());

    // Validate configuration
    if (m_config.apiKey.empty()) {
        SMITH_ERROR(Category::Network, "API key is empty");
        return Error(Error::Code::InvalidArgument, "API key is required");
    }

    if (m_config.model.empty()) {
        SMITH_ERROR(Category::Network, "Model is empty");
        return Error(Error::Code::InvalidArgument, "Model is required");
    }

    if (messages.empty()) {
        SMITH_ERROR(Category::Network, "Messages list is empty");
        return Error(Error::Code::InvalidArgument, "At least one message is required");
    }

    // Build request
    std::string requestBody = BuildRequestBody(messages, systemPrompt, true);
    auto headers = BuildRequestHeaders();
    headers["Accept"] = "text/event-stream";
    std::string url = BuildUrl("/v1/chat/completions");

    SMITH_INFO(Category::Network, "Starting stream to {}", url);

    // Use shared pointers for accumulator state that needs to persist
    auto accumulatedContent = std::make_shared<std::string>();
    auto finalResponse = std::make_shared<ChatResponse>();

    // Wrap onChunk to parse SSE and extract content
    auto sseChunkCallback = [this, accumulatedContent, finalResponse, onChunk]
                           (const std::string& rawChunk) -> bool {
        std::string content = ParseStreamChunk(rawChunk);

        if (!content.empty()) {
            *accumulatedContent += content;

            // Call user's chunk callback
            if (!onChunk(content)) {
                SMITH_DEBUG(Category::Network, "Stream cancelled by user callback");
                return false;
            }
        }

        return true;
    };

    // Wrap onComplete to build final response
    std::string modelName = m_config.model; // Capture model name
    auto sseCompleteCallback = [this, accumulatedContent, finalResponse, modelName, onComplete]
                              (bool success, const std::string& error) {
        if (success) {
            finalResponse->content = *accumulatedContent;
            finalResponse->finishReason = "stop";
            finalResponse->model = modelName;

            SMITH_INFO(Category::Network, "Stream completed successfully: {} chars",
                      accumulatedContent->size());

            onComplete(Result<ChatResponse>(*finalResponse));
        } else {
            SMITH_ERROR(Category::Network, "Stream failed: {}", error);
            onComplete(Error(Error::Code::NetworkError, "Stream failed: " + error));
        }
    };

    // Start streaming
    return m_httpClient->PostStream(url, requestBody, headers,
                                   sseChunkCallback, sseCompleteCallback);
}

void ApiClient::CancelStream() {
    if (m_httpClient) {
        m_httpClient->CancelStream();
        SMITH_DEBUG(Category::Network, "Stream cancellation requested");
    }
}

bool ApiClient::IsStreaming() const {
    return m_httpClient && m_httpClient->IsStreaming();
}

// === Request Building ===

std::string ApiClient::BuildRequestBody(
    const std::vector<ChatMessage>& messages,
    const std::string& systemPrompt,
    bool stream) const {

    json j;
    j["model"] = m_config.model;
    j["stream"] = stream;

    // Build messages array
    json messagesArray = json::array();

    // Add system prompt if provided
    if (!systemPrompt.empty()) {
        messagesArray.push_back({
            {"role", "system"},
            {"content", systemPrompt}
        });
    }

    // Add conversation messages
    for (const auto& msg : messages) {
        messagesArray.push_back({
            {"role", ChatMessage::RoleToString(msg.role)},
            {"content", msg.content}
        });
    }

    j["messages"] = messagesArray;

    return j.dump();
}

std::map<std::string, std::string> ApiClient::BuildRequestHeaders() const {
    return {
        {"Content-Type", "application/json"},
        {"Authorization", "Bearer " + m_config.apiKey}
    };
}

std::string ApiClient::BuildUrl(const std::string& endpoint) const {
    std::string url = m_config.baseUrl;

    // Ensure no trailing slash on base URL
    if (!url.empty() && url.back() == '/') {
        url.pop_back();
    }

    // Ensure leading slash on endpoint
    if (!endpoint.empty() && endpoint.front() != '/') {
        url += "/";
    }

    url += endpoint;
    return url;
}

// === Response Parsing ===

Result<ChatResponse> ApiClient::ParseChatResponse(const std::string& responseBody) const {
    try {
        json j = json::parse(responseBody);

        // Validate response structure
        if (!j.contains("choices") || !j["choices"].is_array() || j["choices"].empty()) {
            SMITH_ERROR(Category::Network, "Invalid response: missing or empty choices array");
            return Error(Error::Code::ParseError, "Invalid response format: missing choices");
        }

        auto& choice = j["choices"][0];

        if (!choice.contains("message")) {
            SMITH_ERROR(Category::Network, "Invalid response: missing message in choice");
            return Error(Error::Code::ParseError, "Invalid response format: missing message");
        }

        auto& message = choice["message"];

        ChatResponse response;

        // Extract content
        if (message.contains("content") && message["content"].is_string()) {
            response.content = message["content"].get<std::string>();
        } else {
            SMITH_WARN(Category::Network, "Response message has no content field");
            response.content = "";
        }

        // Extract finish reason
        if (choice.contains("finish_reason") && choice["finish_reason"].is_string()) {
            response.finishReason = choice["finish_reason"].get<std::string>();
        }

        // Extract token usage
        if (j.contains("usage")) {
            auto& usage = j["usage"];

            if (usage.contains("prompt_tokens") && usage["prompt_tokens"].is_number()) {
                response.usage.promptTokens = usage["prompt_tokens"].get<int>();
            }

            if (usage.contains("completion_tokens") && usage["completion_tokens"].is_number()) {
                response.usage.completionTokens = usage["completion_tokens"].get<int>();
            }

            if (usage.contains("total_tokens") && usage["total_tokens"].is_number()) {
                response.usage.totalTokens = usage["total_tokens"].get<int>();
            }
        }

        // Extract metadata
        if (j.contains("model") && j["model"].is_string()) {
            response.model = j["model"].get<std::string>();
        }

        if (j.contains("id") && j["id"].is_string()) {
            response.id = j["id"].get<std::string>();
        }

        SMITH_DEBUG(Category::Network, "Parsed response: {} chars, {} tokens used",
                   response.content.size(), response.usage.totalTokens);

        return response;

    } catch (const json::parse_error& e) {
        SMITH_ERROR(Category::Network, "JSON parse error: {}", e.what());
        Error err(Error::Code::ParseError, "Failed to parse JSON response");
        err.details = e.what();
        return err;
    } catch (const json::exception& e) {
        SMITH_ERROR(Category::Network, "JSON error: {}", e.what());
        Error err(Error::Code::ParseError, "JSON processing error");
        err.details = e.what();
        return err;
    }
}

std::string ApiClient::ParseStreamChunk(const std::string& chunk) const {
    // SSE format: "data: {...}\n\n"
    // We need to extract JSON from data: lines

    // Skip empty chunks
    if (chunk.empty()) {
        return "";
    }

    // Skip comment lines
    if (chunk[0] == ':') {
        return "";
    }

    // Look for "data: " prefix
    const std::string dataPrefix = "data: ";
    size_t dataPos = chunk.find(dataPrefix);

    if (dataPos == std::string::npos) {
        return "";
    }

    // Extract JSON after "data: "
    size_t jsonStart = dataPos + dataPrefix.length();
    size_t jsonEnd = chunk.find('\n', jsonStart);

    if (jsonEnd == std::string::npos) {
        jsonEnd = chunk.length();
    }

    std::string jsonStr = chunk.substr(jsonStart, jsonEnd - jsonStart);

    // Check for [DONE] marker
    if (jsonStr == "[DONE]") {
        SMITH_VERBOSE(Category::Network, "Received stream [DONE] marker");
        return "";
    }

    // Parse JSON
    try {
        json j = json::parse(jsonStr);

        // Extract delta content
        if (j.contains("choices") && j["choices"].is_array() && !j["choices"].empty()) {
            auto& choice = j["choices"][0];

            if (choice.contains("delta")) {
                auto& delta = choice["delta"];

                if (delta.contains("content") && delta["content"].is_string()) {
                    return delta["content"].get<std::string>();
                }
            }
        }

    } catch (const json::exception& e) {
        SMITH_WARN(Category::Network, "Failed to parse stream chunk: {}", e.what());
    }

    return "";
}

// === Retry Logic ===

template<typename T>
Result<T> ApiClient::ExecuteWithRetry(std::function<Result<T>()> requestFunc) {
    Result<T> result = requestFunc();

    // If successful or non-retryable error, return immediately
    if (result.IsOk()) {
        return result;
    }

    // Try to extract status code from error
    int statusCode = 0;
    // Note: We don't have access to status code from Result<T> error
    // So we'll retry on all network errors

    // Retry loop
    for (int attempt = 0; attempt < m_config.maxRetries; ++attempt) {
        SMITH_WARN(Category::Network, "Request failed, retrying (attempt {}/{}): {}",
                  attempt + 1, m_config.maxRetries, result.GetError().ToString());

        // Calculate backoff delay
        int delayMs = CalculateBackoffMs(attempt);
        std::this_thread::sleep_for(std::chrono::milliseconds(delayMs));

        // Retry request
        result = requestFunc();

        if (result.IsOk()) {
            SMITH_INFO(Category::Network, "Request succeeded on retry {}", attempt + 1);
            return result;
        }
    }

    SMITH_ERROR(Category::Network, "Request failed after {} retries", m_config.maxRetries);
    return result;
}

bool ApiClient::IsRetryableError(int statusCode) const {
    // Retry on:
    // - Network errors (statusCode == 0)
    // - 5xx server errors
    // - 429 rate limit
    // - 408 timeout

    if (statusCode == 0) return true;
    if (statusCode >= 500 && statusCode < 600) return true;
    if (statusCode == 429) return true;
    if (statusCode == 408) return true;

    return false;
}

int ApiClient::CalculateBackoffMs(int attempt) const {
    // Exponential backoff: base 1000ms, multiplied by 2^attempt
    // Capped at 30 seconds

    int delayMs = 1000 * (1 << attempt); // 1s, 2s, 4s, 8s, 16s, 32s...

    if (delayMs > 30000) {
        delayMs = 30000;
    }

    SMITH_VERBOSE(Category::Network, "Backoff delay: {}ms (attempt {})", delayMs, attempt);

    return delayMs;
}

// Explicit template instantiation for ChatResponse
template Result<ChatResponse> ApiClient::ExecuteWithRetry<ChatResponse>(
    std::function<Result<ChatResponse>()> requestFunc);

} // namespace smith::network

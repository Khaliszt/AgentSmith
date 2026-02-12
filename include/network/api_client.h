// C:\FarfadetsCorp\AgentSmith\include\network\api_client.h

#pragma once

#include "core/result.h"
#include "network/http_client.h"
#include <string>
#include <vector>
#include <functional>
#include <memory>

namespace smith::network {

/**
 * @brief Configuration for OpenAI-compatible API providers
 */
struct ApiClientConfig {
    std::string baseUrl;        // e.g., "https://api.x.ai" or "https://api.openai.com"
    std::string apiKey;         // API key for authentication
    std::string model;          // e.g., "grok-2", "gpt-4-turbo"
    int timeoutSeconds = 60;    // Request timeout
    int maxRetries = 3;         // Maximum retry attempts
    bool stream = false;        // Enable streaming by default in requests
};

/**
 * @brief Chat message structure for conversations
 */
struct ChatMessage {
    enum class Role {
        System,
        User,
        Assistant
    };

    Role role;
    std::string content;

    ChatMessage() : role(Role::User) {}
    ChatMessage(Role r, std::string c) : role(r), content(std::move(c)) {}

    /**
     * @brief Converts role enum to OpenAI API string
     */
    static const char* RoleToString(Role role);

    /**
     * @brief Converts OpenAI API string to role enum
     */
    static Role StringToRole(const std::string& str);
};

/**
 * @brief Token usage statistics from API response
 */
struct TokenUsage {
    int promptTokens = 0;
    int completionTokens = 0;
    int totalTokens = 0;
};

/**
 * @brief Response from chat completion request
 */
struct ChatResponse {
    std::string content;            // The assistant's message content
    std::string finishReason;       // e.g., "stop", "length", "content_filter"
    TokenUsage usage;               // Token usage statistics
    std::string model;              // Model used for completion
    std::string id;                 // Response ID

    bool IsComplete() const { return finishReason == "stop"; }
};

/**
 * @brief Callback for streaming response chunks
 *
 * @param chunk The content chunk received
 * @return false to cancel the stream
 */
using ApiStreamChunkCallback = std::function<bool(const std::string& chunk)>;

/**
 * @brief Callback when streaming completes
 *
 * @param response The complete response (if successful)
 * @param error Error message (if failed)
 */
using ApiStreamCompleteCallback = std::function<void(
    const core::Result<ChatResponse>& response
)>;

/**
 * @brief OpenAI-compatible API client
 *
 * Provides a high-level interface for OpenAI-compatible chat completion APIs
 * including xAI Grok and OpenAI ChatGPT. Supports both synchronous and
 * streaming requests with automatic retry logic.
 *
 * Features:
 * - Synchronous chat completions
 * - Streaming chat completions (Server-Sent Events)
 * - Automatic retry with exponential backoff
 * - Token usage tracking
 * - OpenAI JSON format for requests/responses
 *
 * Example usage:
 * @code
 * ApiClientConfig config;
 * config.baseUrl = "https://api.openai.com";
 * config.apiKey = "sk-...";
 * config.model = "gpt-4-turbo";
 *
 * ApiClient client(config);
 *
 * std::vector<ChatMessage> messages = {
 *     ChatMessage(ChatMessage::Role::User, "Hello!")
 * };
 *
 * auto result = client.SendChatCompletion(messages, "You are a helpful assistant.");
 * if (result.IsOk()) {
 *     std::cout << result.Value().content << std::endl;
 * }
 * @endcode
 */
class ApiClient {
public:
    /**
     * @brief Constructs API client with default configuration
     */
    ApiClient();

    /**
     * @brief Constructs API client with custom configuration
     *
     * @param config Client configuration
     */
    explicit ApiClient(const ApiClientConfig& config);

    ~ApiClient();

    // Prevent copy, allow move
    ApiClient(const ApiClient&) = delete;
    ApiClient& operator=(const ApiClient&) = delete;
    ApiClient(ApiClient&&) noexcept;
    ApiClient& operator=(ApiClient&&) noexcept;

    // === Configuration ===

    /**
     * @brief Sets client configuration
     *
     * @param config New configuration
     */
    void SetConfig(const ApiClientConfig& config);

    /**
     * @brief Gets current configuration
     *
     * @return Current configuration
     */
    const ApiClientConfig& GetConfig() const { return m_config; }

    // === Synchronous Methods ===

    /**
     * @brief Sends synchronous chat completion request
     *
     * Sends a chat completion request to the API and waits for the full response.
     * Automatically retries on transient failures with exponential backoff.
     *
     * @param messages The conversation messages
     * @param systemPrompt Optional system prompt (prepended to messages)
     * @return Result with ChatResponse or error
     */
    core::Result<ChatResponse> SendChatCompletion(
        const std::vector<ChatMessage>& messages,
        const std::string& systemPrompt = ""
    );

    // === Streaming Methods ===

    /**
     * @brief Sends streaming chat completion request
     *
     * Sends a chat completion request with streaming enabled. The onChunk callback
     * is called for each content chunk received, and onComplete is called when the
     * stream finishes or errors.
     *
     * The stream can be cancelled by returning false from the onChunk callback.
     *
     * @param messages The conversation messages
     * @param systemPrompt Optional system prompt (prepended to messages)
     * @param onChunk Callback for each content chunk (return false to cancel)
     * @param onComplete Callback when stream completes
     * @return Result indicating if stream was started successfully
     */
    core::Result<void> SendChatCompletionStream(
        const std::vector<ChatMessage>& messages,
        const std::string& systemPrompt,
        ApiStreamChunkCallback onChunk,
        ApiStreamCompleteCallback onComplete
    );

    /**
     * @brief Cancels any ongoing streaming operation
     */
    void CancelStream();

    /**
     * @brief Checks if a stream is currently active
     *
     * @return True if streaming
     */
    bool IsStreaming() const;

private:
    ApiClientConfig m_config;
    std::unique_ptr<HttpClient> m_httpClient;

    // === Request Building ===

    /**
     * @brief Builds OpenAI-format JSON request body
     *
     * @param messages The conversation messages
     * @param systemPrompt Optional system prompt
     * @param stream Enable streaming
     * @return JSON string for request body
     */
    std::string BuildRequestBody(
        const std::vector<ChatMessage>& messages,
        const std::string& systemPrompt,
        bool stream
    ) const;

    /**
     * @brief Builds request headers with authentication
     *
     * @return Map of HTTP headers
     */
    std::map<std::string, std::string> BuildRequestHeaders() const;

    /**
     * @brief Constructs full API endpoint URL
     *
     * @param endpoint API endpoint path (e.g., "/v1/chat/completions")
     * @return Full URL
     */
    std::string BuildUrl(const std::string& endpoint) const;

    // === Response Parsing ===

    /**
     * @brief Parses non-streaming chat completion response
     *
     * Extracts content, token usage, and metadata from OpenAI JSON response.
     *
     * @param responseBody JSON response body
     * @return Result with ChatResponse or parse error
     */
    core::Result<ChatResponse> ParseChatResponse(const std::string& responseBody) const;

    /**
     * @brief Parses streaming SSE chunk
     *
     * Extracts delta content from Server-Sent Events data chunk.
     * Returns empty string for non-data chunks (comments, events, etc.).
     *
     * @param chunk Raw SSE chunk
     * @return Content delta or empty string
     */
    std::string ParseStreamChunk(const std::string& chunk) const;

    // === Retry Logic ===

    /**
     * @brief Executes request with retry logic
     *
     * Retries on transient failures (timeouts, 5xx errors) with exponential backoff.
     *
     * @param requestFunc Function that performs the request
     * @return Result from request
     */
    template<typename T>
    core::Result<T> ExecuteWithRetry(
        std::function<core::Result<T>()> requestFunc
    );

    /**
     * @brief Determines if an error is retryable
     *
     * @param statusCode HTTP status code (0 for network errors)
     * @return True if should retry
     */
    bool IsRetryableError(int statusCode) const;

    /**
     * @brief Calculates backoff delay for retry attempt
     *
     * @param attempt Retry attempt number (0-based)
     * @return Delay in milliseconds
     */
    int CalculateBackoffMs(int attempt) const;
};

} // namespace smith::network

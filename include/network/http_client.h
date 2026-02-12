// C:\FarfadetsCorp\AgentSmith\include\network\http_client.h

#pragma once

#include "core/result.h"
#include <string>
#include <map>
#include <future>
#include <functional>
#include <memory>
#include <atomic>
#include <chrono>

namespace smith::network {

/**
 * @brief HTTP response data
 */
struct HttpResponse {
    int statusCode = 0;
    std::string body;
    std::map<std::string, std::string> headers;
    std::string error;

    bool IsSuccess() const { return statusCode >= 200 && statusCode < 300; }
    bool IsError() const { return statusCode >= 400 || statusCode == 0; }
};

/**
 * @brief Callback for streaming chunks (Server-Sent Events)
 *
 * @param chunk The received data chunk
 * @return false to cancel the stream
 */
using StreamChunkCallback = std::function<bool(const std::string& chunk)>;

/**
 * @brief Callback when stream completes
 *
 * @param success True if stream completed successfully
 * @param error Error message if failed
 */
using StreamCompleteCallback = std::function<void(bool success, const std::string& error)>;

/**
 * @brief HTTP client wrapper using cpp-httplib
 *
 * Provides synchronous, asynchronous, and streaming HTTP operations
 * with HTTPS support, timeout configuration, and thread-safe operation.
 *
 * Features:
 * - Synchronous GET/POST
 * - Asynchronous GET/POST with std::future
 * - Streaming POST for SSE (Server-Sent Events)
 * - HTTPS support (requires CPPHTTPLIB_OPENSSL_SUPPORT)
 * - Configurable timeouts
 * - Thread-safe operation
 * - Cancellation support
 *
 * NOTE: HTTPS requires OpenSSL. To enable HTTPS support:
 * 1. Set HTTPLIB_REQUIRE_OPENSSL ON in cmake/Dependencies.cmake
 * 2. Ensure OpenSSL is installed on the system
 * 3. cpp-httplib will automatically detect and use OpenSSL
 */
class HttpClient {
public:
    /**
     * @brief Configuration for HTTP client
     */
    struct Config {
        std::chrono::seconds connectTimeout = std::chrono::seconds(10);
        std::chrono::seconds readTimeout = std::chrono::seconds(120);  // Long for streaming
        std::chrono::seconds writeTimeout = std::chrono::seconds(30);
        bool followRedirects = true;
        int maxRedirects = 5;
        bool verifySSL = true;
    };

    /**
     * @brief Constructs HTTP client with default configuration
     */
    HttpClient();

    /**
     * @brief Constructs HTTP client with custom configuration
     *
     * @param config Client configuration
     */
    explicit HttpClient(const Config& config);

    ~HttpClient();

    // Prevent copy, allow move
    HttpClient(const HttpClient&) = delete;
    HttpClient& operator=(const HttpClient&) = delete;
    HttpClient(HttpClient&&) noexcept;
    HttpClient& operator=(HttpClient&&) noexcept;

    // === Configuration ===

    /**
     * @brief Sets client configuration
     *
     * @param config New configuration
     */
    void SetConfig(const Config& config);

    /**
     * @brief Gets current configuration
     *
     * @return Current configuration
     */
    const Config& GetConfig() const { return m_config; }

    // === Synchronous Methods ===

    /**
     * @brief Performs synchronous HTTP GET request
     *
     * @param url Target URL (must be HTTPS for API calls)
     * @param headers Optional request headers
     * @return Result with HttpResponse or error
     */
    core::Result<HttpResponse> Get(
        const std::string& url,
        const std::map<std::string, std::string>& headers = {}
    );

    /**
     * @brief Performs synchronous HTTP POST request
     *
     * @param url Target URL (must be HTTPS for API calls)
     * @param body Request body
     * @param headers Optional request headers
     * @return Result with HttpResponse or error
     */
    core::Result<HttpResponse> Post(
        const std::string& url,
        const std::string& body,
        const std::map<std::string, std::string>& headers = {}
    );

    // === Asynchronous Methods ===

    /**
     * @brief Performs asynchronous HTTP GET request
     *
     * @param url Target URL
     * @param headers Optional request headers
     * @return Future with Result containing HttpResponse or error
     */
    std::future<core::Result<HttpResponse>> GetAsync(
        const std::string& url,
        const std::map<std::string, std::string>& headers = {}
    );

    /**
     * @brief Performs asynchronous HTTP POST request
     *
     * @param url Target URL
     * @param body Request body
     * @param headers Optional request headers
     * @return Future with Result containing HttpResponse or error
     */
    std::future<core::Result<HttpResponse>> PostAsync(
        const std::string& url,
        const std::string& body,
        const std::map<std::string, std::string>& headers = {}
    );

    // === Streaming Methods ===

    /**
     * @brief Performs HTTP POST with streaming response (for SSE)
     *
     * This method is designed for Server-Sent Events (SSE) streaming from
     * LLM APIs like xAI Grok and OpenAI ChatGPT. It calls onChunk for each
     * received data chunk and onComplete when done.
     *
     * The onChunk callback should return false to cancel the stream.
     *
     * @param url Target URL (must be HTTPS)
     * @param body Request body (typically JSON)
     * @param headers Request headers (should include "Accept: text/event-stream")
     * @param onChunk Callback for each data chunk (return false to cancel)
     * @param onComplete Callback when stream completes or errors
     * @return Result indicating if stream was started successfully
     */
    core::Result<void> PostStream(
        const std::string& url,
        const std::string& body,
        const std::map<std::string, std::string>& headers,
        StreamChunkCallback onChunk,
        StreamCompleteCallback onComplete
    );

    // === Cancellation ===

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
    struct Impl;
    std::unique_ptr<Impl> m_impl;

    Config m_config;
    std::atomic<bool> m_cancelStream{false};
    std::atomic<bool> m_isStreaming{false};

    // Helper methods
    core::Result<HttpResponse> ExecuteRequest(
        const std::string& method,
        const std::string& url,
        const std::string& body,
        const std::map<std::string, std::string>& headers
    );

    std::pair<std::string, std::string> ParseUrl(const std::string& url);
};

} // namespace smith::network

// C:\FarfadetsCorp\AgentSmith\src\network\http_client.cpp

#include "network/http_client.h"
#include "logging/log_macros.h"
#include "logging/logger.h"

#include <httplib.h>

#include <sstream>
#include <regex>
#include <thread>

namespace smith::network {

// === Implementation Details ===

struct HttpClient::Impl {
    std::unique_ptr<httplib::Client> client;
    std::mutex clientMutex;
};

// === Constructor/Destructor ===

HttpClient::HttpClient()
    : m_impl(std::make_unique<Impl>())
    , m_config() {
    SMITH_DEBUG(logging::Category::Network, "HttpClient created with default config");
}

HttpClient::HttpClient(const Config& config)
    : m_impl(std::make_unique<Impl>())
    , m_config(config) {
    SMITH_DEBUG(logging::Category::Network, "HttpClient created with custom config");
}

HttpClient::~HttpClient() {
    CancelStream();
    SMITH_DEBUG(logging::Category::Network, "HttpClient destroyed");
}

// Move constructor - atomics can't be copied, need custom implementation
HttpClient::HttpClient(HttpClient&& other) noexcept
    : m_impl(std::move(other.m_impl))
    , m_config(std::move(other.m_config))
    , m_cancelStream(other.m_cancelStream.load())
    , m_isStreaming(other.m_isStreaming.load()) {
}

HttpClient& HttpClient::operator=(HttpClient&& other) noexcept {
    if (this != &other) {
        m_impl = std::move(other.m_impl);
        m_config = std::move(other.m_config);
        m_cancelStream.store(other.m_cancelStream.load());
        m_isStreaming.store(other.m_isStreaming.load());
    }
    return *this;
}

// === Configuration ===

void HttpClient::SetConfig(const Config& config) {
    std::lock_guard<std::mutex> lock(m_impl->clientMutex);
    m_config = config;
    SMITH_DEBUG(logging::Category::Network, "HttpClient config updated");
}

// === Helper Methods ===

std::pair<std::string, std::string> HttpClient::ParseUrl(const std::string& url) {
    // Parse URL into scheme+host and path
    // Example: "https://api.example.com/v1/chat" -> ("https://api.example.com", "/v1/chat")

    std::regex urlRegex(R"(^(https?://[^/]+)(/.*)?)");
    std::smatch match;

    if (std::regex_match(url, match, urlRegex)) {
        std::string base = match[1].str();
        std::string path = match[2].str();
        if (path.empty()) path = "/";
        return {base, path};
    }

    SMITH_ERROR(logging::Category::Network, "Invalid URL format: {}", url);
    return {"", ""};
}

core::Result<HttpResponse> HttpClient::ExecuteRequest(
    const std::string& method,
    const std::string& url,
    const std::string& body,
    const std::map<std::string, std::string>& headers) {

    SMITH_DEBUG(logging::Category::Network, "Executing {} request to {}", method, url);

    // Parse URL
    auto [baseUrl, path] = ParseUrl(url);
    if (baseUrl.empty()) {
        return core::Err<HttpResponse>(
            core::Error::Code::InvalidArgument,
            "Invalid URL format"
        );
    }

    // Create client for this request
    std::unique_ptr<httplib::Client> client;

    try {
        client = std::make_unique<httplib::Client>(baseUrl);

        // Configure timeouts
        client->set_connection_timeout(m_config.connectTimeout);
        client->set_read_timeout(m_config.readTimeout);
        client->set_write_timeout(m_config.writeTimeout);

        // Configure redirects
        client->set_follow_location(m_config.followRedirects);

        // SSL verification (only available with CPPHTTPLIB_OPENSSL_SUPPORT)
#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
        if (baseUrl.find("https://") == 0) {
            client->enable_server_certificate_verification(m_config.verifySSL);
        }
#else
        // Warn if trying to use HTTPS without SSL support
        if (baseUrl.find("https://") == 0) {
            SMITH_WARN(logging::Category::Network,
                       "HTTPS URL used but SSL support not compiled in - this will fail!");
        }
#endif

    } catch (const std::exception& e) {
        SMITH_ERROR(logging::Category::Network, "Failed to create HTTP client: {}", e.what());
        return core::Err<HttpResponse>(
            core::Error::Code::NetworkError,
            std::string("Failed to create client: ") + e.what()
        );
    }

    // Prepare headers
    httplib::Headers reqHeaders;
    for (const auto& [key, value] : headers) {
        reqHeaders.emplace(key, value);
    }

    // Execute request
    httplib::Result result;

    try {
        if (method == "GET") {
            result = client->Get(path, reqHeaders);
        } else if (method == "POST") {
            // Determine content type
            std::string contentType = "application/json";
            auto it = headers.find("Content-Type");
            if (it != headers.end()) {
                contentType = it->second;
            }

            result = client->Post(path, reqHeaders, body, contentType);
        } else {
            return core::Err<HttpResponse>(
                core::Error::Code::InvalidArgument,
                "Unsupported HTTP method: " + method
            );
        }
    } catch (const std::exception& e) {
        SMITH_ERROR(logging::Category::Network, "HTTP request exception: {}", e.what());
        return core::Err<HttpResponse>(
            core::Error::Code::NetworkError,
            std::string("Request failed: ") + e.what()
        );
    }

    // Check result
    if (!result) {
        auto err = result.error();
        std::string errorMsg = httplib::to_string(err);
        SMITH_ERROR(logging::Category::Network, "HTTP request failed: {}", errorMsg);

        return core::Err<HttpResponse>(
            core::Error::Code::NetworkError,
            "Request failed: " + errorMsg
        );
    }

    // Build response
    HttpResponse response;
    response.statusCode = result->status;
    response.body = result->body;

    // Copy headers
    for (const auto& [key, value] : result->headers) {
        response.headers[key] = value;
    }

    SMITH_DEBUG(logging::Category::Network,
                "{} {} completed with status {}",
                method, url, response.statusCode);

    if (response.IsError()) {
        SMITH_WARN(logging::Category::Network,
                   "HTTP error response: {} - {}",
                   response.statusCode,
                   response.body.substr(0, 200));
    }

    return response;
}

// === Synchronous Methods ===

core::Result<HttpResponse> HttpClient::Get(
    const std::string& url,
    const std::map<std::string, std::string>& headers) {

    return ExecuteRequest("GET", url, "", headers);
}

core::Result<HttpResponse> HttpClient::Post(
    const std::string& url,
    const std::string& body,
    const std::map<std::string, std::string>& headers) {

    return ExecuteRequest("POST", url, body, headers);
}

// === Asynchronous Methods ===

std::future<core::Result<HttpResponse>> HttpClient::GetAsync(
    const std::string& url,
    const std::map<std::string, std::string>& headers) {

    SMITH_DEBUG(logging::Category::Network, "Starting async GET to {}", url);

    return std::async(std::launch::async, [this, url, headers]() {
        return this->Get(url, headers);
    });
}

std::future<core::Result<HttpResponse>> HttpClient::PostAsync(
    const std::string& url,
    const std::string& body,
    const std::map<std::string, std::string>& headers) {

    SMITH_DEBUG(logging::Category::Network, "Starting async POST to {}", url);

    return std::async(std::launch::async, [this, url, body, headers]() {
        return this->Post(url, body, headers);
    });
}

// === Streaming Methods ===

core::Result<void> HttpClient::PostStream(
    const std::string& url,
    const std::string& body,
    const std::map<std::string, std::string>& headers,
    StreamChunkCallback onChunk,
    StreamCompleteCallback onComplete) {

    SMITH_INFO(logging::Category::Network, "Starting streaming POST to {}", url);

    if (m_isStreaming) {
        return core::Err<void>(
            core::Error::Code::AlreadyExists,
            "A stream is already active"
        );
    }

    // Parse URL
    auto [baseUrl, path] = ParseUrl(url);
    if (baseUrl.empty()) {
        return core::Err<void>(
            core::Error::Code::InvalidArgument,
            "Invalid URL format"
        );
    }

    // Start streaming in background thread
    m_isStreaming = true;
    m_cancelStream = false;

    std::thread([this, baseUrl, path, body, headers, onChunk, onComplete]() {
        bool success = false;
        std::string errorMsg;

        try {
            // Create client for streaming
            auto client = std::make_unique<httplib::Client>(baseUrl);

            // Configure timeouts (longer for streaming)
            client->set_connection_timeout(m_config.connectTimeout);
            client->set_read_timeout(std::chrono::seconds(300)); // 5 minutes for streaming
            client->set_write_timeout(m_config.writeTimeout);

            // SSL verification (only available with CPPHTTPLIB_OPENSSL_SUPPORT)
#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
            if (baseUrl.find("https://") == 0) {
                client->enable_server_certificate_verification(m_config.verifySSL);
            }
#else
            // Warn if trying to use HTTPS without SSL support
            if (baseUrl.find("https://") == 0) {
                SMITH_WARN(logging::Category::Network,
                           "HTTPS URL used but SSL support not compiled in - this will fail!");
            }
#endif

            // Prepare headers
            httplib::Headers reqHeaders;
            for (const auto& [key, value] : headers) {
                reqHeaders.emplace(key, value);
            }

            // Determine content type
            std::string contentType = "application/json";
            auto it = headers.find("Content-Type");
            if (it != headers.end()) {
                contentType = it->second;
            }

            SMITH_DEBUG(logging::Category::Network, "Streaming request starting...");

            // Create a Request object with content receiver for streaming response
            httplib::Request req;
            req.method = "POST";
            req.path = path;
            req.headers = reqHeaders;
            req.body = body;
            req.headers.emplace("Content-Type", contentType);

            // Set content receiver to handle streaming chunks
            req.content_receiver = [this, &onChunk](const char* data, size_t dataLen,
                                                     uint64_t /*offset*/, uint64_t /*total*/) -> bool {
                if (m_cancelStream) {
                    SMITH_INFO(logging::Category::Network, "Stream cancelled by user");
                    return false;
                }

                if (dataLen > 0 && onChunk) {
                    std::string chunk(data, dataLen);

                    // Call user callback
                    bool shouldContinue = onChunk(chunk);
                    if (!shouldContinue) {
                        SMITH_DEBUG(logging::Category::Network, "Stream cancelled by callback");
                        return false;
                    }
                }

                return true;
            };

            // Execute streaming POST using send()
            auto result = client->send(req);

            if (result) {
                if (result->status >= 200 && result->status < 300) {
                    SMITH_INFO(logging::Category::Network,
                               "Stream completed successfully with status {}",
                               result->status);
                    success = true;
                } else {
                    errorMsg = "HTTP error: " + std::to_string(result->status);
                    SMITH_ERROR(logging::Category::Network,
                                "Stream failed with status {}: {}",
                                result->status,
                                result->body.substr(0, 200));
                }
            } else {
                auto err = result.error();
                errorMsg = "Request failed: " + std::string(httplib::to_string(err));
                SMITH_ERROR(logging::Category::Network, "Stream error: {}", errorMsg);
            }

        } catch (const std::exception& e) {
            errorMsg = std::string("Exception: ") + e.what();
            SMITH_ERROR(logging::Category::Network, "Stream exception: {}", e.what());
        }

        m_isStreaming = false;

        // Call completion callback
        if (onComplete) {
            onComplete(success, errorMsg);
        }

    }).detach(); // Detach thread to run independently

    return core::Ok();
}

void HttpClient::CancelStream() {
    if (m_isStreaming) {
        SMITH_INFO(logging::Category::Network, "Cancelling active stream");
        m_cancelStream = true;

        // Wait a bit for stream to stop
        for (int i = 0; i < 50 && m_isStreaming; ++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        if (m_isStreaming) {
            SMITH_WARN(logging::Category::Network, "Stream did not stop gracefully");
        }
    }
}

bool HttpClient::IsStreaming() const {
    return m_isStreaming;
}

} // namespace smith::network

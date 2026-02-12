// C:\FarfadetsCorp\AgentSmith\src\agent\conversation.cpp

#include "agent/conversation.h"
#include "logging/logger.h"
#include "logging/log_macros.h"
#include <algorithm>
#include <numeric>

namespace smith::agent {

// === Message Helper Methods ===

const char* Message::RoleToString(Role role) {
    switch (role) {
        case Role::System:    return "system";
        case Role::User:      return "user";
        case Role::Assistant: return "assistant";
        default:              return "unknown";
    }
}

Message::Role Message::StringToRole(const std::string& str) {
    if (str == "system") return Role::System;
    if (str == "user") return Role::User;
    if (str == "assistant") return Role::Assistant;

    SMITH_WARN(logging::Category::Agent, "Unknown message role: '{}', defaulting to User", str);
    return Role::User;
}

// === Conversation Implementation ===

void Conversation::AddMessage(Message::Role role, const std::string& content, int tokens) {
    Message msg(role, content, tokens);

    // Auto-estimate tokens if not provided
    if (tokens == 0) {
        msg.tokenCount = EstimateTokens(content);
    }

    m_history.push_back(msg);

    SMITH_DEBUG(logging::Category::Agent,
                "Added message: role={}, tokens={}, content_length={}",
                Message::RoleToString(role), msg.tokenCount, content.length());
}

void Conversation::AddMessage(const Message& message) {
    Message msg = message;

    // Auto-estimate tokens if not provided
    if (msg.tokenCount == 0) {
        msg.tokenCount = EstimateTokens(msg.content);
    }

    m_history.push_back(msg);

    SMITH_DEBUG(logging::Category::Agent,
                "Added message: role={}, tokens={}, content_length={}",
                Message::RoleToString(msg.role), msg.tokenCount, msg.content.length());
}

const std::vector<Message>& Conversation::GetHistory() const {
    return m_history;
}

std::vector<Message> Conversation::GetLastN(int n) const {
    if (n <= 0) {
        return {};
    }

    if (n >= static_cast<int>(m_history.size())) {
        return m_history;
    }

    // Return last N messages
    auto start_it = m_history.end() - n;
    return std::vector<Message>(start_it, m_history.end());
}

void Conversation::Clear() {
    SMITH_DEBUG(logging::Category::Agent,
                "Clearing conversation history: {} messages, {} tokens",
                m_history.size(), GetTotalTokens());

    m_history.clear();
    m_systemPrompt.clear();
}

void Conversation::SetSystemPrompt(const std::string& prompt) {
    m_systemPrompt = prompt;

    // Update or insert system message at the beginning
    if (!m_history.empty() && m_history[0].role == Message::Role::System) {
        // Update existing system message
        m_history[0].content = prompt;
        m_history[0].tokenCount = EstimateTokens(prompt);
        m_history[0].timestamp = std::chrono::system_clock::now();

        SMITH_DEBUG(logging::Category::Agent,
                    "Updated system prompt: {} tokens", m_history[0].tokenCount);
    } else {
        // Insert new system message at the beginning
        Message sysMsg(Message::Role::System, prompt);
        sysMsg.tokenCount = EstimateTokens(prompt);
        m_history.insert(m_history.begin(), sysMsg);

        SMITH_DEBUG(logging::Category::Agent,
                    "Set system prompt: {} tokens", sysMsg.tokenCount);
    }
}

const std::string& Conversation::GetSystemPrompt() const {
    return m_systemPrompt;
}

int Conversation::GetTotalTokens() const {
    return std::accumulate(m_history.begin(), m_history.end(), 0,
                          [](int sum, const Message& msg) {
                              return sum + msg.tokenCount;
                          });
}

int Conversation::GetMessageCount() const {
    return static_cast<int>(m_history.size());
}

nlohmann::json Conversation::ToApiFormat() const {
    nlohmann::json messages = nlohmann::json::array();

    for (const auto& msg : m_history) {
        nlohmann::json msgJson;
        msgJson["role"] = Message::RoleToString(msg.role);
        msgJson["content"] = msg.content;
        messages.push_back(msgJson);
    }

    SMITH_DEBUG(logging::Category::Agent,
                "Serialized conversation to API format: {} messages, {} tokens",
                messages.size(), GetTotalTokens());

    return messages;
}

int Conversation::EstimateTokens(const std::string& text) {
    // Simple heuristic: ~4 characters per token
    // This is a rough approximation that works reasonably well for English text
    // For more accurate counts, use the actual tokenizer for the specific model

    if (text.empty()) {
        return 0;
    }

    return static_cast<int>(text.length() / 4);
}

} // namespace smith::agent

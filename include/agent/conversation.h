// C:\FarfadetsCorp\AgentSmith\include\agent\conversation.h

#pragma once

#include <string>
#include <vector>
#include <chrono>
#include <nlohmann/json.hpp>

namespace smith::agent {

/**
 * @brief Represents a single message in a conversation
 *
 * Contains message content, role, timestamp, and token count for
 * managing chat history with API-based agents.
 */
struct Message {
    /**
     * @brief Role of the message sender
     */
    enum class Role {
        System,     // System instructions/prompts
        User,       // User input
        Assistant   // Agent/AI response
    };

    Role role;
    std::string content;
    std::chrono::system_clock::time_point timestamp;
    int tokenCount = 0;  // Estimated or from API response

    // === Helper Methods ===

    /**
     * @brief Converts Role enum to string representation
     * @param role The role to convert
     * @return String representation (e.g., "system", "user", "assistant")
     */
    static const char* RoleToString(Role role);

    /**
     * @brief Converts string to Role enum
     * @param str The string to convert (e.g., "system", "user", "assistant")
     * @return Corresponding Role enum value
     */
    static Role StringToRole(const std::string& str);

    /**
     * @brief Default constructor
     */
    Message() = default;

    /**
     * @brief Constructs a message with specified parameters
     * @param r Role of the message
     * @param c Content of the message
     * @param tokens Token count (0 if unknown)
     */
    Message(Role r, const std::string& c, int tokens = 0)
        : role(r), content(c), timestamp(std::chrono::system_clock::now()), tokenCount(tokens) {}
};

/**
 * @brief Manages conversation history for API-based agents
 *
 * Provides methods to add messages, manage context windows, serialize
 * to API format, and track token usage.
 */
class Conversation {
public:
    /**
     * @brief Default constructor
     */
    Conversation() = default;

    // === Message Management ===

    /**
     * @brief Adds a new message to the conversation
     * @param role Role of the message sender
     * @param content Message content
     * @param tokens Token count (0 for auto-estimation)
     */
    void AddMessage(Message::Role role, const std::string& content, int tokens = 0);

    /**
     * @brief Adds a pre-constructed message to the conversation
     * @param message The message to add
     */
    void AddMessage(const Message& message);

    /**
     * @brief Gets the complete conversation history
     * @return Vector of all messages in chronological order
     */
    const std::vector<Message>& GetHistory() const;

    /**
     * @brief Gets the last N messages from history
     * @param n Number of messages to retrieve
     * @return Vector of the last N messages
     *
     * Useful for context window management when dealing with token limits.
     */
    std::vector<Message> GetLastN(int n) const;

    // === Conversation Control ===

    /**
     * @brief Clears all messages from the conversation
     */
    void Clear();

    /**
     * @brief Sets or updates the system prompt
     * @param prompt The system prompt content
     *
     * If a system message already exists, it will be updated.
     * Otherwise, a new system message will be added at the beginning.
     */
    void SetSystemPrompt(const std::string& prompt);

    /**
     * @brief Gets the current system prompt
     * @return The system prompt content, or empty string if none exists
     */
    const std::string& GetSystemPrompt() const;

    // === Metrics ===

    /**
     * @brief Calculates total token count for all messages
     * @return Sum of token counts across all messages
     */
    int GetTotalTokens() const;

    /**
     * @brief Gets the number of messages in the conversation
     * @return Message count
     */
    int GetMessageCount() const;

    // === Serialization ===

    /**
     * @brief Serializes conversation to OpenAI API format
     * @return JSON array of message objects with "role" and "content" fields
     *
     * Example output:
     * [
     *   {"role": "system", "content": "..."},
     *   {"role": "user", "content": "..."},
     *   {"role": "assistant", "content": "..."}
     * ]
     */
    nlohmann::json ToApiFormat() const;

    // === Static Utilities ===

    /**
     * @brief Estimates token count for text
     * @param text The text to estimate
     * @return Estimated token count using chars/4 heuristic
     *
     * This is a rough approximation. For accurate counts, use the
     * actual API response or a proper tokenizer.
     */
    static int EstimateTokens(const std::string& text);

private:
    std::vector<Message> m_history;
    std::string m_systemPrompt;
};

} // namespace smith::agent

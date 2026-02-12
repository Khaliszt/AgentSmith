// C:\FarfadetsCorp\AgentSmith\tests\test_conversation.cpp

#include "agent/conversation.h"
#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

using namespace smith::agent;

class ConversationTest : public ::testing::Test {
protected:
    Conversation conv;
};

// === Message Tests ===

TEST_F(ConversationTest, RoleToString) {
    EXPECT_STREQ("system", Message::RoleToString(Message::Role::System));
    EXPECT_STREQ("user", Message::RoleToString(Message::Role::User));
    EXPECT_STREQ("assistant", Message::RoleToString(Message::Role::Assistant));
}

TEST_F(ConversationTest, StringToRole) {
    EXPECT_EQ(Message::Role::System, Message::StringToRole("system"));
    EXPECT_EQ(Message::Role::User, Message::StringToRole("user"));
    EXPECT_EQ(Message::Role::Assistant, Message::StringToRole("assistant"));

    // Unknown role defaults to User
    EXPECT_EQ(Message::Role::User, Message::StringToRole("unknown"));
}

TEST_F(ConversationTest, MessageConstruction) {
    Message msg(Message::Role::User, "Hello", 10);

    EXPECT_EQ(Message::Role::User, msg.role);
    EXPECT_EQ("Hello", msg.content);
    EXPECT_EQ(10, msg.tokenCount);
}

// === Conversation Tests ===

TEST_F(ConversationTest, AddMessage) {
    conv.AddMessage(Message::Role::User, "Hello");

    EXPECT_EQ(1, conv.GetMessageCount());

    const auto& history = conv.GetHistory();
    EXPECT_EQ(1, history.size());
    EXPECT_EQ(Message::Role::User, history[0].role);
    EXPECT_EQ("Hello", history[0].content);
}

TEST_F(ConversationTest, AddMessageWithTokens) {
    conv.AddMessage(Message::Role::User, "Hello", 5);

    const auto& history = conv.GetHistory();
    EXPECT_EQ(5, history[0].tokenCount);
}

TEST_F(ConversationTest, AddMessageAutoEstimate) {
    // "Hello" is 5 chars, estimated as 5/4 = 1 token
    conv.AddMessage(Message::Role::User, "Hello");

    const auto& history = conv.GetHistory();
    EXPECT_GT(history[0].tokenCount, 0);
}

TEST_F(ConversationTest, AddPreConstructedMessage) {
    Message msg(Message::Role::Assistant, "Response", 20);
    conv.AddMessage(msg);

    EXPECT_EQ(1, conv.GetMessageCount());

    const auto& history = conv.GetHistory();
    EXPECT_EQ(Message::Role::Assistant, history[0].role);
    EXPECT_EQ("Response", history[0].content);
    EXPECT_EQ(20, history[0].tokenCount);
}

TEST_F(ConversationTest, GetHistory) {
    conv.AddMessage(Message::Role::User, "First");
    conv.AddMessage(Message::Role::Assistant, "Second");
    conv.AddMessage(Message::Role::User, "Third");

    const auto& history = conv.GetHistory();
    EXPECT_EQ(3, history.size());
    EXPECT_EQ("First", history[0].content);
    EXPECT_EQ("Second", history[1].content);
    EXPECT_EQ("Third", history[2].content);
}

TEST_F(ConversationTest, GetLastN) {
    conv.AddMessage(Message::Role::User, "First");
    conv.AddMessage(Message::Role::Assistant, "Second");
    conv.AddMessage(Message::Role::User, "Third");
    conv.AddMessage(Message::Role::Assistant, "Fourth");

    auto last2 = conv.GetLastN(2);
    EXPECT_EQ(2, last2.size());
    EXPECT_EQ("Third", last2[0].content);
    EXPECT_EQ("Fourth", last2[1].content);
}

TEST_F(ConversationTest, GetLastNMoreThanAvailable) {
    conv.AddMessage(Message::Role::User, "First");
    conv.AddMessage(Message::Role::User, "Second");

    auto last10 = conv.GetLastN(10);
    EXPECT_EQ(2, last10.size());
}

TEST_F(ConversationTest, GetLastNZero) {
    conv.AddMessage(Message::Role::User, "First");

    auto last0 = conv.GetLastN(0);
    EXPECT_EQ(0, last0.size());
}

TEST_F(ConversationTest, Clear) {
    conv.AddMessage(Message::Role::User, "First");
    conv.AddMessage(Message::Role::Assistant, "Second");

    EXPECT_EQ(2, conv.GetMessageCount());

    conv.Clear();

    EXPECT_EQ(0, conv.GetMessageCount());
    EXPECT_EQ(0, conv.GetTotalTokens());
}

TEST_F(ConversationTest, SetSystemPrompt) {
    conv.SetSystemPrompt("You are a helpful assistant");

    EXPECT_EQ(1, conv.GetMessageCount());

    const auto& history = conv.GetHistory();
    EXPECT_EQ(Message::Role::System, history[0].role);
    EXPECT_EQ("You are a helpful assistant", history[0].content);
}

TEST_F(ConversationTest, UpdateSystemPrompt) {
    conv.SetSystemPrompt("First prompt");
    conv.AddMessage(Message::Role::User, "Hello");

    EXPECT_EQ(2, conv.GetMessageCount());

    conv.SetSystemPrompt("Updated prompt");

    // Should still have 2 messages (system updated, not added)
    EXPECT_EQ(2, conv.GetMessageCount());

    const auto& history = conv.GetHistory();
    EXPECT_EQ(Message::Role::System, history[0].role);
    EXPECT_EQ("Updated prompt", history[0].content);
    EXPECT_EQ("Hello", history[1].content);
}

TEST_F(ConversationTest, GetSystemPrompt) {
    EXPECT_EQ("", conv.GetSystemPrompt());

    conv.SetSystemPrompt("Test prompt");
    EXPECT_EQ("Test prompt", conv.GetSystemPrompt());
}

TEST_F(ConversationTest, GetTotalTokens) {
    conv.AddMessage(Message::Role::User, "Hello", 5);
    conv.AddMessage(Message::Role::Assistant, "Hi there", 8);
    conv.AddMessage(Message::Role::User, "How are you?", 12);

    EXPECT_EQ(25, conv.GetTotalTokens());
}

TEST_F(ConversationTest, GetMessageCount) {
    EXPECT_EQ(0, conv.GetMessageCount());

    conv.AddMessage(Message::Role::User, "First");
    EXPECT_EQ(1, conv.GetMessageCount());

    conv.AddMessage(Message::Role::Assistant, "Second");
    EXPECT_EQ(2, conv.GetMessageCount());
}

TEST_F(ConversationTest, ToApiFormat) {
    conv.SetSystemPrompt("You are helpful");
    conv.AddMessage(Message::Role::User, "Hello");
    conv.AddMessage(Message::Role::Assistant, "Hi there!");

    nlohmann::json apiFormat = conv.ToApiFormat();

    ASSERT_TRUE(apiFormat.is_array());
    EXPECT_EQ(3, apiFormat.size());

    // Check system message
    EXPECT_EQ("system", apiFormat[0]["role"]);
    EXPECT_EQ("You are helpful", apiFormat[0]["content"]);

    // Check user message
    EXPECT_EQ("user", apiFormat[1]["role"]);
    EXPECT_EQ("Hello", apiFormat[1]["content"]);

    // Check assistant message
    EXPECT_EQ("assistant", apiFormat[2]["role"]);
    EXPECT_EQ("Hi there!", apiFormat[2]["content"]);
}

TEST_F(ConversationTest, EstimateTokens) {
    // Empty string
    EXPECT_EQ(0, Conversation::EstimateTokens(""));

    // Short string: "Hello" = 5 chars / 4 = 1 token
    EXPECT_EQ(1, Conversation::EstimateTokens("Hello"));

    // Longer string: "This is a test message" = 22 chars / 4 = 5 tokens
    EXPECT_EQ(5, Conversation::EstimateTokens("This is a test message"));

    // Exactly 100 chars should be 25 tokens
    std::string longString(100, 'a');
    EXPECT_EQ(25, Conversation::EstimateTokens(longString));
}

// === Integration Test ===

TEST_F(ConversationTest, FullConversationFlow) {
    // Set system prompt
    conv.SetSystemPrompt("You are a helpful coding assistant");

    // Add user question
    conv.AddMessage(Message::Role::User, "How do I write a for loop in C++?", 50);

    // Add assistant response
    conv.AddMessage(Message::Role::Assistant,
                    "A for loop in C++ has this syntax: for(init; condition; increment) { body }",
                    100);

    // Add follow-up question
    conv.AddMessage(Message::Role::User, "Can you show an example?", 30);

    // Verify state
    EXPECT_EQ(4, conv.GetMessageCount());
    EXPECT_EQ(188, conv.GetTotalTokens()); // 8 (system auto-estimated) + 50 + 100 + 30

    // Get last 2 messages for context window
    auto recent = conv.GetLastN(2);
    EXPECT_EQ(2, recent.size());

    // Convert to API format
    auto apiJson = conv.ToApiFormat();
    EXPECT_EQ(4, apiJson.size());

    // Verify API format structure
    for (const auto& msg : apiJson) {
        EXPECT_TRUE(msg.contains("role"));
        EXPECT_TRUE(msg.contains("content"));
    }
}

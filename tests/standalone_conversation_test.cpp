// C:\FarfadetsCorp\AgentSmith\tests\standalone_conversation_test.cpp
// Standalone test that can be compiled independently

#include "agent/conversation.h"
#include <iostream>
#include <cassert>

using namespace smith::agent;

void TestRoleConversion() {
    std::cout << "Testing role conversion..." << std::endl;

    assert(std::string(Message::RoleToString(Message::Role::System)) == "system");
    assert(std::string(Message::RoleToString(Message::Role::User)) == "user");
    assert(std::string(Message::RoleToString(Message::Role::Assistant)) == "assistant");

    assert(Message::StringToRole("system") == Message::Role::System);
    assert(Message::StringToRole("user") == Message::Role::User);
    assert(Message::StringToRole("assistant") == Message::Role::Assistant);

    std::cout << "  Role conversion: PASSED" << std::endl;
}

void TestMessageCreation() {
    std::cout << "Testing message creation..." << std::endl;

    Message msg(Message::Role::User, "Hello", 10);
    assert(msg.role == Message::Role::User);
    assert(msg.content == "Hello");
    assert(msg.tokenCount == 10);

    std::cout << "  Message creation: PASSED" << std::endl;
}

void TestConversationBasics() {
    std::cout << "Testing conversation basics..." << std::endl;

    Conversation conv;

    // Test adding messages
    conv.AddMessage(Message::Role::User, "Hello");
    assert(conv.GetMessageCount() == 1);

    conv.AddMessage(Message::Role::Assistant, "Hi there!");
    assert(conv.GetMessageCount() == 2);

    // Test history
    const auto& history = conv.GetHistory();
    assert(history.size() == 2);
    assert(history[0].content == "Hello");
    assert(history[1].content == "Hi there!");

    std::cout << "  Conversation basics: PASSED" << std::endl;
}

void TestSystemPrompt() {
    std::cout << "Testing system prompt..." << std::endl;

    Conversation conv;

    // Set system prompt
    conv.SetSystemPrompt("You are a helpful assistant");
    assert(conv.GetMessageCount() == 1);
    assert(conv.GetSystemPrompt() == "You are a helpful assistant");

    const auto& history = conv.GetHistory();
    assert(history[0].role == Message::Role::System);
    assert(history[0].content == "You are a helpful assistant");

    // Add user message
    conv.AddMessage(Message::Role::User, "Hello");
    assert(conv.GetMessageCount() == 2);

    // Update system prompt
    conv.SetSystemPrompt("Updated prompt");
    assert(conv.GetMessageCount() == 2); // Should still be 2
    assert(history[0].content == "Updated prompt");

    std::cout << "  System prompt: PASSED" << std::endl;
}

void TestGetLastN() {
    std::cout << "Testing GetLastN..." << std::endl;

    Conversation conv;
    conv.AddMessage(Message::Role::User, "First");
    conv.AddMessage(Message::Role::Assistant, "Second");
    conv.AddMessage(Message::Role::User, "Third");
    conv.AddMessage(Message::Role::Assistant, "Fourth");

    auto last2 = conv.GetLastN(2);
    assert(last2.size() == 2);
    assert(last2[0].content == "Third");
    assert(last2[1].content == "Fourth");

    auto last10 = conv.GetLastN(10);
    assert(last10.size() == 4); // Only 4 messages total

    auto last0 = conv.GetLastN(0);
    assert(last0.size() == 0);

    std::cout << "  GetLastN: PASSED" << std::endl;
}

void TestTokenCounting() {
    std::cout << "Testing token counting..." << std::endl;

    Conversation conv;
    conv.AddMessage(Message::Role::User, "Hello", 5);
    conv.AddMessage(Message::Role::Assistant, "Hi there", 8);

    assert(conv.GetTotalTokens() == 13);

    // Test estimation
    assert(Conversation::EstimateTokens("") == 0);
    assert(Conversation::EstimateTokens("Hello") == 1); // 5 chars / 4 = 1

    std::string longString(100, 'a');
    assert(Conversation::EstimateTokens(longString) == 25); // 100 / 4 = 25

    std::cout << "  Token counting: PASSED" << std::endl;
}

void TestApiFormat() {
    std::cout << "Testing API format..." << std::endl;

    Conversation conv;
    conv.SetSystemPrompt("You are helpful");
    conv.AddMessage(Message::Role::User, "Hello");
    conv.AddMessage(Message::Role::Assistant, "Hi!");

    nlohmann::json apiFormat = conv.ToApiFormat();

    assert(apiFormat.is_array());
    assert(apiFormat.size() == 3);

    assert(apiFormat[0]["role"] == "system");
    assert(apiFormat[0]["content"] == "You are helpful");

    assert(apiFormat[1]["role"] == "user");
    assert(apiFormat[1]["content"] == "Hello");

    assert(apiFormat[2]["role"] == "assistant");
    assert(apiFormat[2]["content"] == "Hi!");

    std::cout << "  API format: PASSED" << std::endl;
}

void TestClear() {
    std::cout << "Testing clear..." << std::endl;

    Conversation conv;
    conv.AddMessage(Message::Role::User, "Hello");
    conv.AddMessage(Message::Role::Assistant, "Hi");

    assert(conv.GetMessageCount() == 2);

    conv.Clear();

    assert(conv.GetMessageCount() == 0);
    assert(conv.GetTotalTokens() == 0);
    assert(conv.GetSystemPrompt() == "");

    std::cout << "  Clear: PASSED" << std::endl;
}

int main() {
    std::cout << "=== Running Conversation Model Tests ===" << std::endl << std::endl;

    try {
        TestRoleConversion();
        TestMessageCreation();
        TestConversationBasics();
        TestSystemPrompt();
        TestGetLastN();
        TestTokenCounting();
        TestApiFormat();
        TestClear();

        std::cout << std::endl << "=== ALL TESTS PASSED ===" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "TEST FAILED: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "TEST FAILED: Unknown exception" << std::endl;
        return 1;
    }
}

// C:\FarfadetsCorp\AgentSmith\src\agent\agent.cpp

#include "agent/agent.h"
#include "logging/logger.h"
#include "logging/log_macros.h"
#include <sstream>
#include <iomanip>
#include <ctime>

namespace smith::agent {

// === Factory Methods ===

Agent Agent::Create(AgentType type, const std::string& name, const std::string& workingDir) {
    Agent agent;

    // Generate unique ID based on timestamp
    auto now = std::chrono::system_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    std::ostringstream oss;
    oss << "agent_" << std::hex << std::setw(12) << std::setfill('0') << ms;
    agent.id = oss.str();

    agent.name = name;
    agent.type = type;
    agent.status = AgentStatus::Idle;
    agent.workingDirectory = workingDir;
    agent.createdTime = now;
    agent.lastActivityTime = now;

    // Set default command based on type
    switch (type) {
        case AgentType::ClaudeCode:
            agent.command = "claude-code";
            break;
        case AgentType::Cursor:
            agent.command = "cursor";
            break;
        case AgentType::Grok:
            agent.apiEndpoint = "https://api.x.ai/v1/chat/completions";
            agent.modelId = "grok-2-latest";
            break;
        case AgentType::ChatGPT:
            agent.apiEndpoint = "https://api.openai.com/v1/chat/completions";
            agent.modelId = "gpt-4-turbo";
            break;
        case AgentType::Custom:
            // User must configure
            break;
    }

    SMITH_DEBUG(logging::Category::Agent, "Created agent: id={}, name={}, type={}",
                agent.id, name, static_cast<int>(type));

    return agent;
}

Agent Agent::FromConfig(const nlohmann::json& config) {
    Agent agent;

    try {
        // Required fields
        agent.id = config.value("id", "");
        agent.name = config.value("name", "Agent");

        // Parse type
        std::string typeStr = config.value("type", "ClaudeCode");
        if (typeStr == "ClaudeCode") agent.type = AgentType::ClaudeCode;
        else if (typeStr == "Grok") agent.type = AgentType::Grok;
        else if (typeStr == "ChatGPT") agent.type = AgentType::ChatGPT;
        else if (typeStr == "Cursor") agent.type = AgentType::Cursor;
        else agent.type = AgentType::Custom;

        // Process information
        agent.workingDirectory = config.value("workingDirectory", "");
        agent.command = config.value("command", "");

        if (config.contains("args") && config["args"].is_array()) {
            for (const auto& arg : config["args"]) {
                agent.args.push_back(arg.get<std::string>());
            }
        }

        if (config.contains("environment") && config["environment"].is_object()) {
            for (auto& [key, value] : config["environment"].items()) {
                agent.environment[key] = value.get<std::string>();
            }
        }

        // API configuration
        agent.apiKey = config.value("apiKey", "");
        agent.apiEndpoint = config.value("apiEndpoint", "");
        agent.modelId = config.value("modelId", "");

        if (config.contains("apiParams") && config["apiParams"].is_object()) {
            for (auto& [key, value] : config["apiParams"].items()) {
                agent.apiParams[key] = value.get<std::string>();
            }
        }

        // Configuration
        agent.autoRestart = config.value("autoRestart", false);
        agent.maxRestarts = config.value("maxRestarts", 3);
        agent.logOutput = config.value("logOutput", true);

        // Initialize timing
        agent.createdTime = std::chrono::system_clock::now();
        agent.lastActivityTime = agent.createdTime;

        SMITH_DEBUG(logging::Category::Agent, "Loaded agent from config: id={}, name={}",
                    agent.id, agent.name);

    } catch (const std::exception& e) {
        SMITH_ERROR(logging::Category::Agent, "Failed to parse agent config: {}", e.what());
        throw;
    }

    return agent;
}

nlohmann::json Agent::ToConfig() const {
    nlohmann::json config;

    // Basic info
    config["id"] = id;
    config["name"] = name;

    // Type
    switch (type) {
        case AgentType::ClaudeCode: config["type"] = "ClaudeCode"; break;
        case AgentType::Grok:       config["type"] = "Grok"; break;
        case AgentType::ChatGPT:    config["type"] = "ChatGPT"; break;
        case AgentType::Cursor:     config["type"] = "Cursor"; break;
        case AgentType::Custom:     config["type"] = "Custom"; break;
    }

    // Process information
    if (!workingDirectory.empty()) {
        config["workingDirectory"] = workingDirectory;
    }
    if (!command.empty()) {
        config["command"] = command;
    }
    if (!args.empty()) {
        config["args"] = args;
    }
    if (!environment.empty()) {
        config["environment"] = environment;
    }

    // API configuration
    if (!apiKey.empty()) {
        config["apiKey"] = apiKey;
    }
    if (!apiEndpoint.empty()) {
        config["apiEndpoint"] = apiEndpoint;
    }
    if (!modelId.empty()) {
        config["modelId"] = modelId;
    }
    if (!apiParams.empty()) {
        config["apiParams"] = apiParams;
    }

    // Configuration
    config["autoRestart"] = autoRestart;
    config["maxRestarts"] = maxRestarts;
    config["logOutput"] = logOutput;

    return config;
}

} // namespace smith::agent

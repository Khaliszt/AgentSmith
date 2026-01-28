// C:\FarfadetsCorp\AgentSmith\src\agent\providers\claude_provider.cpp

#include "agent/providers/claude_provider.h"
#include "agent/agent.h"
#include "logging/logger.h"
#include "logging/log_macros.h"
#include <algorithm>

namespace smith::agent {

// Static regex patterns
const std::regex ClaudeAgentProvider::s_costPattern(R"(Session cost:\s*\$(\d+\.?\d*))");
const std::regex ClaudeAgentProvider::s_tokenPattern(R"((\w+(?:\s+\w+)*)\s+tokens\s*[│|]\s*([\d,]+))");
const std::regex ClaudeAgentProvider::s_modelPattern(R"(Model:\s*(\S+))");
const std::regex ClaudeAgentProvider::s_thinkingPattern(R"(Thinking\.\.\.|<thinking>)");
const std::regex ClaudeAgentProvider::s_promptPattern(R"(^>\s*$)");
const std::regex ClaudeAgentProvider::s_toolCallPattern(R"(Calling tool:|Invoking function:)");

ClaudeAgentProvider::ClaudeAgentProvider() = default;

ClaudeAgentProvider::~ClaudeAgentProvider() {
    // Clear callbacks to prevent dangling pointer invocations
    if (m_terminal) {
        m_terminal->SetOutputCallback(nullptr);
    }
}

core::Result<void> ClaudeAgentProvider::Initialize(const Agent& agent) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    if (agent.type != AgentType::ClaudeCode) {
        return core::Result<void>(core::Error(core::Error::Code::InvalidArgument,
                                              "Agent type must be ClaudeCode"));
    }

    m_config = agent;
    m_status = AgentStatus::Idle;
    m_metrics.Reset();

    SMITH_INFO(logging::Category::Agent, "Initialized ClaudeAgentProvider for agent: {}",
               agent.name);

    return core::Ok();
}

core::Result<void> ClaudeAgentProvider::Start() {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    if (!m_terminal) {
        return core::Result<void>(core::Error(core::Error::Code::InvalidArgument,
                                              "Terminal not set - call SetTerminal() first"));
    }

    // Start the terminal (which launches the process)
    if (!m_terminal->IsRunning()) {
        // Terminal should handle launching the process
        SMITH_INFO(logging::Category::Agent, "Starting Claude Code agent: {}", m_config.name);
    }

    SetStatus(AgentStatus::Starting);

    // Request initial cost update after a delay
    m_lastCostRequest = std::chrono::system_clock::now() - std::chrono::seconds(10);

    return core::Ok();
}

core::Result<void> ClaudeAgentProvider::Stop() {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    if (m_terminal) {
        // Clear callbacks before stopping to prevent dangling invocations
        m_terminal->SetOutputCallback(nullptr);

        if (m_terminal->IsRunning()) {
            SMITH_INFO(logging::Category::Agent, "Stopping Claude Code agent: {}", m_config.name);
            // Terminal will handle process termination
        }
    }

    SetStatus(AgentStatus::Stopped);
    return core::Ok();
}

core::Result<void> ClaudeAgentProvider::SendInput(const std::string& input) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    if (!m_terminal) {
        return core::Result<void>(core::Error(core::Error::Code::InvalidArgument, "Terminal not set"));
    }

    if (!m_terminal->IsRunning()) {
        return core::Result<void>(core::Error(core::Error::Code::InvalidArgument, "Terminal not running"));
    }

    m_terminal->Write(input);
    SMITH_DEBUG(logging::Category::Agent, "Sent input to Claude: {} bytes", input.size());

    return core::Ok();
}

core::Result<void> ClaudeAgentProvider::SendCommand(const std::string& command) {
    if (command == "/cost" || command.rfind("/cost", 0) == 0) {
        return RequestCostUpdate();
    }
    return SendInput(command);
}

AgentStatus ClaudeAgentProvider::GetStatus() const {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    return m_status;
}

AgentMetrics ClaudeAgentProvider::GetMetrics() const {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    return m_metrics;
}

core::Result<AgentMetrics> ClaudeAgentProvider::UpdateMetrics() {
    // Trigger a /cost request
    auto result = RequestCostUpdate();
    if (!result) {
        return core::Result<AgentMetrics>(result.GetError());
    }

    // Return current metrics (will be updated when response arrives)
    AgentMetrics metrics = GetMetrics();
    return core::Result<AgentMetrics>(metrics);
}

void ClaudeAgentProvider::Update() {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    if (!m_terminal) return;

    // Check if terminal is still running
    if (!m_terminal->IsRunning() && m_status != AgentStatus::Stopped) {
        SetStatus(AgentStatus::Error);
        return;
    }

    // Periodic cost updates (every 30 seconds)
    auto now = std::chrono::system_clock::now();
    if (std::chrono::duration_cast<std::chrono::seconds>(now - m_lastCostRequest).count() > 30) {
        if (!m_waitingForCostResponse) {
            // Note: We can't call RequestCostUpdate here due to lock, so just mark it
            m_lastCostRequest = now;
        }
    }

    // Status detection would happen via output callback
    // For now, if we're running and not in error, assume Running
    if (m_status == AgentStatus::Starting && m_terminal->IsRunning()) {
        SetStatus(AgentStatus::Running);
    }
}

core::Result<void> ClaudeAgentProvider::RequestCostUpdate() {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    if (!m_terminal || !m_terminal->IsRunning()) {
        return core::Result<void>(core::Error(core::Error::Code::InvalidArgument,
                                              "Cannot request cost - terminal not running"));
    }

    SMITH_DEBUG(logging::Category::Agent, "Requesting cost update for agent: {}", m_config.name);

    m_terminal->Write("/cost\n");
    m_waitingForCostResponse = true;
    m_lastCostRequest = std::chrono::system_clock::now();

    return core::Ok();
}

void ClaudeAgentProvider::SetTerminal(std::shared_ptr<terminal::ITerminal> terminal) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    m_terminal = std::move(terminal);

    // Set up terminal callbacks
    if (m_terminal) {
        m_terminal->SetOutputCallback([this](const std::string& text) {
            std::lock_guard<std::recursive_mutex> lock(m_mutex);

            // Forward to our output callback
            InvokeOutputCallback(text, false);

            // Accumulate for parsing
            m_outputBuffer += text;

            // Parse for status and metrics
            DetectStatusFromOutput(text);

            if (m_waitingForCostResponse) {
                ParseCostOutput(m_outputBuffer);
            }

            // Keep buffer size reasonable
            if (m_outputBuffer.size() > 10000) {
                m_outputBuffer = m_outputBuffer.substr(m_outputBuffer.size() - 5000);
            }
        });
    }
}

void ClaudeAgentProvider::SetStatus(AgentStatus newStatus) {
    if (m_status != newStatus) {
        AgentStatus oldStatus = m_status;
        m_status = newStatus;
        InvokeStatusCallback(oldStatus, newStatus);

        SMITH_DEBUG(logging::Category::Agent, "Agent {} status: {} -> {}",
                    m_config.name, static_cast<int>(oldStatus), static_cast<int>(newStatus));
    }
}

void ClaudeAgentProvider::ParseCostOutput(const std::string& output) {
    std::smatch match;

    // Parse session cost
    if (std::regex_search(output, match, s_costPattern)) {
        try {
            m_metrics.sessionCost = std::stod(match[1].str());
            SMITH_DEBUG(logging::Category::Agent, "Parsed session cost: ${:.4f}",
                        m_metrics.sessionCost);
        } catch (...) {
            SMITH_WARN(logging::Category::Agent, "Failed to parse cost value: {}", match[1].str());
        }
    }

    // Parse model name
    if (std::regex_search(output, match, s_modelPattern)) {
        m_metrics.modelName = match[1].str();
        SMITH_DEBUG(logging::Category::Agent, "Parsed model: {}", m_metrics.modelName);
    }

    // Parse token counts
    std::string::const_iterator searchStart(output.cbegin());
    while (std::regex_search(searchStart, output.cend(), match, s_tokenPattern)) {
        std::string tokenType = match[1].str();
        std::string countStr = match[2].str();

        // Remove commas from count
        countStr.erase(std::remove(countStr.begin(), countStr.end(), ','), countStr.end());

        try {
            uint64_t count = std::stoull(countStr);

            // Normalize token type names
            std::transform(tokenType.begin(), tokenType.end(), tokenType.begin(), ::tolower);

            if (tokenType.find("input") != std::string::npos) {
                m_metrics.inputTokens = count;
            } else if (tokenType.find("output") != std::string::npos) {
                m_metrics.outputTokens = count;
            } else if (tokenType.find("cache creation") != std::string::npos) {
                m_metrics.cacheCreationTokens = count;
            } else if (tokenType.find("cache read") != std::string::npos) {
                m_metrics.cacheReadTokens = count;
            }

            SMITH_VERBOSE(logging::Category::Agent, "Parsed token count: {} = {}", tokenType, count);
        } catch (...) {
            SMITH_WARN(logging::Category::Agent, "Failed to parse token count: {}", countStr);
        }

        searchStart = match.suffix().first;
    }

    // Check if we got a complete cost response
    if (m_metrics.sessionCost > 0.0 || m_metrics.GetTotalTokens() > 0) {
        m_waitingForCostResponse = false;
        m_metrics.lastUpdateTime = std::chrono::system_clock::now();
        InvokeMetricsCallback(m_metrics);

        SMITH_INFO(logging::Category::Agent, "Agent {} metrics updated: tokens={}, cost=${:.4f}",
                   m_config.name, m_metrics.GetTotalTokens(), m_metrics.sessionCost);
    }
}

void ClaudeAgentProvider::DetectStatusFromOutput(const std::string& output) {
    // Detect thinking state
    if (std::regex_search(output, s_thinkingPattern)) {
        if (m_status != AgentStatus::Thinking) {
            SetStatus(AgentStatus::Thinking);
        }
        return;
    }

    // Detect tool calls
    if (std::regex_search(output, s_toolCallPattern)) {
        m_metrics.toolCalls++;
        if (m_status != AgentStatus::Running) {
            SetStatus(AgentStatus::Running);
        }
        return;
    }

    // Detect waiting for input (prompt)
    if (std::regex_search(output, s_promptPattern)) {
        if (m_status != AgentStatus::Waiting) {
            SetStatus(AgentStatus::Waiting);
        }
        return;
    }

    // Default: if we're starting or idle, move to running
    if (m_status == AgentStatus::Starting || m_status == AgentStatus::Idle) {
        if (m_terminal && m_terminal->IsRunning()) {
            SetStatus(AgentStatus::Running);
        }
    }
}

} // namespace smith::agent

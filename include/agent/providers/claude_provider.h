// C:\FarfadetsCorp\AgentSmith\include\agent\providers\claude_provider.h

#pragma once

#include "agent/agent_provider.h"
#include "terminal/terminal_interface.h"
#include <memory>
#include <string>
#include <regex>
#include <mutex>

namespace smith::agent {

/**
 * @brief Provider for Claude Code (terminal-based)
 *
 * This provider manages a Claude Code process via ConPTY terminal,
 * parses /cost command output to extract metrics, and detects
 * thinking/response states from terminal output.
 */
class ClaudeAgentProvider : public IAgentProvider {
public:
    ClaudeAgentProvider();
    ~ClaudeAgentProvider() override;

    // === IAgentProvider Implementation ===

    core::Result<void> Initialize(const Agent& agent) override;
    core::Result<void> Start() override;
    core::Result<void> Stop() override;

    core::Result<void> SendInput(const std::string& input) override;
    core::Result<void> SendCommand(const std::string& command) override;

    AgentStatus GetStatus() const override;
    AgentMetrics GetMetrics() const override;
    core::Result<AgentMetrics> UpdateMetrics() override;

    const char* GetProviderName() const override { return "ClaudeCodeProvider"; }
    AgentType GetAgentType() const override { return AgentType::ClaudeCode; }
    bool RequiresTerminal() const override { return true; }

    void Update() override;

    // === Claude-Specific Methods ===

    /**
     * @brief Requests cost/metrics update by sending /cost command
     *
     * @return Result indicating success or error
     */
    core::Result<void> RequestCostUpdate();

    /**
     * @brief Gets the associated terminal
     *
     * @return Terminal pointer (may be null)
     */
    terminal::ITerminal* GetTerminal() const { return m_terminal.get(); }

    /**
     * @brief Sets the terminal to use
     *
     * @param terminal Terminal instance
     */
    void SetTerminal(std::shared_ptr<terminal::ITerminal> terminal);

private:
    // === Parsing Methods ===

    /**
     * @brief Parses /cost command output to extract metrics
     *
     * Example output:
     * ```
     * Model: claude-sonnet-4.5
     * Session cost: $0.123
     * Input tokens │ 1,234
     * Output tokens │ 567
     * Cache creation tokens │ 89
     * Cache read tokens │ 12
     * ```
     *
     * @param output Output text from terminal
     */
    void ParseCostOutput(const std::string& output);

    /**
     * @brief Detects status from terminal output patterns
     *
     * Looks for patterns like:
     * - "Thinking..." -> Thinking
     * - ">" (prompt) -> Waiting
     * - Tool invocations -> Running
     *
     * @param output Output text
     */
    void DetectStatusFromOutput(const std::string& output);

    /**
     * @brief Updates status and invokes callback if changed
     *
     * @param newStatus New status
     */
    void SetStatus(AgentStatus newStatus);

    // === State ===
    Agent m_config;
    std::shared_ptr<terminal::ITerminal> m_terminal;
    AgentMetrics m_metrics;
    AgentStatus m_status = AgentStatus::Idle;
    mutable std::recursive_mutex m_mutex;  // Recursive for callback safety

    // === Parsing State ===
    std::string m_outputBuffer;         // Accumulates output for parsing
    bool m_waitingForCostResponse = false;
    std::chrono::system_clock::time_point m_lastCostRequest;

    // === Regex Patterns (compiled once) ===
    static const std::regex s_costPattern;
    static const std::regex s_tokenPattern;
    static const std::regex s_modelPattern;
    static const std::regex s_thinkingPattern;
    static const std::regex s_promptPattern;
    static const std::regex s_toolCallPattern;
};

} // namespace smith::agent

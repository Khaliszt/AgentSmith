// C:\FarfadetsCorp\AgentSmith\include\core\types.h

#pragma once

#include <string>
#include <cstdint>

namespace smith::core {

// Forward declarations
class AppBase;
class Application;

} // namespace smith::core

namespace smith::agent {

enum class AgentType {
    ClaudeCode,
    Grok,
    ChatGPT,
    Cursor,
    Custom
};

enum class AgentStatus {
    Idle,
    Starting,
    Running,
    Waiting,
    Thinking,
    Error,
    Stopped
};

// Forward declarations
struct Agent;
struct AgentMetrics;
class IAgentProvider;
class AgentTracker;

} // namespace smith::agent

namespace smith::terminal {

class ITerminal;

} // namespace smith::terminal

// C:\FarfadetsCorp\AgentSmith\include\terminal\terminal_interface.h

#pragma once

#include "core/result.h"
#include <string>
#include <functional>

namespace smith::terminal {

/**
 * @brief Callback for terminal output
 *
 * @param text Output text
 */
using OutputCallback = std::function<void(const std::string& text)>;

/**
 * @brief Callback for terminal resize events
 *
 * @param cols New column count
 * @param rows New row count
 */
using ResizeCallback = std::function<void(int cols, int rows)>;

/**
 * @brief Abstract interface for terminal implementations
 *
 * This interface abstracts terminal backends (ConPTY, WebView2+xterm.js, ImGui).
 * Phase 2 provides a minimal interface; Phase 3 will implement full backends.
 */
class ITerminal {
public:
    virtual ~ITerminal() = default;

    /**
     * @brief Writes text to the terminal (output)
     *
     * @param text Text to write
     */
    virtual void Write(const std::string& text) = 0;

    /**
     * @brief Checks if terminal is running
     *
     * @return True if process/terminal is active
     */
    virtual bool IsRunning() const = 0;

    /**
     * @brief Sets output callback
     *
     * @param callback Callback function
     */
    virtual void SetOutputCallback(OutputCallback callback) = 0;

    /**
     * @brief Resizes the terminal
     *
     * @param cols Column count
     * @param rows Row count
     */
    virtual void Resize(int cols, int rows) = 0;

    /**
     * @brief Clears the terminal display
     */
    virtual void Clear() = 0;

    /**
     * @brief Gets current size
     *
     * @param outCols Output: column count
     * @param outRows Output: row count
     */
    virtual void GetSize(int& outCols, int& outRows) const = 0;
};

} // namespace smith::terminal

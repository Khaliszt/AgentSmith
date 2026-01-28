// C:\FarfadetsCorp\AgentSmith\include\terminal\terminal_interface.h

#pragma once

#include "core/result.h"
#include <string>
#include <vector>
#include <functional>

#ifdef _WIN32
#include <Windows.h>
#endif

namespace smith::terminal {

// Forward declaration
struct TerminalTheme;

/**
 * @brief Callback for terminal output (raw data)
 *
 * @param data Output data buffer
 * @param length Data length
 */
using OutputCallbackRaw = std::function<void(const char* data, size_t length)>;

/**
 * @brief Callback for terminal output (string)
 *
 * Backward-compatible callback type. Implementations may convert raw output
 * to string for convenience.
 *
 * @param text Output text
 */
using OutputCallback = std::function<void(const std::string& text)>;

/**
 * @brief Callback for process exit
 *
 * @param exitCode Process exit code
 */
using ExitCallback = std::function<void(int exitCode)>;

/**
 * @brief Callback for terminal errors
 *
 * @param error Error message
 */
using ErrorCallback = std::function<void(const std::string& error)>;

/**
 * @brief Abstract interface for terminal implementations
 *
 * This interface abstracts terminal backends:
 * - ConPTYTerminal: Windows Console Pseudo Terminal for process I/O
 * - WebViewTerminal: WebView2 + xterm.js for rendering (primary)
 * - ImGuiTerminal: Custom ANSI parser + ImGui (fallback)
 *
 * All implementations must support process lifecycle, I/O, and basic display.
 * Advanced features (selection, scrolling, themes) have default implementations.
 */
class ITerminal {
public:
    virtual ~ITerminal() = default;

    // === Process Lifecycle ===

    /**
     * @brief Launch a process in the terminal
     *
     * Creates and attaches a process to the terminal with the specified
     * working directory and window size.
     *
     * @param command Command to execute
     * @param args Command arguments
     * @param workingDir Working directory for the process
     * @param cols Initial column count
     * @param rows Initial row count
     * @return True if process launched successfully
     */
    virtual bool Launch(const std::string& command,
                       const std::vector<std::string>& args,
                       const std::string& workingDir,
                       int cols, int rows) = 0;

    /**
     * @brief Terminate the running process
     *
     * Sends termination signal to the process. May be forceful depending
     * on implementation.
     */
    virtual void Terminate() = 0;

    /**
     * @brief Check if the process is still running
     *
     * @return True if process is active
     */
    virtual bool IsRunning() const = 0;

    /**
     * @brief Get the process exit code
     *
     * Only valid after process has exited (IsRunning() == false).
     *
     * @return Exit code, or -1 if process still running
     */
    virtual int GetExitCode() const = 0;

    /**
     * @brief Get the process ID
     *
     * @return Process ID, or -1 if not running
     */
    virtual int GetProcessId() const = 0;

    // === I/O ===

    /**
     * @brief Write text to the terminal (process input)
     *
     * @param text Text to write
     */
    virtual void Write(const std::string& text) = 0;

    /**
     * @brief Write raw data to the terminal
     *
     * @param data Data buffer
     * @param length Data length
     */
    virtual void Write(const char* data, size_t length) = 0;

    /**
     * @brief Set callback for terminal output
     *
     * Called when the process writes output. May be called from background thread.
     *
     * @param callback Output callback function
     */
    virtual void SetOutputCallback(OutputCallback callback) = 0;

    /**
     * @brief Set callback for process exit
     *
     * Called when the process terminates. May be called from background thread.
     *
     * @param callback Exit callback function
     */
    virtual void SetExitCallback(ExitCallback callback) = 0;

    /**
     * @brief Set callback for terminal errors
     *
     * Called when terminal errors occur (I/O errors, launch failures, etc.)
     *
     * @param callback Error callback function
     */
    virtual void SetErrorCallback(ErrorCallback callback) = 0;

    // === Display Size ===

    /**
     * @brief Resize the terminal display
     *
     * Updates both the visual display size and the PTY size (if applicable).
     *
     * @param cols New column count
     * @param rows New row count
     */
    virtual void Resize(int cols, int rows) = 0;

    /**
     * @brief Get current column count
     *
     * @return Column count
     */
    virtual int GetCols() const = 0;

    /**
     * @brief Get current row count
     *
     * @return Row count
     */
    virtual int GetRows() const = 0;

    /**
     * @brief Clear the terminal display
     *
     * Clears visible content. Scrollback may be preserved depending on implementation.
     */
    virtual void Clear() = 0;

    /**
     * @brief Get current size (legacy interface for backward compatibility)
     *
     * @param outCols Output: column count
     * @param outRows Output: row count
     */
    virtual void GetSize(int& outCols, int& outRows) const {
        outCols = GetCols();
        outRows = GetRows();
    }

    // === Rendering (backend-specific) ===

    /**
     * @brief Render the terminal (for ImGui-based implementations)
     *
     * Called every frame for terminals that render via ImGui.
     * WebView2 terminals ignore this.
     *
     * @param x X position
     * @param y Y position
     * @param width Width
     * @param height Height
     */
    virtual void Render(float x, float y, float width, float height) {
        (void)x; (void)y; (void)width; (void)height;
    }

    /**
     * @brief Get native window handle (for WebView2 implementations)
     *
     * Returns the HWND of the WebView2 window on Windows.
     * ImGui terminals return nullptr.
     *
     * @return Native window handle, or nullptr
     */
#ifdef _WIN32
    virtual HWND GetNativeHandle() const { return nullptr; }
#endif

    /**
     * @brief Set window bounds (for WebView2 implementations)
     *
     * Updates the position and size of the native window.
     *
     * @param x X position
     * @param y Y position
     * @param width Width
     * @param height Height
     */
    virtual void SetBounds(int x, int y, int width, int height) {
        (void)x; (void)y; (void)width; (void)height;
    }

    /**
     * @brief Set window visibility
     *
     * @param visible True to show, false to hide
     */
    virtual void SetVisible(bool visible) {
        (void)visible;
    }

    /**
     * @brief Focus the terminal
     *
     * Gives keyboard focus to the terminal window.
     */
    virtual void Focus() {}

    // === Capabilities ===

    /**
     * @brief Check if terminal uses native rendering
     *
     * @return True for WebView2 terminals, false for ImGui terminals
     */
    virtual bool HasNativeRendering() const { return false; }

    /**
     * @brief Check if terminal supports text selection
     *
     * @return True if selection is supported
     */
    virtual bool SupportsSelection() const { return false; }

    // === Clipboard Operations ===

    /**
     * @brief Get selected text
     *
     * @return Selected text, or empty string if no selection
     */
    virtual std::string GetSelectedText() const { return ""; }

    /**
     * @brief Clear current selection
     */
    virtual void ClearSelection() {}

    /**
     * @brief Select all text in terminal
     */
    virtual void SelectAll() {}

    /**
     * @brief Copy selected text to clipboard
     */
    virtual void Copy() {}

    /**
     * @brief Paste text from clipboard
     *
     * Default implementation writes text to terminal.
     *
     * @param text Text to paste
     */
    virtual void Paste(const std::string& text) { Write(text); }

    // === Scrolling ===

    /**
     * @brief Scroll up by lines
     *
     * @param lines Number of lines to scroll (positive = up, negative = down)
     */
    virtual void Scroll(int lines) { (void)lines; }

    /**
     * @brief Scroll to top of scrollback
     */
    virtual void ScrollToTop() {}

    /**
     * @brief Scroll to bottom (current output)
     */
    virtual void ScrollToBottom() {}

    /**
     * @brief Get current scroll position
     *
     * @return Scroll position in lines from bottom (0 = bottom)
     */
    virtual int GetScrollPosition() const { return 0; }

    /**
     * @brief Get total scrollback size
     *
     * @return Number of lines in scrollback buffer
     */
    virtual int GetScrollbackLines() const { return 0; }

    // === Theme ===

    /**
     * @brief Set terminal theme
     *
     * Updates colors for background, foreground, cursor, and ANSI colors.
     *
     * @param theme Terminal theme
     */
    virtual void SetTheme(const TerminalTheme& theme) = 0;

    /**
     * @brief Get current theme
     *
     * @return Current terminal theme
     */
    virtual const TerminalTheme& GetTheme() const = 0;

    // === Search (future expansion) ===

    /**
     * @brief Find text in terminal (optional feature)
     *
     * @param text Text to find
     * @param caseSensitive Case-sensitive search
     * @return True if found
     */
    virtual bool Find(const std::string& text, bool caseSensitive = false) {
        (void)text; (void)caseSensitive;
        return false;
    }

    /**
     * @brief Find next occurrence
     */
    virtual void FindNext() {}

    /**
     * @brief Find previous occurrence
     */
    virtual void FindPrevious() {}

    /**
     * @brief Clear find highlighting
     */
    virtual void ClearFind() {}
};

} // namespace smith::terminal

#pragma once

#include "types.h"
#include "conpty_terminal.h"
#include "terminal_buffer.h"
#include <imgui.h>
#include <memory>
#include <string>
#include <functional>

namespace AgentSmith {

//=============================================================================
// Agent Window
//
// Single agent's terminal view with embedded ConPTY terminal.
// Evolved from AgentPanel - now renders terminal output directly in ImGui.
//
// Features:
// - Embedded terminal with ANSI color support
// - Session tracking (duration, tokens, project)
// - Right-click context menu with agent info, settings
// - Fullscreen toggle
// - Auto-accept edits toggle
//=============================================================================

class AgentWindow {
public:
    // Callback types
    using AddAgentCallback = std::function<void()>;

    AgentWindow();
    ~AgentWindow();

    // Disable copy
    AgentWindow(const AgentWindow&) = delete;
    AgentWindow& operator=(const AgentWindow&) = delete;

    // Move support
    AgentWindow(AgentWindow&& other) noexcept;
    AgentWindow& operator=(AgentWindow&& other) noexcept;

    // Set callback for requesting "Add Agent" dialog
    void SetAddAgentCallback(AddAgentCallback callback) { m_addAgentCallback = callback; }

    // Set the agent this window displays
    void SetAgent(Agent* agent);
    Agent* GetAgent() { return m_agent; }
    const Agent* GetAgent() const { return m_agent; }

    // Render the window at the given screen region
    // Returns true if the window needs attention
    bool Render(const ImVec2& pos, const ImVec2& size);

    // Terminal management
    void LaunchTerminal();
    void TerminateTerminal();
    void RestartTerminal();
    bool IsTerminalRunning() const;

    // Focus management - unified entry point for all focus changes
    void Focus();
    void Unfocus();
    bool IsFocused() const { return m_isFocused; }

    // Handle keyboard input (when focused)
    void HandleKeyboardInput();

    // Copy/Paste support
    void CopyToClipboard();
    void PasteFromClipboard();

    // Fullscreen
    void SetFullscreen(bool fullscreen);
    bool IsFullscreen() const { return m_isFullscreen; }
    void ToggleFullscreen() { SetFullscreen(!m_isFullscreen); }

    // Get terminal dimensions in characters
    int GetTerminalCols() const;
    int GetTerminalRows() const;

private:
    // Rendering helpers
    void RenderTopOverlay(const ImVec2& pos, const ImVec2& size);
    void RenderBottomOverlay(const ImVec2& pos, const ImVec2& size);
    void RenderTerminalContent(const ImVec2& pos, const ImVec2& size);
    void RenderContextMenu();
    void RenderAgentInfoPopup();
    void RenderRemoveConfirmDialog();
    void RenderLaunchButton(const ImVec2& pos, const ImVec2& size);

    // Calculate terminal region (excluding overlays)
    ImVec4 GetTerminalRegion(const ImVec2& panelPos, const ImVec2& panelSize) const;

    // Format session duration as string
    std::string FormatSessionDuration() const;

    // Terminal output callback
    void OnTerminalOutput(const char* data, size_t length);

    // Update terminal size based on pixel dimensions
    void UpdateTerminalSize(int widthPx, int heightPx);

    // Apply terminal theme based on agent settings
    void ApplyTerminalTheme();

    // Agent data
    Agent* m_agent = nullptr;

    // Terminal components
    std::unique_ptr<ConPTYTerminal> m_terminal;
    std::unique_ptr<TerminalBuffer> m_buffer;

    // State
    bool m_isFocused = false;
    bool m_terminalLaunched = false;
    bool m_isFullscreen = false;
    bool m_showAgentInfo = false;
    bool m_pendingRemoval = false;

    // Character dimensions (monospace font)
    float m_charWidth = 8.0f;
    float m_charHeight = 16.0f;

    // Current terminal size in characters
    int m_terminalCols = 80;
    int m_terminalRows = 24;

    // Overlay dimensions
    float m_topOverlayHeight = 26.0f;
    float m_bottomOverlayHeight = 22.0f;
    float m_overlayOpacity = 0.85f;

    // Cached display strings
    std::string m_branchDisplay;
    float m_lastGitUpdate = 0.0f;

    // Input buffer for pending keyboard input
    std::string m_pendingInput;

    // Callbacks
    AddAgentCallback m_addAgentCallback;

    // Unique ID for ImGui widgets
    int m_windowId = 0;
    static int s_nextWindowId;
};

} // namespace AgentSmith

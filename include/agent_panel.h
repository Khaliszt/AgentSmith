#pragma once

#include "types.h"
#include "terminal_embed.h"
#include <imgui.h>

namespace AgentSmith {

//=============================================================================
// Agent Panel
//
// Renders a single agent panel in the grid. Contains:
// - Embedded terminal window
// - Top overlay bar with: status indicator, agent name, git branch
// - Bottom overlay bar with: working directory, stats
// - Context menu for agent actions
//=============================================================================

class AgentPanel {
public:
    AgentPanel();
    ~AgentPanel();
    
    // Set the agent this panel displays
    void SetAgent(Agent* agent);
    Agent* GetAgent() { return m_agent; }
    
    // Render the panel at the given screen region
    // Returns true if the panel needs attention (e.g., terminal finished)
    bool Render(const ImVec2& pos, const ImVec2& size, void* parent_window);
    
    // Terminal management
    void LaunchTerminal(void* parent_window);
    void TerminateTerminal();
    void ResizeTerminal(int x, int y, int width, int height);
    
    // Focus this panel's terminal
    void Focus();
    
    // Check if this panel is focused
    bool IsFocused() const { return m_is_focused; }
    
private:
    // Render the info overlay at the top of the panel
    void RenderTopOverlay(const ImVec2& pos, const ImVec2& size);
    
    // Render the info overlay at the bottom of the panel
    void RenderBottomOverlay(const ImVec2& pos, const ImVec2& size);
    
    // Render status indicator dot
    void RenderStatusIndicator(const ImVec2& pos, float radius);
    
    // Render context menu (right-click)
    void RenderContextMenu();
    
    // Calculate terminal region (excluding overlays)
    ImVec4 GetTerminalRegion(const ImVec2& panel_pos, const ImVec2& panel_size) const;
    
    Agent* m_agent = nullptr;
    TerminalEmbed m_terminal;
    
    bool m_is_focused = false;
    bool m_show_context_menu = false;
    bool m_terminal_launched = false;
    
    // Overlay settings
    float m_top_overlay_height = 26.0f;
    float m_bottom_overlay_height = 22.0f;
    float m_overlay_opacity = 0.85f;
    
    // Cached display strings (updated periodically)
    std::string m_branch_display;
    std::string m_status_display;
    float m_last_git_update = 0.0f;
};

} // namespace AgentSmith

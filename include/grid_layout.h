#pragma once

#include "types.h"
#include "agent_window.h"
#include <vector>
#include <memory>
#include <functional>

namespace AgentSmith {

//=============================================================================
// Grid Layout
//
// Manages the grid of agent windows. Handles:
// - Dynamic grid resizing (1x1 to 4x4)
// - Window positioning and sizing
// - Drag-and-drop reordering
// - Focus management
//=============================================================================

class GridLayout {
public:
    // Callback for when a slot requests "Add Agent"
    using AddAgentCallback = std::function<void()>;

    GridLayout();
    ~GridLayout();

    // Set callback for "Add Agent" requests from empty slots
    void SetAddAgentCallback(AddAgentCallback callback);

    // Configuration
    void SetGridSize(int rows, int cols);
    void SetConfig(const GridConfig& config);
    GridConfig& GetConfig() { return m_config; }

    // Update windows from agents
    void SyncWithAgents(std::vector<Agent>& agents);

    // Render the entire grid
    void Render(const ImVec2& area_pos, const ImVec2& area_size);

    // Focus management
    void FocusPanel(int row, int col);
    void FocusNext();
    void FocusPrev();
    AgentWindow* GetFocusedPanel();

    // Get window at position
    AgentWindow* GetPanelAt(int row, int col);

    // Get all windows as flat list (for InputManager)
    std::vector<AgentWindow*> GetAllWindows();

    // Get focused row/col
    int GetFocusedRow() const { return m_focused_row; }
    int GetFocusedCol() const { return m_focused_col; }

    // Handle keyboard shortcuts
    void HandleKeyboardShortcuts();

    // Check if any window has a fullscreen request
    AgentWindow* GetFullscreenWindow();

    // Layout presets
    void SetLayout1x1();
    void SetLayout1x2();
    void SetLayout2x1();
    void SetLayout2x2();
    void SetLayout2x3();
    void SetLayout3x3();

private:
    // Calculate panel bounds for a given grid position
    ImVec4 CalculatePanelBounds(int row, int col, const ImVec2& area_pos, const ImVec2& area_size);

    // Ensure we have the right number of panels
    void ResizePanelGrid();

    GridConfig m_config;

    // 2D array of windows [row][col]
    std::vector<std::vector<std::unique_ptr<AgentWindow>>> m_windows;

    // Focus tracking
    int m_focused_row = 0;
    int m_focused_col = 0;

    // Drag state for reordering
    bool m_dragging = false;
    int m_drag_from_row = -1;
    int m_drag_from_col = -1;

    // Callback
    AddAgentCallback m_addAgentCallback;
};

} // namespace AgentSmith

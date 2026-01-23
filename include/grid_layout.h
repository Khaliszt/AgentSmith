#pragma once

#include "types.h"
#include "agent_panel.h"
#include <vector>
#include <memory>

namespace AgentSmith {

//=============================================================================
// Grid Layout
//
// Manages the grid of agent panels. Handles:
// - Dynamic grid resizing (1x1 to 4x4)
// - Panel positioning and sizing
// - Drag-and-drop reordering
// - Focus management
//=============================================================================

class GridLayout {
public:
    GridLayout();
    ~GridLayout();
    
    // Configuration
    void SetGridSize(int rows, int cols);
    void SetConfig(const GridConfig& config);
    GridConfig& GetConfig() { return m_config; }
    
    // Update panels from agent manager
    void SyncWithAgents(std::vector<Agent>& agents);
    
    // Render the entire grid
    void Render(const ImVec2& area_pos, const ImVec2& area_size, void* parent_window);
    
    // Focus management
    void FocusPanel(int row, int col);
    void FocusNext();
    void FocusPrev();
    AgentPanel* GetFocusedPanel();
    
    // Get panel at position
    AgentPanel* GetPanelAt(int row, int col);
    
    // Handle keyboard shortcuts
    void HandleKeyboardShortcuts();
    
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
    
    // 2D array of panels [row][col]
    std::vector<std::vector<std::unique_ptr<AgentPanel>>> m_panels;
    
    // Focus tracking
    int m_focused_row = 0;
    int m_focused_col = 0;
    
    // Drag state for reordering
    bool m_dragging = false;
    int m_drag_from_row = -1;
    int m_drag_from_col = -1;
};

} // namespace AgentSmith

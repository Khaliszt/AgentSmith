#include "grid_layout.h"
#include <imgui.h>
#include <algorithm>

namespace AgentSmith {

GridLayout::GridLayout() {
    ResizePanelGrid();
}

GridLayout::~GridLayout() = default;

void GridLayout::SetGridSize(int rows, int cols) {
    m_config.rows = std::clamp(rows, 1, 4);
    m_config.cols = std::clamp(cols, 1, 4);
    ResizePanelGrid();
}

void GridLayout::SetConfig(const GridConfig& config) {
    m_config = config;
    m_config.rows = std::clamp(m_config.rows, 1, 4);
    m_config.cols = std::clamp(m_config.cols, 1, 4);
    ResizePanelGrid();
}

void GridLayout::ResizePanelGrid() {
    // Resize rows
    m_panels.resize(m_config.rows);
    
    // Resize columns in each row
    for (int r = 0; r < m_config.rows; r++) {
        int old_size = (int)m_panels[r].size();
        m_panels[r].resize(m_config.cols);
        
        // Create new panels if needed
        for (int c = old_size; c < m_config.cols; c++) {
            m_panels[r][c] = std::make_unique<AgentPanel>();
        }
    }
    
    // Clamp focus to valid range
    m_focused_row = std::clamp(m_focused_row, 0, m_config.rows - 1);
    m_focused_col = std::clamp(m_focused_col, 0, m_config.cols - 1);
}

void GridLayout::SyncWithAgents(std::vector<Agent>& agents) {
    // Assign agents to panels in order
    size_t agent_idx = 0;
    
    for (int r = 0; r < m_config.rows && agent_idx < agents.size(); r++) {
        for (int c = 0; c < m_config.cols && agent_idx < agents.size(); c++) {
            m_panels[r][c]->SetAgent(&agents[agent_idx]);
            agents[agent_idx].grid_row = r;
            agents[agent_idx].grid_col = c;
            agent_idx++;
        }
    }
    
    // Clear remaining panels
    for (int r = 0; r < m_config.rows; r++) {
        for (int c = 0; c < m_config.cols; c++) {
            size_t panel_idx = r * m_config.cols + c;
            if (panel_idx >= agents.size()) {
                m_panels[r][c]->SetAgent(nullptr);
            }
        }
    }
}

ImVec4 GridLayout::CalculatePanelBounds(int row, int col, const ImVec2& area_pos, const ImVec2& area_size) {
    float panel_width = (area_size.x - (m_config.cols + 1) * m_config.panel_padding) / m_config.cols;
    float panel_height = (area_size.y - (m_config.rows + 1) * m_config.panel_padding) / m_config.rows;
    
    float x = area_pos.x + m_config.panel_padding + col * (panel_width + m_config.panel_padding);
    float y = area_pos.y + m_config.panel_padding + row * (panel_height + m_config.panel_padding);
    
    return ImVec4(x, y, panel_width, panel_height);
}

void GridLayout::Render(const ImVec2& area_pos, const ImVec2& area_size, void* parent_window) {
    ImDrawList* draw_list = ImGui::GetBackgroundDrawList();
    
    // Draw background
    draw_list->AddRectFilled(
        area_pos,
        ImVec2(area_pos.x + area_size.x, area_pos.y + area_size.y),
        IM_COL32(20, 20, 25, 255)
    );
    
    // Render each panel
    for (int r = 0; r < m_config.rows; r++) {
        for (int c = 0; c < m_config.cols; c++) {
            ImVec4 bounds = CalculatePanelBounds(r, c, area_pos, area_size);
            ImVec2 pos(bounds.x, bounds.y);
            ImVec2 size(bounds.z, bounds.w);
            
            // Check if this panel is focused
            bool is_focused = (r == m_focused_row && c == m_focused_col);
            
            // Draw panel border
            ImU32 border_color = is_focused ? IM_COL32(60, 180, 90, 255) : IM_COL32(50, 50, 55, 255);
            float border_thickness = is_focused ? 2.0f : 1.0f;
            
            draw_list->AddRect(
                pos,
                ImVec2(pos.x + size.x, pos.y + size.y),
                border_color,
                3.0f,
                0,
                border_thickness
            );
            
            // Render panel content
            m_panels[r][c]->Render(pos, size, parent_window);
            
            // Handle click to focus
            if (ImGui::IsMouseClicked(0)) {
                ImVec2 mouse = ImGui::GetMousePos();
                if (mouse.x >= pos.x && mouse.x <= pos.x + size.x &&
                    mouse.y >= pos.y && mouse.y <= pos.y + size.y) {
                    m_focused_row = r;
                    m_focused_col = c;
                }
            }
        }
    }
}

void GridLayout::FocusPanel(int row, int col) {
    m_focused_row = std::clamp(row, 0, m_config.rows - 1);
    m_focused_col = std::clamp(col, 0, m_config.cols - 1);
    
    if (auto* panel = GetFocusedPanel()) {
        panel->Focus();
    }
}

void GridLayout::FocusNext() {
    m_focused_col++;
    if (m_focused_col >= m_config.cols) {
        m_focused_col = 0;
        m_focused_row++;
        if (m_focused_row >= m_config.rows) {
            m_focused_row = 0;
        }
    }
    
    if (auto* panel = GetFocusedPanel()) {
        panel->Focus();
    }
}

void GridLayout::FocusPrev() {
    m_focused_col--;
    if (m_focused_col < 0) {
        m_focused_col = m_config.cols - 1;
        m_focused_row--;
        if (m_focused_row < 0) {
            m_focused_row = m_config.rows - 1;
        }
    }
    
    if (auto* panel = GetFocusedPanel()) {
        panel->Focus();
    }
}

AgentPanel* GridLayout::GetFocusedPanel() {
    if (m_focused_row >= 0 && m_focused_row < m_config.rows &&
        m_focused_col >= 0 && m_focused_col < m_config.cols) {
        return m_panels[m_focused_row][m_focused_col].get();
    }
    return nullptr;
}

AgentPanel* GridLayout::GetPanelAt(int row, int col) {
    if (row >= 0 && row < m_config.rows && col >= 0 && col < m_config.cols) {
        return m_panels[row][col].get();
    }
    return nullptr;
}

void GridLayout::HandleKeyboardShortcuts() {
    // Arrow keys to navigate panels (when Ctrl is held)
    if (ImGui::GetIO().KeyCtrl) {
        if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow)) {
            m_focused_col = std::max(0, m_focused_col - 1);
        }
        if (ImGui::IsKeyPressed(ImGuiKey_RightArrow)) {
            m_focused_col = std::min(m_config.cols - 1, m_focused_col + 1);
        }
        if (ImGui::IsKeyPressed(ImGuiKey_UpArrow)) {
            m_focused_row = std::max(0, m_focused_row - 1);
        }
        if (ImGui::IsKeyPressed(ImGuiKey_DownArrow)) {
            m_focused_row = std::min(m_config.rows - 1, m_focused_row + 1);
        }
    }
    
    // Tab to cycle through panels
    if (ImGui::IsKeyPressed(ImGuiKey_Tab) && ImGui::GetIO().KeyCtrl) {
        if (ImGui::GetIO().KeyShift) {
            FocusPrev();
        } else {
            FocusNext();
        }
    }
    
    // Number keys 1-9 to jump to panel
    for (int i = 0; i < 9; i++) {
        if (ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed((ImGuiKey)(ImGuiKey_1 + i))) {
            int row = i / m_config.cols;
            int col = i % m_config.cols;
            if (row < m_config.rows) {
                FocusPanel(row, col);
            }
        }
    }
}

void GridLayout::SetLayout1x1() { SetGridSize(1, 1); }
void GridLayout::SetLayout1x2() { SetGridSize(1, 2); }
void GridLayout::SetLayout2x1() { SetGridSize(2, 1); }
void GridLayout::SetLayout2x2() { SetGridSize(2, 2); }
void GridLayout::SetLayout2x3() { SetGridSize(2, 3); }
void GridLayout::SetLayout3x3() { SetGridSize(3, 3); }

} // namespace AgentSmith

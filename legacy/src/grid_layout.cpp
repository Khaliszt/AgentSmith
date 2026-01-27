#include "grid_layout.h"
#include "output_log.h"
#include <imgui.h>
#include <algorithm>

namespace AgentSmith {

GridLayout::GridLayout() {
    ResizePanelGrid();
}

GridLayout::~GridLayout() = default;

void GridLayout::SetAddAgentCallback(AddAgentCallback callback) {
    m_addAgentCallback = callback;

    // Set callback on all existing windows
    for (auto& row : m_windows) {
        for (auto& window : row) {
            window->SetAddAgentCallback(callback);
        }
    }
}

void GridLayout::SetGridSize(int rows, int cols) {
    m_config.rows = (std::clamp)(rows, 1, 4);
    m_config.cols = (std::clamp)(cols, 1, 4);
    ResizePanelGrid();
}

void GridLayout::SetConfig(const GridConfig& config) {
    m_config = config;
    m_config.rows = (std::clamp)(m_config.rows, 1, 4);
    m_config.cols = (std::clamp)(m_config.cols, 1, 4);
    ResizePanelGrid();
}

void GridLayout::ResizePanelGrid() {
    // Resize rows
    m_windows.resize(m_config.rows);

    // Resize columns in each row
    for (int r = 0; r < m_config.rows; r++) {
        int old_size = static_cast<int>(m_windows[r].size());
        m_windows[r].resize(m_config.cols);

        // Create new windows if needed
        for (int c = old_size; c < m_config.cols; c++) {
            m_windows[r][c] = std::make_unique<AgentWindow>();
            // Set callback on new windows
            if (m_addAgentCallback) {
                m_windows[r][c]->SetAddAgentCallback(m_addAgentCallback);
            }
        }
    }

    // Clamp focus to valid range
    m_focused_row = (std::clamp)(m_focused_row, 0, m_config.rows - 1);
    m_focused_col = (std::clamp)(m_focused_col, 0, m_config.cols - 1);
}

void GridLayout::SyncWithAgents(const std::vector<Agent*>& agents) {
    // Assign agents to windows in order
    size_t agent_idx = 0;

    for (int r = 0; r < m_config.rows && agent_idx < agents.size(); r++) {
        for (int c = 0; c < m_config.cols && agent_idx < agents.size(); c++) {
            m_windows[r][c]->SetAgent(agents[agent_idx]);
            agents[agent_idx]->grid_row = r;
            agents[agent_idx]->grid_col = c;
            agent_idx++;
        }
    }

    // Clear remaining windows
    for (int r = 0; r < m_config.rows; r++) {
        for (int c = 0; c < m_config.cols; c++) {
            size_t panel_idx = r * m_config.cols + c;
            if (panel_idx >= agents.size()) {
                m_windows[r][c]->SetAgent(nullptr);
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

void GridLayout::Render(const ImVec2& area_pos, const ImVec2& area_size, const FColor& accent_color) {
    ImDrawList* draw_list = ImGui::GetBackgroundDrawList();

    // Check for fullscreen window
    AgentWindow* fullscreenWindow = GetFullscreenWindow();

    if (fullscreenWindow) {
        // Render only the fullscreen window
        draw_list->AddRectFilled(
            area_pos,
            ImVec2(area_pos.x + area_size.x, area_pos.y + area_size.y),
            IM_COL32(20, 20, 25, 255)
        );

        ImVec2 padding(m_config.panel_padding, m_config.panel_padding);
        ImVec2 pos(area_pos.x + padding.x, area_pos.y + padding.y);
        ImVec2 size(area_size.x - padding.x * 2, area_size.y - padding.y * 2);

        // Border (use accent color)
        draw_list->AddRect(
            pos,
            ImVec2(pos.x + size.x, pos.y + size.y),
            IM_COL32(
                static_cast<int>(accent_color.r * 255),
                static_cast<int>(accent_color.g * 255),
                static_cast<int>(accent_color.b * 255),
                255
            ),
            3.0f,
            0,
            2.0f
        );

        fullscreenWindow->Render(pos, size);
        return;
    }

    // Draw background
    draw_list->AddRectFilled(
        area_pos,
        ImVec2(area_pos.x + area_size.x, area_pos.y + area_size.y),
        IM_COL32(20, 20, 25, 255)
    );

    // Render each window
    for (int r = 0; r < m_config.rows; r++) {
        for (int c = 0; c < m_config.cols; c++) {
            ImVec4 bounds = CalculatePanelBounds(r, c, area_pos, area_size);
            ImVec2 pos(bounds.x, bounds.y);
            ImVec2 size(bounds.z, bounds.w);

            // Check if this window is focused
            bool is_focused = (r == m_focused_row && c == m_focused_col);

            // Draw panel border (use accent color for focused window)
            ImU32 border_color = is_focused
                ? IM_COL32(
                    static_cast<int>(accent_color.r * 255),
                    static_cast<int>(accent_color.g * 255),
                    static_cast<int>(accent_color.b * 255),
                    255
                  )
                : IM_COL32(50, 50, 55, 255);
            float border_thickness = is_focused ? 2.0f : 1.0f;

            draw_list->AddRect(
                pos,
                ImVec2(pos.x + size.x, pos.y + size.y),
                border_color,
                3.0f,
                0,
                border_thickness
            );

            // Render window content
            m_windows[r][c]->Render(pos, size);

            // Only handle clicks if ImGui doesn't want to capture the mouse
            // (i.e., no popup/modal/window is capturing input)
            bool imguiWantsMouse = ImGui::GetIO().WantCaptureMouse;

            // Handle click to focus
            if (!imguiWantsMouse && ImGui::IsMouseClicked(0)) {
                ImVec2 mouse = ImGui::GetMousePos();
                if (mouse.x >= pos.x && mouse.x <= pos.x + size.x &&
                    mouse.y >= pos.y && mouse.y <= pos.y + size.y) {
                    // Unfocus old window
                    if (m_focused_row != r || m_focused_col != c) {
                        m_windows[m_focused_row][m_focused_col]->Unfocus();
                    }
                    m_focused_row = r;
                    m_focused_col = c;
                    m_windows[r][c]->Focus();
                }
            }

            // Handle double-click for fullscreen
            if (!imguiWantsMouse && ImGui::IsMouseDoubleClicked(0)) {
                ImVec2 mouse = ImGui::GetMousePos();
                if (mouse.x >= pos.x && mouse.x <= pos.x + size.x &&
                    mouse.y >= pos.y && mouse.y <= pos.y + size.y) {
                    m_windows[r][c]->ToggleFullscreen();
                }
            }
        }
    }

    // Update focus state on focused window
    if (m_focused_row >= 0 && m_focused_row < m_config.rows &&
        m_focused_col >= 0 && m_focused_col < m_config.cols) {
        // Ensure focused window knows it's focused
        if (!m_windows[m_focused_row][m_focused_col]->IsFocused()) {
            m_windows[m_focused_row][m_focused_col]->Focus();
        }
    }
}

void GridLayout::FocusPanel(int row, int col) {
    int newRow = (std::clamp)(row, 0, m_config.rows - 1);
    int newCol = (std::clamp)(col, 0, m_config.cols - 1);

    // Only do something if focus actually changes
    if (newRow == m_focused_row && newCol == m_focused_col) {
        // Still make sure it's focused
        if (auto* window = GetFocusedPanel()) {
            window->Focus();
        }
        return;
    }

    // Unfocus old window
    if (m_focused_row >= 0 && m_focused_row < m_config.rows &&
        m_focused_col >= 0 && m_focused_col < m_config.cols) {
        m_windows[m_focused_row][m_focused_col]->Unfocus();
    }

    m_focused_row = newRow;
    m_focused_col = newCol;

    if (auto* window = GetFocusedPanel()) {
        window->Focus();
        LOG_DEBUG_SRC("Focused panel at (" + std::to_string(row) + "," + std::to_string(col) + ")", "GridLayout");
    }
}

void GridLayout::FocusNext() {
    // Unfocus current
    if (auto* current = GetFocusedPanel()) {
        current->Unfocus();
    }

    m_focused_col++;
    if (m_focused_col >= m_config.cols) {
        m_focused_col = 0;
        m_focused_row++;
        if (m_focused_row >= m_config.rows) {
            m_focused_row = 0;
        }
    }

    if (auto* window = GetFocusedPanel()) {
        window->Focus();
    }
}

void GridLayout::FocusPrev() {
    // Unfocus current
    if (auto* current = GetFocusedPanel()) {
        current->Unfocus();
    }

    m_focused_col--;
    if (m_focused_col < 0) {
        m_focused_col = m_config.cols - 1;
        m_focused_row--;
        if (m_focused_row < 0) {
            m_focused_row = m_config.rows - 1;
        }
    }

    if (auto* window = GetFocusedPanel()) {
        window->Focus();
    }
}

AgentWindow* GridLayout::GetFocusedPanel() {
    if (m_focused_row >= 0 && m_focused_row < m_config.rows &&
        m_focused_col >= 0 && m_focused_col < m_config.cols) {
        return m_windows[m_focused_row][m_focused_col].get();
    }
    return nullptr;
}

AgentWindow* GridLayout::GetPanelAt(int row, int col) {
    if (row >= 0 && row < m_config.rows && col >= 0 && col < m_config.cols) {
        return m_windows[row][col].get();
    }
    return nullptr;
}

std::vector<AgentWindow*> GridLayout::GetAllWindows() {
    std::vector<AgentWindow*> windows;
    for (int r = 0; r < m_config.rows; r++) {
        for (int c = 0; c < m_config.cols; c++) {
            windows.push_back(m_windows[r][c].get());
        }
    }
    return windows;
}

AgentWindow* GridLayout::GetFullscreenWindow() {
    for (int r = 0; r < m_config.rows; r++) {
        for (int c = 0; c < m_config.cols; c++) {
            if (m_windows[r][c]->IsFullscreen()) {
                return m_windows[r][c].get();
            }
        }
    }
    return nullptr;
}

void GridLayout::HandleKeyboardShortcuts() {
    // Arrow keys to navigate panels (when Ctrl is held)
    if (ImGui::GetIO().KeyCtrl) {
        if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow)) {
            if (auto* current = GetFocusedPanel()) current->Unfocus();
            m_focused_col = (std::max)(0, m_focused_col - 1);
            if (auto* window = GetFocusedPanel()) window->Focus();
        }
        if (ImGui::IsKeyPressed(ImGuiKey_RightArrow)) {
            if (auto* current = GetFocusedPanel()) current->Unfocus();
            m_focused_col = (std::min)(m_config.cols - 1, m_focused_col + 1);
            if (auto* window = GetFocusedPanel()) window->Focus();
        }
        if (ImGui::IsKeyPressed(ImGuiKey_UpArrow)) {
            if (auto* current = GetFocusedPanel()) current->Unfocus();
            m_focused_row = (std::max)(0, m_focused_row - 1);
            if (auto* window = GetFocusedPanel()) window->Focus();
        }
        if (ImGui::IsKeyPressed(ImGuiKey_DownArrow)) {
            if (auto* current = GetFocusedPanel()) current->Unfocus();
            m_focused_row = (std::min)(m_config.rows - 1, m_focused_row + 1);
            if (auto* window = GetFocusedPanel()) window->Focus();
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
        if (ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(static_cast<ImGuiKey>(ImGuiKey_1 + i))) {
            int row = i / m_config.cols;
            int col = i % m_config.cols;
            if (row < m_config.rows) {
                FocusPanel(row, col);
            }
        }
    }

    // Escape to exit fullscreen
    if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
        if (auto* fullscreen = GetFullscreenWindow()) {
            fullscreen->SetFullscreen(false);
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

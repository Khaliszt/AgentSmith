#include "agent_panel.h"
#include "git_utils.h"
#include <imgui.h>

namespace AgentSmith {

AgentPanel::AgentPanel() = default;
AgentPanel::~AgentPanel() {
    if (m_agent && m_terminal_launched) {
        m_terminal.Terminate(*m_agent);
    }
}

void AgentPanel::SetAgent(Agent* agent) {
    // Clean up old terminal if agent changes
    if (m_agent && m_agent != agent && m_terminal_launched) {
        m_terminal.Terminate(*m_agent);
        m_terminal_launched = false;
    }
    m_agent = agent;
}

bool AgentPanel::Render(const ImVec2& pos, const ImVec2& size, void* parent_window) {
    ImDrawList* draw_list = ImGui::GetBackgroundDrawList();
    
    // Panel background
    draw_list->AddRectFilled(
        pos,
        ImVec2(pos.x + size.x, pos.y + size.y),
        IM_COL32(25, 25, 30, 255),
        3.0f
    );
    
    if (!m_agent) {
        // Empty panel - show placeholder
        ImVec2 text_size = ImGui::CalcTextSize("Drop agent here or click + to add");
        ImVec2 text_pos(
            pos.x + (size.x - text_size.x) / 2,
            pos.y + (size.y - text_size.y) / 2
        );
        draw_list->AddText(text_pos, IM_COL32(80, 80, 85, 255), "Drop agent here or click + to add");
        return false;
    }
    
    // Update git info periodically
    float time = (float)ImGui::GetTime();
    if (time - m_last_git_update > 5.0f) {
        GitUtils::UpdateGitInfo(*m_agent);
        m_branch_display = GitUtils::FormatBranchDisplay(m_agent->git_info);
        m_last_git_update = time;
    }
    
    // Render overlays
    RenderTopOverlay(pos, size);
    RenderBottomOverlay(pos, size);
    
    // Calculate terminal region (between overlays)
    ImVec4 term_region = GetTerminalRegion(pos, size);
    
    // Terminal area background (darker)
    draw_list->AddRectFilled(
        ImVec2(term_region.x, term_region.y),
        ImVec2(term_region.x + term_region.z, term_region.y + term_region.w),
        IM_COL32(15, 15, 18, 255)
    );
    
    // If terminal not launched, show launch button
    if (!m_terminal_launched) {
        ImVec2 btn_size(120, 36);
        ImVec2 btn_pos(
            term_region.x + (term_region.z - btn_size.x) / 2,
            term_region.y + (term_region.w - btn_size.y) / 2
        );
        
        ImGui::SetCursorScreenPos(btn_pos);
        ImGui::PushID(m_agent->id.c_str());
        if (ImGui::Button("Launch Terminal", btn_size)) {
            LaunchTerminal(parent_window);
        }
        ImGui::PopID();
    } else {
        // Update terminal size/position
        ResizeTerminal(
            (int)term_region.x,
            (int)term_region.y,
            (int)term_region.z,
            (int)term_region.w
        );
    }
    
    // Handle right-click context menu
    if (ImGui::IsMouseClicked(1)) {
        ImVec2 mouse = ImGui::GetMousePos();
        if (mouse.x >= pos.x && mouse.x <= pos.x + size.x &&
            mouse.y >= pos.y && mouse.y <= pos.y + size.y) {
            m_show_context_menu = true;
            ImGui::OpenPopup(("AgentContext_" + m_agent->id).c_str());
        }
    }
    
    RenderContextMenu();
    
    return false;
}

void AgentPanel::RenderTopOverlay(const ImVec2& pos, const ImVec2& size) {
    ImDrawList* draw_list = ImGui::GetBackgroundDrawList();
    
    // Overlay background with gradient
    ImVec2 overlay_end(pos.x + size.x, pos.y + m_top_overlay_height);
    draw_list->AddRectFilledMultiColor(
        pos,
        overlay_end,
        IM_COL32(35, 35, 40, (int)(255 * m_overlay_opacity)),
        IM_COL32(35, 35, 40, (int)(255 * m_overlay_opacity)),
        IM_COL32(25, 25, 30, (int)(200 * m_overlay_opacity)),
        IM_COL32(25, 25, 30, (int)(200 * m_overlay_opacity))
    );
    
    float x_offset = pos.x + 8;
    float y_center = pos.y + m_top_overlay_height / 2;
    
    // Status indicator dot
    float dot_radius = 5.0f;
    float r, g, b;
    GetStatusColor(m_agent->status, r, g, b);
    
    draw_list->AddCircleFilled(
        ImVec2(x_offset + dot_radius, y_center),
        dot_radius,
        IM_COL32((int)(r * 255), (int)(g * 255), (int)(b * 255), 255)
    );
    x_offset += dot_radius * 2 + 8;
    
    // Agent name
    draw_list->AddText(
        ImVec2(x_offset, y_center - 7),
        IM_COL32(230, 230, 230, 255),
        m_agent->name.c_str()
    );
    x_offset += ImGui::CalcTextSize(m_agent->name.c_str()).x + 12;
    
    // Agent type badge
    const char* type_text = GetAgentTypeText(m_agent->type);
    ImVec2 badge_size = ImGui::CalcTextSize(type_text);
    badge_size.x += 8;
    badge_size.y += 4;
    
    draw_list->AddRectFilled(
        ImVec2(x_offset, y_center - badge_size.y / 2),
        ImVec2(x_offset + badge_size.x, y_center + badge_size.y / 2),
        IM_COL32(60, 60, 70, 255),
        3.0f
    );
    draw_list->AddText(
        ImVec2(x_offset + 4, y_center - 6),
        IM_COL32(180, 180, 180, 255),
        type_text
    );
    
    // Git branch (right-aligned)
    if (m_agent->git_info.is_git_repo && !m_branch_display.empty()) {
        ImVec2 branch_size = ImGui::CalcTextSize(m_branch_display.c_str());
        float branch_x = pos.x + size.x - branch_size.x - 10;
        
        // Branch icon (simplified git branch symbol)
        draw_list->AddText(
            ImVec2(branch_x - 16, y_center - 7),
            IM_COL32(120, 180, 120, 255),
            "\xef\x84\xa6"  // Git branch icon if font supports it, otherwise use text
        );
        
        draw_list->AddText(
            ImVec2(branch_x, y_center - 7),
            IM_COL32(120, 180, 120, 255),
            m_branch_display.c_str()
        );
    }
}

void AgentPanel::RenderBottomOverlay(const ImVec2& pos, const ImVec2& size) {
    ImDrawList* draw_list = ImGui::GetBackgroundDrawList();
    
    ImVec2 overlay_start(pos.x, pos.y + size.y - m_bottom_overlay_height);
    ImVec2 overlay_end(pos.x + size.x, pos.y + size.y);
    
    // Overlay background with gradient (inverted)
    draw_list->AddRectFilledMultiColor(
        overlay_start,
        overlay_end,
        IM_COL32(25, 25, 30, (int)(200 * m_overlay_opacity)),
        IM_COL32(25, 25, 30, (int)(200 * m_overlay_opacity)),
        IM_COL32(35, 35, 40, (int)(255 * m_overlay_opacity)),
        IM_COL32(35, 35, 40, (int)(255 * m_overlay_opacity))
    );
    
    float y_center = overlay_start.y + m_bottom_overlay_height / 2;
    
    // Working directory (left)
    std::string dir_display = m_agent->working_directory;
    if (dir_display.length() > 50) {
        dir_display = "..." + dir_display.substr(dir_display.length() - 47);
    }
    
    draw_list->AddText(
        ImVec2(pos.x + 8, y_center - 6),
        IM_COL32(140, 140, 145, 255),
        dir_display.c_str()
    );
    
    // Status text (right-aligned)
    const char* status_text = GetStatusText(m_agent->status);
    ImVec2 status_size = ImGui::CalcTextSize(status_text);
    
    float r, g, b;
    GetStatusColor(m_agent->status, r, g, b);
    
    draw_list->AddText(
        ImVec2(pos.x + size.x - status_size.x - 10, y_center - 6),
        IM_COL32((int)(r * 200), (int)(g * 200), (int)(b * 200), 255),
        status_text
    );
}

void AgentPanel::RenderContextMenu() {
    if (!m_agent) return;
    
    if (ImGui::BeginPopup(("AgentContext_" + m_agent->id).c_str())) {
        ImGui::Text("%s", m_agent->name.c_str());
        ImGui::Separator();
        
        if (!m_terminal_launched) {
            if (ImGui::MenuItem("Launch Terminal")) {
                // Will be handled in next render
            }
        } else {
            if (ImGui::MenuItem("Restart Terminal")) {
                TerminateTerminal();
            }
            if (ImGui::MenuItem("Stop Terminal")) {
                TerminateTerminal();
            }
        }
        
        ImGui::Separator();
        
        if (ImGui::MenuItem("Refresh Git Info")) {
            GitUtils::UpdateGitInfo(*m_agent);
            m_branch_display = GitUtils::FormatBranchDisplay(m_agent->git_info);
        }
        
        if (ImGui::MenuItem("Open Directory...")) {
            // TODO: Open file explorer
        }
        
        ImGui::Separator();
        
        if (ImGui::MenuItem("Remove Agent")) {
            // TODO: Signal removal to manager
        }
        
        ImGui::EndPopup();
    }
}

ImVec4 AgentPanel::GetTerminalRegion(const ImVec2& panel_pos, const ImVec2& panel_size) const {
    return ImVec4(
        panel_pos.x + 1,
        panel_pos.y + m_top_overlay_height,
        panel_size.x - 2,
        panel_size.y - m_top_overlay_height - m_bottom_overlay_height
    );
}

void AgentPanel::LaunchTerminal(void* parent_window) {
    if (!m_agent || m_terminal_launched) return;
    
    if (m_terminal.LaunchAndEmbed(*m_agent, parent_window)) {
        m_terminal_launched = true;
        m_agent->status = AgentStatus::Running;
        m_agent->started_at = std::chrono::system_clock::now();
    } else {
        m_agent->status = AgentStatus::Error;
        m_agent->status_message = "Failed to launch terminal";
    }
}

void AgentPanel::TerminateTerminal() {
    if (!m_agent || !m_terminal_launched) return;
    
    m_terminal.Terminate(*m_agent);
    m_terminal_launched = false;
    m_agent->status = AgentStatus::Stopped;
}

void AgentPanel::ResizeTerminal(int x, int y, int width, int height) {
    if (!m_agent || !m_terminal_launched) return;
    m_terminal.Resize(*m_agent, x, y, width, height);
}

void AgentPanel::Focus() {
    if (!m_agent || !m_terminal_launched) return;
    m_terminal.Focus(*m_agent);
    m_is_focused = true;
}

} // namespace AgentSmith

#include "agent_window.h"
#include "git_utils.h"
#include "output_log.h"
#include <imgui.h>
#include <algorithm>
#include <cstring>
#include <sstream>
#include <iomanip>

#ifdef PLATFORM_WINDOWS
#include <windows.h>
#endif

namespace AgentSmith {

// Static ID counter for unique ImGui widget IDs
int AgentWindow::s_nextWindowId = 0;

AgentWindow::AgentWindow() {
    m_buffer = std::make_unique<TerminalBuffer>(80, 24);
    m_windowId = s_nextWindowId++;
}

AgentWindow::~AgentWindow() {
    if (m_terminal && m_terminalLaunched) {
        m_terminal->Terminate();
    }
}

AgentWindow::AgentWindow(AgentWindow&& other) noexcept
    : m_agent(other.m_agent)
    , m_terminal(std::move(other.m_terminal))
    , m_buffer(std::move(other.m_buffer))
    , m_isFocused(other.m_isFocused)
    , m_terminalLaunched(other.m_terminalLaunched)
    , m_isFullscreen(other.m_isFullscreen)
    , m_showAgentInfo(other.m_showAgentInfo)
    , m_charWidth(other.m_charWidth)
    , m_charHeight(other.m_charHeight)
    , m_terminalCols(other.m_terminalCols)
    , m_terminalRows(other.m_terminalRows)
    , m_branchDisplay(std::move(other.m_branchDisplay))
    , m_lastGitUpdate(other.m_lastGitUpdate) {
    other.m_agent = nullptr;
    other.m_terminalLaunched = false;
}

AgentWindow& AgentWindow::operator=(AgentWindow&& other) noexcept {
    if (this != &other) {
        if (m_terminal && m_terminalLaunched) {
            m_terminal->Terminate();
        }

        m_agent = other.m_agent;
        m_terminal = std::move(other.m_terminal);
        m_buffer = std::move(other.m_buffer);
        m_isFocused = other.m_isFocused;
        m_terminalLaunched = other.m_terminalLaunched;
        m_isFullscreen = other.m_isFullscreen;
        m_showAgentInfo = other.m_showAgentInfo;
        m_charWidth = other.m_charWidth;
        m_charHeight = other.m_charHeight;
        m_terminalCols = other.m_terminalCols;
        m_terminalRows = other.m_terminalRows;
        m_branchDisplay = std::move(other.m_branchDisplay);
        m_lastGitUpdate = other.m_lastGitUpdate;

        other.m_agent = nullptr;
        other.m_terminalLaunched = false;
    }
    return *this;
}

void AgentWindow::SetAgent(Agent* agent) {
    if (m_agent == agent) return;

    // Cleanup old terminal
    if (m_agent && m_terminalLaunched) {
        m_terminal->Terminate();
        m_terminalLaunched = false;
    }

    m_agent = agent;
    m_branchDisplay.clear();
    m_lastGitUpdate = 0.0f;

    // Clear buffer for new agent
    if (m_buffer) {
        m_buffer->Clear();
    }
}

bool AgentWindow::Render(const ImVec2& pos, const ImVec2& size) {
    ImDrawList* drawList = ImGui::GetBackgroundDrawList();

    // Panel background
    drawList->AddRectFilled(
        pos,
        ImVec2(pos.x + size.x, pos.y + size.y),
        IM_COL32(25, 25, 30, 255),
        3.0f
    );

    // Create unique ID for this window's widgets
    ImGui::PushID(m_windowId);

    if (!m_agent) {
        // Empty panel placeholder
        ImVec2 textSize = ImGui::CalcTextSize("Right-click to add agent");
        ImVec2 textPos(
            pos.x + (size.x - textSize.x) / 2,
            pos.y + (size.y - textSize.y) / 2
        );
        drawList->AddText(textPos, IM_COL32(80, 80, 85, 255), "Right-click to add agent");

        // Handle right-click on empty slot
        if (ImGui::IsMouseClicked(1)) {
            ImVec2 mouse = ImGui::GetMousePos();
            if (mouse.x >= pos.x && mouse.x <= pos.x + size.x &&
                mouse.y >= pos.y && mouse.y <= pos.y + size.y) {
                ImGui::OpenPopup("EmptySlotContext");
            }
        }

        // Empty slot context menu
        if (ImGui::BeginPopup("EmptySlotContext")) {
            if (ImGui::MenuItem("Add Agent...")) {
                if (m_addAgentCallback) {
                    m_addAgentCallback();
                }
            }
            ImGui::EndPopup();
        }

        ImGui::PopID();
        return false;
    }

    // Update git info periodically
    float time = static_cast<float>(ImGui::GetTime());
    if (time - m_lastGitUpdate > 5.0f) {
        GitUtils::UpdateGitInfo(*m_agent);
        m_branchDisplay = GitUtils::FormatBranchDisplay(m_agent->git_info);
        m_lastGitUpdate = time;
    }

    // Render overlays
    RenderTopOverlay(pos, size);
    RenderBottomOverlay(pos, size);

    // Calculate terminal region
    ImVec4 termRegion = GetTerminalRegion(pos, size);

    // Terminal area background
    drawList->AddRectFilled(
        ImVec2(termRegion.x, termRegion.y),
        ImVec2(termRegion.x + termRegion.z, termRegion.y + termRegion.w),
        IM_COL32(15, 15, 18, 255)
    );

    if (!m_terminalLaunched) {
        RenderLaunchButton(ImVec2(termRegion.x, termRegion.y), ImVec2(termRegion.z, termRegion.w));
    } else {
        // Update terminal size if needed
        UpdateTerminalSize(static_cast<int>(termRegion.z), static_cast<int>(termRegion.w));

        // Render terminal content
        RenderTerminalContent(ImVec2(termRegion.x, termRegion.y), ImVec2(termRegion.z, termRegion.w));

        // Create an invisible button over the terminal area to capture keyboard focus
        ImGui::SetCursorScreenPos(ImVec2(termRegion.x, termRegion.y));
        ImGui::InvisibleButton("##TerminalCapture", ImVec2(termRegion.z, termRegion.w));

        // When this invisible button is clicked, we want focus
        if (ImGui::IsItemClicked(0)) {
            m_isFocused = true;
            ImGui::SetKeyboardFocusHere(-1);
        }

        // Handle keyboard input when focused
        if (m_isFocused && ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows)) {
            HandleKeyboardInput();
        } else if (m_isFocused) {
            // Still handle input if we're marked as focused
            HandleKeyboardInput();
        }
    }

    // Context menu on right-click
    if (ImGui::IsMouseClicked(1)) {
        ImVec2 mouse = ImGui::GetMousePos();
        if (mouse.x >= pos.x && mouse.x <= pos.x + size.x &&
            mouse.y >= pos.y && mouse.y <= pos.y + size.y) {
            ImGui::OpenPopup(("AgentContext_" + m_agent->id).c_str());
        }
    }

    RenderContextMenu();

    if (m_showAgentInfo) {
        RenderAgentInfoPopup();
    }

    // Check if process exited
    if (m_terminalLaunched && m_terminal && !m_terminal->IsRunning()) {
        m_agent->status = AgentStatus::Stopped;
        m_agent->needs_attention = true;
        ImGui::PopID();
        return true;  // Needs attention
    }

    ImGui::PopID();
    return m_agent->needs_attention;
}

void AgentWindow::RenderTopOverlay(const ImVec2& pos, const ImVec2& size) {
    ImDrawList* drawList = ImGui::GetBackgroundDrawList();

    // Gradient background
    ImVec2 overlayEnd(pos.x + size.x, pos.y + m_topOverlayHeight);
    drawList->AddRectFilledMultiColor(
        pos,
        overlayEnd,
        IM_COL32(35, 35, 40, static_cast<int>(255 * m_overlayOpacity)),
        IM_COL32(35, 35, 40, static_cast<int>(255 * m_overlayOpacity)),
        IM_COL32(25, 25, 30, static_cast<int>(200 * m_overlayOpacity)),
        IM_COL32(25, 25, 30, static_cast<int>(200 * m_overlayOpacity))
    );

    float xOffset = pos.x + 8;
    float yCenter = pos.y + m_topOverlayHeight / 2;

    // Status indicator dot
    float dotRadius = 5.0f;
    float r, g, b;
    GetStatusColor(m_agent->status, r, g, b);

    drawList->AddCircleFilled(
        ImVec2(xOffset + dotRadius, yCenter),
        dotRadius,
        IM_COL32(static_cast<int>(r * 255), static_cast<int>(g * 255), static_cast<int>(b * 255), 255)
    );
    xOffset += dotRadius * 2 + 8;

    // Agent name
    drawList->AddText(
        ImVec2(xOffset, yCenter - 7),
        IM_COL32(230, 230, 230, 255),
        m_agent->name.c_str()
    );
    xOffset += ImGui::CalcTextSize(m_agent->name.c_str()).x + 12;

    // Agent type badge
    const char* typeText = GetAgentTypeText(m_agent->type);
    ImVec2 badgeSize = ImGui::CalcTextSize(typeText);
    badgeSize.x += 8;
    badgeSize.y += 4;

    // Badge color based on agent type
    ImU32 badgeColor = IM_COL32(60, 60, 70, 255);
    switch (m_agent->type) {
        case AgentType::ClaudeCode:
            badgeColor = IM_COL32(80, 60, 50, 255);  // Brownish orange
            break;
        case AgentType::Aider:
            badgeColor = IM_COL32(50, 70, 60, 255);  // Greenish
            break;
        case AgentType::Cursor:
            badgeColor = IM_COL32(50, 60, 80, 255);  // Bluish
            break;
        default:
            break;
    }

    drawList->AddRectFilled(
        ImVec2(xOffset, yCenter - badgeSize.y / 2),
        ImVec2(xOffset + badgeSize.x, yCenter + badgeSize.y / 2),
        badgeColor,
        3.0f
    );
    drawList->AddText(
        ImVec2(xOffset + 4, yCenter - 6),
        IM_COL32(180, 180, 180, 255),
        typeText
    );
    xOffset += badgeSize.x + 8;

    // Auto-accept indicator
    if (m_agent->auto_accept_edits) {
        const char* autoText = "AUTO";
        ImVec2 autoSize = ImGui::CalcTextSize(autoText);
        autoSize.x += 6;
        autoSize.y += 4;

        drawList->AddRectFilled(
            ImVec2(xOffset, yCenter - autoSize.y / 2),
            ImVec2(xOffset + autoSize.x, yCenter + autoSize.y / 2),
            IM_COL32(90, 60, 30, 255),
            3.0f
        );
        drawList->AddText(
            ImVec2(xOffset + 3, yCenter - 6),
            IM_COL32(255, 180, 80, 255),
            autoText
        );
    }

    // Git branch (right-aligned)
    if (m_agent->git_info.is_git_repo && !m_branchDisplay.empty()) {
        ImVec2 branchSize = ImGui::CalcTextSize(m_branchDisplay.c_str());
        float branchX = pos.x + size.x - branchSize.x - 10;

        drawList->AddText(
            ImVec2(branchX, yCenter - 7),
            IM_COL32(120, 180, 120, 255),
            m_branchDisplay.c_str()
        );
    }
}

void AgentWindow::RenderBottomOverlay(const ImVec2& pos, const ImVec2& size) {
    ImDrawList* drawList = ImGui::GetBackgroundDrawList();

    ImVec2 overlayStart(pos.x, pos.y + size.y - m_bottomOverlayHeight);
    ImVec2 overlayEnd(pos.x + size.x, pos.y + size.y);

    // Gradient background (inverted)
    drawList->AddRectFilledMultiColor(
        overlayStart,
        overlayEnd,
        IM_COL32(25, 25, 30, static_cast<int>(200 * m_overlayOpacity)),
        IM_COL32(25, 25, 30, static_cast<int>(200 * m_overlayOpacity)),
        IM_COL32(35, 35, 40, static_cast<int>(255 * m_overlayOpacity)),
        IM_COL32(35, 35, 40, static_cast<int>(255 * m_overlayOpacity))
    );

    float yCenter = overlayStart.y + m_bottomOverlayHeight / 2;

    // Working directory (left)
    std::string dirDisplay = m_agent->working_directory;
    if (dirDisplay.length() > 50) {
        dirDisplay = "..." + dirDisplay.substr(dirDisplay.length() - 47);
    }

    drawList->AddText(
        ImVec2(pos.x + 8, yCenter - 6),
        IM_COL32(140, 140, 145, 255),
        dirDisplay.c_str()
    );

    // Session duration (right-aligned)
    if (m_terminalLaunched) {
        std::string duration = FormatSessionDuration();
        ImVec2 durationSize = ImGui::CalcTextSize(duration.c_str());
        drawList->AddText(
            ImVec2(pos.x + size.x - durationSize.x - 10, yCenter - 6),
            IM_COL32(100, 140, 180, 255),
            duration.c_str()
        );
    } else {
        // Status text
        const char* statusText = GetStatusText(m_agent->status);
        ImVec2 statusSize = ImGui::CalcTextSize(statusText);

        float r, g, b;
        GetStatusColor(m_agent->status, r, g, b);

        drawList->AddText(
            ImVec2(pos.x + size.x - statusSize.x - 10, yCenter - 6),
            IM_COL32(static_cast<int>(r * 200), static_cast<int>(g * 200), static_cast<int>(b * 200), 255),
            statusText
        );
    }
}

void AgentWindow::RenderTerminalContent(const ImVec2& pos, const ImVec2& size) {
    if (!m_buffer) return;

    m_buffer->Lock();
    m_buffer->Render(
        ImGui::GetBackgroundDrawList(),
        pos,
        size,
        m_charWidth,
        m_charHeight,
        m_isFocused  // Only show cursor when focused
    );
    m_buffer->Unlock();
}

void AgentWindow::RenderLaunchButton(const ImVec2& pos, const ImVec2& size) {
    ImVec2 btnSize(140, 40);
    ImVec2 btnPos(
        pos.x + (size.x - btnSize.x) / 2,
        pos.y + (size.y - btnSize.y) / 2
    );

    ImGui::SetCursorScreenPos(btnPos);
    ImGui::PushID(m_agent ? m_agent->id.c_str() : "empty");

    if (ImGui::Button("Launch Terminal", btnSize)) {
        LaunchTerminal();
    }

    ImGui::PopID();
}

void AgentWindow::RenderContextMenu() {
    if (!m_agent) return;

    if (ImGui::BeginPopup(("AgentContext_" + m_agent->id).c_str())) {
        // Header
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Agent: %s", m_agent->name.c_str());
        ImGui::Separator();

        // Agent Info
        if (ImGui::MenuItem("Agent Info...")) {
            m_showAgentInfo = true;
        }

        ImGui::Separator();

        // Auto-Accept Edits toggle
        if (ImGui::MenuItem("Auto-Accept Edits", nullptr, m_agent->auto_accept_edits)) {
            m_agent->auto_accept_edits = !m_agent->auto_accept_edits;
        }

        ImGui::Separator();

        // Fullscreen toggle
        if (ImGui::MenuItem("Fullscreen", nullptr, m_isFullscreen)) {
            ToggleFullscreen();
        }

        // Terminal controls
        if (!m_terminalLaunched) {
            if (ImGui::MenuItem("Launch Terminal")) {
                LaunchTerminal();
            }
        } else {
            if (ImGui::MenuItem("Restart Terminal")) {
                RestartTerminal();
            }
            if (ImGui::MenuItem("Stop Terminal")) {
                TerminateTerminal();
            }
        }

        ImGui::Separator();

        // Git operations
        if (ImGui::MenuItem("Refresh Git Info")) {
            GitUtils::UpdateGitInfo(*m_agent);
            m_branchDisplay = GitUtils::FormatBranchDisplay(m_agent->git_info);
        }

        if (ImGui::MenuItem("Open Directory...")) {
#ifdef PLATFORM_WINDOWS
            std::string cmd = "explorer \"" + m_agent->working_directory + "\"";
            system(cmd.c_str());
#endif
        }

        ImGui::Separator();

        // Remove agent (dangerous)
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.3f, 0.3f, 1.0f));
        if (ImGui::MenuItem("Remove Agent")) {
            // TODO: Signal removal to manager
        }
        ImGui::PopStyleColor();

        ImGui::EndPopup();
    }
}

void AgentWindow::RenderAgentInfoPopup() {
    if (!m_agent) {
        m_showAgentInfo = false;
        return;
    }

    ImGui::SetNextWindowSize(ImVec2(400, 320), ImGuiCond_FirstUseEver);

    if (ImGui::Begin("Agent Information", &m_showAgentInfo)) {
        ImGui::TextColored(ImVec4(0.3f, 0.8f, 0.4f, 1.0f), "AGENT INFORMATION");
        ImGui::Separator();
        ImGui::Spacing();

        // Basic info
        ImGui::Text("Name:      %s", m_agent->name.c_str());
        ImGui::Text("Type:      %s", GetAgentTypeText(m_agent->type));
        ImGui::Text("Command:   %s", m_agent->command.c_str());

        // Truncate directory if too long
        std::string dir = m_agent->working_directory;
        if (dir.length() > 40) {
            dir = "..." + dir.substr(dir.length() - 37);
        }
        ImGui::Text("Directory: %s", dir.c_str());

        if (m_agent->git_info.is_git_repo) {
            ImGui::Text("Branch:    %s", m_agent->git_info.branch.c_str());
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // Session info
        if (m_terminalLaunched) {
            ImGui::Text("Session Duration: %s", FormatSessionDuration().c_str());

            if (m_agent->tokens_remaining >= 0) {
                ImGui::Text("Tokens Remaining: ~%d", m_agent->tokens_remaining);
            }
        } else {
            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Terminal not started");
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // Settings
        ImGui::Text("Auto-Accept: %s", m_agent->auto_accept_edits ? "ON" : "OFF");
        ImGui::Text("Status:      %s", GetStatusText(m_agent->status));

        ImGui::Spacing();

        if (ImGui::Button("Close", ImVec2(-1, 30))) {
            m_showAgentInfo = false;
        }
    }
    ImGui::End();
}

ImVec4 AgentWindow::GetTerminalRegion(const ImVec2& panelPos, const ImVec2& panelSize) const {
    return ImVec4(
        panelPos.x + 1,
        panelPos.y + m_topOverlayHeight,
        panelSize.x - 2,
        panelSize.y - m_topOverlayHeight - m_bottomOverlayHeight
    );
}

std::string AgentWindow::FormatSessionDuration() const {
    if (!m_agent || m_agent->session_start == std::chrono::system_clock::time_point()) {
        return "";
    }

    auto now = std::chrono::system_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - m_agent->session_start);

    int hours = static_cast<int>(duration.count() / 3600);
    int minutes = static_cast<int>((duration.count() % 3600) / 60);
    int seconds = static_cast<int>(duration.count() % 60);

    std::ostringstream oss;
    if (hours > 0) {
        oss << hours << "h " << minutes << "m";
    } else if (minutes > 0) {
        oss << minutes << "m " << seconds << "s";
    } else {
        oss << seconds << "s";
    }

    return oss.str();
}

void AgentWindow::LaunchTerminal() {
    if (!m_agent) {
        LOG_WARNING_SRC("LaunchTerminal called with no agent", "AgentWindow");
        return;
    }

    if (m_terminalLaunched) {
        LOG_WARNING_SRC("LaunchTerminal called but terminal already launched for: " + m_agent->name, "AgentWindow");
        return;
    }

    LOG_INFO_SRC("Launching terminal for agent: " + m_agent->name, "AgentWindow");

    // Create terminal
    m_terminal = std::make_unique<ConPTYTerminal>();

    // Set up output callback
    m_terminal->SetOutputCallback([this](const char* data, size_t length) {
        OnTerminalOutput(data, length);
    });

    // Build command
    std::string command = m_agent->command;
    if (command.empty()) {
        // Default to cmd.exe if no command specified
        command = "cmd.exe";
        LOG_DEBUG_SRC("No command specified, using default: cmd.exe", "AgentWindow");
    }

    LOG_INFO_SRC("Command: " + command + " | Dir: " + m_agent->working_directory, "AgentWindow");

    // Launch
    if (m_terminal->Launch(command, m_agent->args, m_agent->working_directory,
                           m_terminalCols, m_terminalRows)) {
        m_terminalLaunched = true;
        m_agent->status = AgentStatus::Running;
        m_agent->session_start = std::chrono::system_clock::now();
        m_agent->started_at = m_agent->session_start;
        m_agent->pid = m_terminal->GetProcessId();
        m_agent->needs_attention = false;
        LOG_INFO_SRC("Terminal launched successfully, PID: " + std::to_string(m_agent->pid), "AgentWindow");
    } else {
        m_agent->status = AgentStatus::Error;
        m_agent->status_message = "Failed to launch terminal";
        m_terminal.reset();
        LOG_ERROR_SRC("Failed to launch terminal for: " + m_agent->name, "AgentWindow");
    }
}

void AgentWindow::TerminateTerminal() {
    if (!m_terminalLaunched || !m_terminal) return;

    m_terminal->Terminate();
    m_terminal.reset();
    m_terminalLaunched = false;
    m_agent->status = AgentStatus::Stopped;
    m_agent->pid = -1;
}

void AgentWindow::RestartTerminal() {
    TerminateTerminal();

    // Clear buffer
    if (m_buffer) {
        m_buffer->Clear();
    }

    LaunchTerminal();
}

bool AgentWindow::IsTerminalRunning() const {
    return m_terminalLaunched && m_terminal && m_terminal->IsRunning();
}

void AgentWindow::Focus() {
    m_isFocused = true;
    if (m_buffer) {
        m_buffer->ScrollToBottom();
    }
}

void AgentWindow::Unfocus() {
    m_isFocused = false;
}

void AgentWindow::SetFullscreen(bool fullscreen) {
    m_isFullscreen = fullscreen;
}

int AgentWindow::GetTerminalCols() const {
    return m_terminalCols;
}

int AgentWindow::GetTerminalRows() const {
    return m_terminalRows;
}

void AgentWindow::HandleKeyboardInput() {
    if (!m_terminal || !m_terminalLaunched || !m_isFocused) return;

    ImGuiIO& io = ImGui::GetIO();

    // Handle text input
    if (io.InputQueueCharacters.Size > 0) {
        for (int i = 0; i < io.InputQueueCharacters.Size; ++i) {
            ImWchar ch = io.InputQueueCharacters[i];
            if (ch > 0 && ch < 0x10000 && ch != 127) {  // Exclude DEL
                // Convert to UTF-8
                char utf8[5] = {0};
                if (ch < 0x80) {
                    utf8[0] = static_cast<char>(ch);
                } else if (ch < 0x800) {
                    utf8[0] = static_cast<char>(0xC0 | (ch >> 6));
                    utf8[1] = static_cast<char>(0x80 | (ch & 0x3F));
                } else {
                    utf8[0] = static_cast<char>(0xE0 | (ch >> 12));
                    utf8[1] = static_cast<char>(0x80 | ((ch >> 6) & 0x3F));
                    utf8[2] = static_cast<char>(0x80 | (ch & 0x3F));
                }
                m_terminal->Write(utf8);
            }
        }
    }

    // Handle special keys
    auto sendEscape = [this](const char* seq) {
        m_terminal->Write(seq);
    };

    // Arrow keys
    if (ImGui::IsKeyPressed(ImGuiKey_UpArrow)) {
        sendEscape("\x1b[A");
    }
    if (ImGui::IsKeyPressed(ImGuiKey_DownArrow)) {
        sendEscape("\x1b[B");
    }
    if (ImGui::IsKeyPressed(ImGuiKey_RightArrow)) {
        sendEscape("\x1b[C");
    }
    if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow)) {
        sendEscape("\x1b[D");
    }

    // Home/End
    if (ImGui::IsKeyPressed(ImGuiKey_Home)) {
        sendEscape("\x1b[H");
    }
    if (ImGui::IsKeyPressed(ImGuiKey_End)) {
        sendEscape("\x1b[F");
    }

    // Page Up/Down
    if (ImGui::IsKeyPressed(ImGuiKey_PageUp)) {
        sendEscape("\x1b[5~");
    }
    if (ImGui::IsKeyPressed(ImGuiKey_PageDown)) {
        sendEscape("\x1b[6~");
    }

    // Delete/Insert
    if (ImGui::IsKeyPressed(ImGuiKey_Delete)) {
        sendEscape("\x1b[3~");
    }
    if (ImGui::IsKeyPressed(ImGuiKey_Insert)) {
        sendEscape("\x1b[2~");
    }

    // Enter
    if (ImGui::IsKeyPressed(ImGuiKey_Enter) || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter)) {
        m_terminal->Write("\r");
    }

    // Backspace
    if (ImGui::IsKeyPressed(ImGuiKey_Backspace)) {
        m_terminal->Write("\x7f");
    }

    // Tab
    if (ImGui::IsKeyPressed(ImGuiKey_Tab)) {
        m_terminal->Write("\t");
    }

    // Escape
    if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
        m_terminal->Write("\x1b");
    }

    // Ctrl+C - Send interrupt to terminal (use Ctrl+Shift+C for copy)
    if (io.KeyCtrl && !io.KeyShift && ImGui::IsKeyPressed(ImGuiKey_C)) {
        m_terminal->Write("\x03");
    }

    // Ctrl+Shift+C - Copy from terminal
    if (io.KeyCtrl && io.KeyShift && ImGui::IsKeyPressed(ImGuiKey_C)) {
        CopyToClipboard();
    }

    // Ctrl+V or Ctrl+Shift+V - Paste to terminal
    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_V)) {
        PasteFromClipboard();
    }

    // Ctrl+D
    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_D)) {
        m_terminal->Write("\x04");
    }

    // Ctrl+Z
    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Z)) {
        m_terminal->Write("\x1a");
    }

    // Ctrl+L (clear)
    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_L)) {
        m_terminal->Write("\x0c");
    }

    // Function keys
    const char* fkeySeqs[] = {
        "\x1bOP", "\x1bOQ", "\x1bOR", "\x1bOS",  // F1-F4
        "\x1b[15~", "\x1b[17~", "\x1b[18~", "\x1b[19~",  // F5-F8
        "\x1b[20~", "\x1b[21~", "\x1b[23~", "\x1b[24~"   // F9-F12
    };

    for (int i = 0; i < 12; ++i) {
        if (ImGui::IsKeyPressed(static_cast<ImGuiKey>(ImGuiKey_F1 + i))) {
            sendEscape(fkeySeqs[i]);
        }
    }
}

void AgentWindow::OnTerminalOutput(const char* data, size_t length) {
    if (m_buffer) {
        m_buffer->ProcessInput(data, length);
    }
}

void AgentWindow::CopyToClipboard() {
    if (!m_buffer) return;

    // Get selected text from buffer (or last line for now)
    std::string text = m_buffer->GetSelectedText();

    if (text.empty()) {
        LOG_DEBUG_SRC("No text selected to copy", "AgentWindow");
        return;
    }

#ifdef PLATFORM_WINDOWS
    if (OpenClipboard(nullptr)) {
        EmptyClipboard();
        HGLOBAL hGlobal = GlobalAlloc(GMEM_MOVEABLE, text.size() + 1);
        if (hGlobal) {
            char* pGlobal = static_cast<char*>(GlobalLock(hGlobal));
            memcpy(pGlobal, text.c_str(), text.size() + 1);
            GlobalUnlock(hGlobal);
            SetClipboardData(CF_TEXT, hGlobal);
        }
        CloseClipboard();
        LOG_DEBUG_SRC("Copied to clipboard: " + std::to_string(text.size()) + " chars", "AgentWindow");
    }
#else
    // Use ImGui clipboard for other platforms
    ImGui::SetClipboardText(text.c_str());
#endif
}

void AgentWindow::PasteFromClipboard() {
    if (!m_terminal || !m_terminalLaunched) return;

    std::string text;

#ifdef PLATFORM_WINDOWS
    if (OpenClipboard(nullptr)) {
        HANDLE hData = GetClipboardData(CF_TEXT);
        if (hData) {
            char* pData = static_cast<char*>(GlobalLock(hData));
            if (pData) {
                text = pData;
                GlobalUnlock(hData);
            }
        }
        CloseClipboard();
    }
#else
    // Use ImGui clipboard for other platforms
    const char* clipText = ImGui::GetClipboardText();
    if (clipText) {
        text = clipText;
    }
#endif

    if (!text.empty()) {
        // Send pasted text to terminal
        m_terminal->Write(text);
        LOG_DEBUG_SRC("Pasted from clipboard: " + std::to_string(text.size()) + " chars", "AgentWindow");
    }
}

void AgentWindow::UpdateTerminalSize(int widthPx, int heightPx) {
    // Calculate character dimensions (use default for now, could load from font)
    // For a typical monospace font at default size
    m_charWidth = 8.0f;
    m_charHeight = 16.0f;

    int newCols = static_cast<int>(widthPx / m_charWidth);
    int newRows = static_cast<int>(heightPx / m_charHeight);

    // Clamp to reasonable values
    newCols = (std::max)(10, (std::min)(500, newCols));
    newRows = (std::max)(5, (std::min)(200, newRows));

    if (newCols != m_terminalCols || newRows != m_terminalRows) {
        m_terminalCols = newCols;
        m_terminalRows = newRows;

        // Resize terminal and buffer
        if (m_terminal && m_terminalLaunched) {
            m_terminal->Resize(newCols, newRows);
        }
        if (m_buffer) {
            m_buffer->Resize(newCols, newRows);
        }
    }
}

} // namespace AgentSmith

#include "input_manager.h"
#include "agent_window.h"
#include <imgui.h>

namespace AgentSmith {

InputManager::InputManager() = default;
InputManager::~InputManager() = default;

void InputManager::SetAgentWindows(std::vector<AgentWindow*> windows) {
    m_windows = std::move(windows);

    // Clamp focus index
    if (m_windows.empty()) {
        m_focusedIndex = 0;
    } else {
        m_focusedIndex = (std::min)(m_focusedIndex, static_cast<int>(m_windows.size()) - 1);
    }
}

void InputManager::SetFocusedIndex(int index) {
    if (m_windows.empty()) {
        m_focusedIndex = 0;
        return;
    }

    int oldIndex = m_focusedIndex;
    m_focusedIndex = (std::max)(0, (std::min)(index, static_cast<int>(m_windows.size()) - 1));

    if (oldIndex != m_focusedIndex) {
        // Unfocus old window
        if (oldIndex >= 0 && oldIndex < static_cast<int>(m_windows.size())) {
            m_windows[oldIndex]->Unfocus();
        }

        // Focus new window
        if (m_focusedIndex >= 0 && m_focusedIndex < static_cast<int>(m_windows.size())) {
            m_windows[m_focusedIndex]->Focus();
        }

        // Notify callback
        if (m_focusCallback) {
            m_focusCallback(oldIndex, m_focusedIndex);
        }
    }
}

AgentWindow* InputManager::GetFocusedWindow() {
    if (m_focusedIndex >= 0 && m_focusedIndex < static_cast<int>(m_windows.size())) {
        return m_windows[m_focusedIndex];
    }
    return nullptr;
}

bool InputManager::ProcessInput() {
    // First check global shortcuts
    if (HandleGlobalShortcuts()) {
        return true;
    }

    // Route input to focused window
    if (auto* window = GetFocusedWindow()) {
        window->HandleKeyboardInput();
    }

    return false;
}

void InputManager::FocusNext() {
    if (m_windows.empty()) return;

    int newIndex = (m_focusedIndex + 1) % static_cast<int>(m_windows.size());
    SetFocusedIndex(newIndex);
}

void InputManager::FocusPrev() {
    if (m_windows.empty()) return;

    int newIndex = m_focusedIndex - 1;
    if (newIndex < 0) {
        newIndex = static_cast<int>(m_windows.size()) - 1;
    }
    SetFocusedIndex(newIndex);
}

void InputManager::FocusIndex(int index) {
    SetFocusedIndex(index);
}

void InputManager::SetFocusChangeCallback(FocusChangeCallback callback) {
    m_focusCallback = std::move(callback);
}

bool InputManager::HandleGlobalShortcuts() {
    ImGuiIO& io = ImGui::GetIO();

    // Only handle when Ctrl is pressed for navigation shortcuts
    if (!io.KeyCtrl) {
        return false;
    }

    // Ctrl+Tab / Ctrl+Shift+Tab: Cycle focus
    if (ImGui::IsKeyPressed(ImGuiKey_Tab)) {
        if (io.KeyShift) {
            FocusPrev();
        } else {
            FocusNext();
        }
        return true;
    }

    // Ctrl+1 through Ctrl+9: Direct focus
    for (int i = 0; i < 9; ++i) {
        ImGuiKey key = static_cast<ImGuiKey>(ImGuiKey_1 + i);
        if (ImGui::IsKeyPressed(key)) {
            if (i < static_cast<int>(m_windows.size())) {
                SetFocusedIndex(i);
            }
            return true;
        }
    }

    // Ctrl+Arrow keys for 2D navigation could be added here
    // (would need grid layout info)

    return false;
}

const char* InputManager::KeyToEscapeSequence(int key, bool ctrl, bool shift, bool alt) {
    // This method could be expanded for more complex key translation
    // Currently, key handling is done in AgentWindow::HandleKeyboardInput

    // Basic escape sequences for common keys
    switch (key) {
        case ImGuiKey_UpArrow:    return "\x1b[A";
        case ImGuiKey_DownArrow:  return "\x1b[B";
        case ImGuiKey_RightArrow: return "\x1b[C";
        case ImGuiKey_LeftArrow:  return "\x1b[D";
        case ImGuiKey_Home:       return "\x1b[H";
        case ImGuiKey_End:        return "\x1b[F";
        case ImGuiKey_PageUp:     return "\x1b[5~";
        case ImGuiKey_PageDown:   return "\x1b[6~";
        case ImGuiKey_Delete:     return "\x1b[3~";
        case ImGuiKey_Insert:     return "\x1b[2~";
        case ImGuiKey_F1:         return "\x1bOP";
        case ImGuiKey_F2:         return "\x1bOQ";
        case ImGuiKey_F3:         return "\x1bOR";
        case ImGuiKey_F4:         return "\x1bOS";
        case ImGuiKey_F5:         return "\x1b[15~";
        case ImGuiKey_F6:         return "\x1b[17~";
        case ImGuiKey_F7:         return "\x1b[18~";
        case ImGuiKey_F8:         return "\x1b[19~";
        case ImGuiKey_F9:         return "\x1b[20~";
        case ImGuiKey_F10:        return "\x1b[21~";
        case ImGuiKey_F11:        return "\x1b[23~";
        case ImGuiKey_F12:        return "\x1b[24~";
        default:                  return nullptr;
    }
}

} // namespace AgentSmith

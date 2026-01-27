#pragma once

#include <functional>
#include <vector>

namespace AgentSmith {

class AgentWindow;

//=============================================================================
// Input Manager
//
// Manages keyboard input routing to focused agent windows.
// Handles global shortcuts (Ctrl+Tab, Ctrl+1-9, etc.)
//=============================================================================

class InputManager {
public:
    InputManager();
    ~InputManager();

    // Set the list of agent windows to manage
    void SetAgentWindows(std::vector<AgentWindow*> windows);

    // Set the currently focused window index
    void SetFocusedIndex(int index);
    int GetFocusedIndex() const { return m_focusedIndex; }

    // Get the focused window
    AgentWindow* GetFocusedWindow();

    // Process input for the current frame
    // Returns true if a global shortcut was consumed
    bool ProcessInput();

    // Focus navigation
    void FocusNext();
    void FocusPrev();
    void FocusIndex(int index);

    // Callbacks for focus changes
    using FocusChangeCallback = std::function<void(int oldIndex, int newIndex)>;
    void SetFocusChangeCallback(FocusChangeCallback callback);

private:
    // Check and handle global shortcuts
    bool HandleGlobalShortcuts();

    // Translate key to terminal escape sequence
    const char* KeyToEscapeSequence(int key, bool ctrl, bool shift, bool alt);

    std::vector<AgentWindow*> m_windows;
    int m_focusedIndex = 0;

    FocusChangeCallback m_focusCallback;
};

} // namespace AgentSmith

#pragma once

#include "types.h"
#include "config.h"
#include "agents_tracker.h"
#include "grid_layout.h"
#include "input_manager.h"
#include <memory>

struct GLFWwindow;

namespace AgentSmith {

//=============================================================================
// Application
//
// Main application class. Handles:
// - Window creation and management
// - ImGui setup and rendering
// - Main menu bar
// - Coordination between components
//=============================================================================

class App {
public:
    App();
    ~App();

    // Initialize the application
    bool Init();

    // Run the main loop
    void Run();

    // Shutdown
    void Shutdown();

private:
    // Setup ImGui style
    void SetupStyle();

    // Load monospace font for terminal
    void LoadTerminalFont();

    // Render main menu bar
    void RenderMenuBar();

    // Render the "Add Agent" dialog
    void RenderAddAgentDialog();

    // Render the settings dialog
    void RenderSettingsDialog();

    // Render the about dialog
    void RenderAboutDialog();

    // Render the clear agents confirmation dialog
    void RenderClearAgentsDialog();

    // Handle global keyboard shortcuts
    void HandleGlobalShortcuts();

    // Get the directory containing the executable
    std::string GetExecutableDirectory();

    // Window and state
    GLFWwindow* m_window = nullptr;
    void* m_native_window = nullptr;
    bool m_running = true;

    // Core components
    std::unique_ptr<ConfigManager> m_config;
    std::unique_ptr<GridLayout> m_grid_layout;
    std::unique_ptr<InputManager> m_input_manager;

    // UI state
    bool m_show_add_agent_dialog = false;
    bool m_show_settings_dialog = false;
    bool m_show_about_dialog = false;
    bool m_show_demo_window = false;
    bool m_show_output_log = true;  // Auto-open for debugging
    bool m_show_clear_agents_dialog = false;

    // Add agent dialog state
    char m_new_agent_name[128] = "New Agent";
    char m_new_agent_dir[512] = "";
    int m_new_agent_type = 0;

    // Timing
    float m_git_update_timer = 0.0f;
    float m_git_update_interval = 5.0f;  // Update git info every 5 seconds

    // Config path (in executable directory)
    std::string m_config_path;
};

} // namespace AgentSmith

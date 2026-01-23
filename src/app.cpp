#include "app.h"
#include "terminal_embed.h"
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <iostream>

#ifdef PLATFORM_WINDOWS
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#elif defined(PLATFORM_LINUX)
#define GLFW_EXPOSE_NATIVE_X11
#include <GLFW/glfw3native.h>
#elif defined(PLATFORM_MACOS)
#define GLFW_EXPOSE_NATIVE_COCOA
#include <GLFW/glfw3native.h>
#endif

namespace AgentSmith {

App::App() = default;
App::~App() = default;

bool App::Init() {
    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return false;
    }
    
    // GL 3.3 + GLSL 330
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
    
    // Create window
    m_window = glfwCreateWindow(1920, 1080, "AgentSmith - Multi-Agent Control", nullptr, nullptr);
    if (!m_window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return false;
    }
    
    glfwMakeContextCurrent(m_window);
    glfwSwapInterval(1);  // Enable vsync
    
    // Get native window handle for terminal embedding
    m_native_window = TerminalEmbed::GetNativeWindowHandle(m_window);
    
    // Initialize terminal embedding system
    if (!TerminalEmbed::Initialize()) {
        std::cerr << "Warning: Terminal embedding not available on this platform" << std::endl;
    }
    
    // Initialize ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    
    // Setup platform/renderer backends
    ImGui_ImplGlfw_InitForOpenGL(m_window, true);
    ImGui_ImplOpenGL3_Init("#version 330");
    
    // Load configuration
    m_config = std::make_unique<ConfigManager>();
    m_config->Load("config.json");
    
    // Initialize agent manager
    m_agent_manager = std::make_unique<AgentManager>();
    
    // Load saved agents from config
    for (auto& agent : m_config->GetConfig().agents) {
        m_agent_manager->GetAgents().push_back(agent);
    }
    
    // Initialize grid layout
    m_grid_layout = std::make_unique<GridLayout>();
    m_grid_layout->SetConfig(m_config->GetGridConfig());
    m_grid_layout->SyncWithAgents(m_agent_manager->GetAgents());
    
    // Setup style
    SetupStyle();
    
    return true;
}

void App::SetupStyle() {
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;
    
    // Dark theme optimized for terminal viewing
    colors[ImGuiCol_WindowBg] = ImVec4(0.08f, 0.08f, 0.10f, 1.00f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.10f, 0.10f, 0.12f, 1.00f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.12f, 0.12f, 0.14f, 0.95f);
    
    colors[ImGuiCol_Header] = ImVec4(0.18f, 0.18f, 0.20f, 1.00f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.26f, 0.26f, 0.28f, 1.00f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.30f, 0.30f, 0.33f, 1.00f);
    
    // Accent color (matrix green)
    colors[ImGuiCol_Button] = ImVec4(0.20f, 0.55f, 0.30f, 1.00f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.25f, 0.65f, 0.35f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.30f, 0.75f, 0.40f, 1.00f);
    
    colors[ImGuiCol_FrameBg] = ImVec4(0.12f, 0.12f, 0.14f, 1.00f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.18f, 0.18f, 0.20f, 1.00f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.22f, 0.22f, 0.24f, 1.00f);
    
    colors[ImGuiCol_TitleBg] = ImVec4(0.06f, 0.06f, 0.08f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.10f, 0.10f, 0.12f, 1.00f);
    
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.10f, 0.10f, 0.12f, 1.00f);
    
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.08f, 0.08f, 0.10f, 1.00f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.28f, 0.28f, 0.30f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.38f, 0.38f, 0.40f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.48f, 0.48f, 0.50f, 1.00f);
    
    colors[ImGuiCol_Tab] = ImVec4(0.12f, 0.12f, 0.14f, 1.00f);
    colors[ImGuiCol_TabHovered] = ImVec4(0.20f, 0.55f, 0.30f, 0.80f);
    colors[ImGuiCol_TabActive] = ImVec4(0.20f, 0.55f, 0.30f, 1.00f);
    
    colors[ImGuiCol_Text] = ImVec4(0.92f, 0.92f, 0.92f, 1.00f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.45f, 0.45f, 0.45f, 1.00f);
    
    colors[ImGuiCol_Separator] = ImVec4(0.22f, 0.22f, 0.24f, 1.00f);
    colors[ImGuiCol_Border] = ImVec4(0.22f, 0.22f, 0.24f, 1.00f);
    
    // Style adjustments
    style.WindowRounding = 4.0f;
    style.FrameRounding = 3.0f;
    style.GrabRounding = 3.0f;
    style.ScrollbarRounding = 3.0f;
    style.TabRounding = 3.0f;
    style.WindowPadding = ImVec2(8, 8);
    style.FramePadding = ImVec2(6, 4);
    style.ItemSpacing = ImVec2(6, 4);
    style.ScrollbarSize = 12.0f;
    style.WindowBorderSize = 1.0f;
    style.ChildBorderSize = 1.0f;
}

void App::Run() {
    while (!glfwWindowShouldClose(m_window) && m_running) {
        glfwPollEvents();
        
        // Handle global shortcuts
        HandleGlobalShortcuts();
        
        // Update timing
        float delta_time = ImGui::GetIO().DeltaTime;
        m_git_update_timer += delta_time;
        
        // Periodically update git info
        if (m_git_update_timer >= m_git_update_interval) {
            m_agent_manager->UpdateAllGitInfo();
            m_git_update_timer = 0.0f;
        }
        
        // Update agent statuses
        m_agent_manager->Update();
        
        // Start ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        
        // Render menu bar
        RenderMenuBar();
        
        // Calculate content area (below menu bar)
        float menu_bar_height = ImGui::GetFrameHeight();
        int window_width, window_height;
        glfwGetWindowSize(m_window, &window_width, &window_height);
        
        ImVec2 content_pos(0, menu_bar_height);
        ImVec2 content_size((float)window_width, (float)window_height - menu_bar_height);
        
        // Render grid layout
        m_grid_layout->SyncWithAgents(m_agent_manager->GetAgents());
        m_grid_layout->Render(content_pos, content_size, m_native_window);
        
        // Render dialogs
        if (m_show_add_agent_dialog) RenderAddAgentDialog();
        if (m_show_settings_dialog) RenderSettingsDialog();
        if (m_show_about_dialog) RenderAboutDialog();
        if (m_show_demo_window) ImGui::ShowDemoWindow(&m_show_demo_window);
        
        // Rendering
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(m_window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.08f, 0.08f, 0.10f, 1.00f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        
        glfwSwapBuffers(m_window);
    }
}

void App::RenderMenuBar() {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Add Agent...", "Ctrl+N")) {
                m_show_add_agent_dialog = true;
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Save Layout")) {
                m_config->GetConfig().agents = m_agent_manager->GetAgents();
                m_config->Save("config.json");
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Exit", "Alt+F4")) {
                m_running = false;
            }
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("View")) {
            if (ImGui::BeginMenu("Grid Layout")) {
                if (ImGui::MenuItem("1x1", nullptr, m_config->GetGridConfig().rows == 1 && m_config->GetGridConfig().cols == 1)) {
                    m_grid_layout->SetLayout1x1();
                }
                if (ImGui::MenuItem("1x2", nullptr, m_config->GetGridConfig().rows == 1 && m_config->GetGridConfig().cols == 2)) {
                    m_grid_layout->SetLayout1x2();
                }
                if (ImGui::MenuItem("2x1", nullptr, m_config->GetGridConfig().rows == 2 && m_config->GetGridConfig().cols == 1)) {
                    m_grid_layout->SetLayout2x1();
                }
                if (ImGui::MenuItem("2x2", nullptr, m_config->GetGridConfig().rows == 2 && m_config->GetGridConfig().cols == 2)) {
                    m_grid_layout->SetLayout2x2();
                }
                if (ImGui::MenuItem("2x3", nullptr, m_config->GetGridConfig().rows == 2 && m_config->GetGridConfig().cols == 3)) {
                    m_grid_layout->SetLayout2x3();
                }
                if (ImGui::MenuItem("3x3", nullptr, m_config->GetGridConfig().rows == 3 && m_config->GetGridConfig().cols == 3)) {
                    m_grid_layout->SetLayout3x3();
                }
                ImGui::EndMenu();
            }
            ImGui::Separator();
            ImGui::MenuItem("Show Git Info", nullptr, &m_config->GetGridConfig().show_git_info);
            ImGui::MenuItem("Show Status Indicator", nullptr, &m_config->GetGridConfig().show_status_indicator);
            ImGui::Separator();
            if (ImGui::MenuItem("Settings...", "Ctrl+,")) {
                m_show_settings_dialog = true;
            }
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("Agents")) {
            if (ImGui::MenuItem("Start All")) {
                m_agent_manager->StartAllAgents();
            }
            if (ImGui::MenuItem("Stop All")) {
                m_agent_manager->StopAllAgents();
            }
            if (ImGui::MenuItem("Restart All")) {
                m_agent_manager->RestartAllAgents();
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Refresh Git Info")) {
                m_agent_manager->UpdateAllGitInfo();
            }
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("Help")) {
            if (ImGui::MenuItem("About AgentSmith")) {
                m_show_about_dialog = true;
            }
            ImGui::Separator();
            if (ImGui::MenuItem("ImGui Demo")) {
                m_show_demo_window = true;
            }
            ImGui::EndMenu();
        }
        
        // Right-aligned agent count
        float right_text_width = 150.0f;
        ImGui::SetCursorPosX(ImGui::GetWindowWidth() - right_text_width);
        
        size_t running_count = 0;
        for (const auto& agent : m_agent_manager->GetAgents()) {
            if (agent.status == AgentStatus::Running) running_count++;
        }
        
        ImGui::Text("%zu/%zu agents", running_count, m_agent_manager->GetAgentCount());
        
        ImGui::EndMainMenuBar();
    }
}

void App::RenderAddAgentDialog() {
    ImGui::SetNextWindowSize(ImVec2(450, 280), ImGuiCond_FirstUseEver);
    
    if (ImGui::Begin("Add Agent", &m_show_add_agent_dialog)) {
        ImGui::Text("Agent Name:");
        ImGui::InputText("##AgentName", m_new_agent_name, sizeof(m_new_agent_name));
        
        ImGui::Spacing();
        
        ImGui::Text("Working Directory:");
        ImGui::InputText("##WorkDir", m_new_agent_dir, sizeof(m_new_agent_dir));
        ImGui::SameLine();
        if (ImGui::Button("Browse...")) {
            // TODO: Open file dialog
        }
        
        ImGui::Spacing();
        
        ImGui::Text("Agent Type:");
        const char* agent_types[] = { "Claude Code", "Aider", "Cursor", "Custom" };
        ImGui::Combo("##AgentType", &m_new_agent_type, agent_types, 4);
        
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        
        if (ImGui::Button("Create", ImVec2(100, 30))) {
            if (strlen(m_new_agent_name) > 0 && strlen(m_new_agent_dir) > 0) {
                m_agent_manager->CreateAgent(
                    m_new_agent_name,
                    m_new_agent_dir,
                    static_cast<AgentType>(m_new_agent_type)
                );
                m_grid_layout->SyncWithAgents(m_agent_manager->GetAgents());
                m_show_add_agent_dialog = false;
                
                // Reset form
                strcpy(m_new_agent_name, "New Agent");
                m_new_agent_dir[0] = '\0';
                m_new_agent_type = 0;
            }
        }
        
        ImGui::SameLine();
        
        if (ImGui::Button("Cancel", ImVec2(100, 30))) {
            m_show_add_agent_dialog = false;
        }
    }
    ImGui::End();
}

void App::RenderSettingsDialog() {
    ImGui::SetNextWindowSize(ImVec2(500, 400), ImGuiCond_FirstUseEver);
    
    if (ImGui::Begin("Settings", &m_show_settings_dialog)) {
        if (ImGui::BeginTabBar("SettingsTabs")) {
            if (ImGui::BeginTabItem("Grid")) {
                auto& grid = m_config->GetGridConfig();
                
                ImGui::Text("Grid Size");
                ImGui::SliderInt("Rows", &grid.rows, 1, 4);
                ImGui::SliderInt("Columns", &grid.cols, 1, 4);
                
                ImGui::Spacing();
                ImGui::Text("Overlays");
                ImGui::Checkbox("Show Git Info", &grid.show_git_info);
                ImGui::Checkbox("Show Status Indicator", &grid.show_status_indicator);
                ImGui::Checkbox("Show File Activity (Future)", &grid.show_file_activity);
                
                ImGui::Spacing();
                ImGui::Text("Appearance");
                ImGui::SliderFloat("Panel Padding", &grid.panel_padding, 0.0f, 20.0f);
                ImGui::SliderFloat("Overlay Height", &grid.info_overlay_height, 20.0f, 50.0f);
                
                ImGui::EndTabItem();
            }
            
            if (ImGui::BeginTabItem("Terminals")) {
                auto& config = m_config->GetConfig();
                
                ImGui::Text("Default Terminal:");
                static char term_cmd[256];
                strncpy(term_cmd, config.default_terminal_command.c_str(), sizeof(term_cmd) - 1);
                if (ImGui::InputText("##TermCmd", term_cmd, sizeof(term_cmd))) {
                    config.default_terminal_command = term_cmd;
                }
                
                ImGui::Spacing();
                ImGui::Text("Default Shell:");
                static char shell_cmd[256];
                strncpy(shell_cmd, config.default_shell.c_str(), sizeof(shell_cmd) - 1);
                if (ImGui::InputText("##ShellCmd", shell_cmd, sizeof(shell_cmd))) {
                    config.default_shell = shell_cmd;
                }
                
                ImGui::EndTabItem();
            }
            
            if (ImGui::BeginTabItem("Theme")) {
                auto& theme = m_config->GetConfig().theme;
                
                float accent[3] = { theme.accent_r, theme.accent_g, theme.accent_b };
                if (ImGui::ColorEdit3("Accent Color", accent)) {
                    theme.accent_r = accent[0];
                    theme.accent_g = accent[1];
                    theme.accent_b = accent[2];
                }
                
                ImGui::EndTabItem();
            }
            
            ImGui::EndTabBar();
        }
        
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        
        if (ImGui::Button("Save", ImVec2(100, 30))) {
            m_config->Save("config.json");
            m_grid_layout->SetConfig(m_config->GetGridConfig());
            m_show_settings_dialog = false;
        }
        
        ImGui::SameLine();
        
        if (ImGui::Button("Cancel", ImVec2(100, 30))) {
            m_show_settings_dialog = false;
        }
    }
    ImGui::End();
}

void App::RenderAboutDialog() {
    ImGui::SetNextWindowSize(ImVec2(400, 300), ImGuiCond_FirstUseEver);
    
    if (ImGui::Begin("About AgentSmith", &m_show_about_dialog)) {
        ImGui::TextColored(ImVec4(0.3f, 0.8f, 0.4f, 1.0f), "AgentSmith");
        ImGui::Text("Version 0.1.0");
        
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        
        ImGui::TextWrapped(
            "A mission control interface for managing multiple AI coding agents. "
            "Monitor Claude Code, Aider, and other AI assistants in a unified grid view "
            "with real-time git status and terminal embedding."
        );
        
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        
        ImGui::Text("Future Features:");
        ImGui::BulletText("Working file visualizer per agent");
        ImGui::BulletText("Token usage tracking");
        ImGui::BulletText("Diff viewer for changes");
        ImGui::BulletText("Agent communication/coordination");
        ImGui::BulletText("Session recording and playback");
        
        ImGui::Spacing();
        
        if (ImGui::Button("Close", ImVec2(-1, 30))) {
            m_show_about_dialog = false;
        }
    }
    ImGui::End();
}

void App::HandleGlobalShortcuts() {
    ImGuiIO& io = ImGui::GetIO();
    
    // Ctrl+N: New agent
    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_N)) {
        m_show_add_agent_dialog = true;
    }
    
    // Ctrl+,: Settings
    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Comma)) {
        m_show_settings_dialog = true;
    }
    
    // Forward shortcuts to grid layout for panel navigation
    m_grid_layout->HandleKeyboardShortcuts();
}

void App::Shutdown() {
    // Save configuration
    m_config->GetConfig().agents = m_agent_manager->GetAgents();
    m_config->Save("config.json");
    
    // Shutdown terminal embedding
    TerminalEmbed::Shutdown();
    
    // Cleanup ImGui
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    
    // Cleanup GLFW
    glfwDestroyWindow(m_window);
    glfwTerminate();
}

} // namespace AgentSmith

#include "app.h"
#include "terminal_embed.h"
#include "output_log.h"
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <algorithm>

#ifdef PLATFORM_WINDOWS
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#include <shlobj.h>  // For SHBrowseForFolder
#include <commdlg.h>
#include <direct.h>  // For _getcwd
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "ole32.lib")
#elif defined(PLATFORM_LINUX)
#define GLFW_EXPOSE_NATIVE_X11
#include <GLFW/glfw3native.h>
#elif defined(PLATFORM_MACOS)
#define GLFW_EXPOSE_NATIVE_COCOA
#include <GLFW/glfw3native.h>
#endif

namespace AgentSmith {

#ifdef PLATFORM_WINDOWS
// Windows folder browser dialog
static bool BrowseForFolder(char* outPath, size_t pathSize, const char* title = "Select Folder") {
    bool result = false;

    // Initialize COM (required for newer folder dialogs)
    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    if (FAILED(hr)) {
        return false;
    }

    BROWSEINFOA bi = {};
    bi.lpszTitle = title;
    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE | BIF_EDITBOX;

    PIDLIST_ABSOLUTE pidl = SHBrowseForFolderA(&bi);
    if (pidl != nullptr) {
        char path[MAX_PATH];
        if (SHGetPathFromIDListA(pidl, path)) {
            strncpy(outPath, path, pathSize - 1);
            outPath[pathSize - 1] = '\0';
            result = true;
        }
        CoTaskMemFree(pidl);
    }

    CoUninitialize();
    return result;
}
#else
// Stub for non-Windows platforms
static bool BrowseForFolder(char* outPath, size_t pathSize, const char* title = "Select Folder") {
    (void)outPath;
    (void)pathSize;
    (void)title;
    return false;  // Not implemented
}
#endif

std::string App::GetExecutableDirectory() {
#ifdef PLATFORM_WINDOWS
    char path[MAX_PATH];
    DWORD length = GetModuleFileNameA(nullptr, path, MAX_PATH);
    if (length > 0 && length < MAX_PATH) {
        std::string fullPath(path);
        size_t lastSlash = fullPath.find_last_of("\\/");
        if (lastSlash != std::string::npos) {
            return fullPath.substr(0, lastSlash + 1);
        }
    }
    return "";
#else
    // Linux/macOS: Use /proc/self/exe or _NSGetExecutablePath
    char path[PATH_MAX];
    ssize_t count = readlink("/proc/self/exe", path, PATH_MAX);
    if (count > 0) {
        path[count] = '\0';
        std::string fullPath(path);
        size_t lastSlash = fullPath.find_last_of('/');
        if (lastSlash != std::string::npos) {
            return fullPath.substr(0, lastSlash + 1);
        }
    }
    return "";
#endif
}

App::App() = default;
App::~App() = default;

bool App::Init() {
    // Log working directory for debugging build differences
    char cwd[512];
    if (_getcwd(cwd, sizeof(cwd))) {
        LOG_INFO_SRC("Working directory: " + std::string(cwd), "App");
    }
    LOG_INFO_SRC("Initializing...", "App");

    // Initialize GLFW
    if (!glfwInit()) {
        LOG_ERROR_SRC("Failed to initialize GLFW", "App");
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
        LOG_ERROR_SRC("Failed to create GLFW window", "App");
        glfwTerminate();
        return false;
    }
    LOG_INFO_SRC("Window created (1920x1080)", "App");

    glfwMakeContextCurrent(m_window);
    glfwSwapInterval(1);  // Enable vsync

    // Get native window handle for terminal embedding
    m_native_window = TerminalEmbed::GetNativeWindowHandle(m_window);

    // Initialize terminal embedding system
    if (!TerminalEmbed::Initialize()) {
        LOG_WARNING_SRC("Terminal embedding not available on this platform", "App");
    }

    // Initialize ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    // Setup platform/renderer backends
    ImGui_ImplGlfw_InitForOpenGL(m_window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    // Load configuration from executable directory
    m_config = std::make_unique<ConfigManager>();
    m_config_path = GetExecutableDirectory() + "config.json";
    LOG_INFO_SRC("Loading config from: " + m_config_path, "App");
    m_config->Load(m_config_path);

    // Load saved agents into tracker
    AgentsTracker::Instance().LoadAgents(m_config->GetConfig().agents);

    // Initialize grid layout
    m_grid_layout = std::make_unique<GridLayout>();
    m_grid_layout->SetConfig(m_config->GetGridConfig());

    // Set callback for "Add Agent" requests from empty slots
    m_grid_layout->SetAddAgentCallback([this]() {
        m_show_add_agent_dialog = true;
    });

    m_grid_layout->SyncWithAgents(AgentsTracker::Instance().GetAgents());

    // Initialize input manager
    m_input_manager = std::make_unique<InputManager>();

    // Load terminal font
    LoadTerminalFont();

    // Setup style
    SetupStyle();

    return true;
}

void App::LoadTerminalFont() {
    ImGuiIO& io = ImGui::GetIO();

    // Build glyph ranges for terminal use (includes Unicode blocks for spinners, boxes, etc.)
    ImFontGlyphRangesBuilder glyphRangeBuilder;
    glyphRangeBuilder.AddRanges(io.Fonts->GetGlyphRangesDefault());  // Basic Latin

    // Add box drawing characters (U+2500-U+257F)
    static const ImWchar boxDrawing[] = { 0x2500, 0x257F, 0 };
    glyphRangeBuilder.AddRanges(boxDrawing);

    // Add block elements (U+2580-U+259F)
    static const ImWchar blockElements[] = { 0x2580, 0x259F, 0 };
    glyphRangeBuilder.AddRanges(blockElements);

    // Add geometric shapes (U+25A0-U+25FF) - includes spinners
    static const ImWchar geometricShapes[] = { 0x25A0, 0x25FF, 0 };
    glyphRangeBuilder.AddRanges(geometricShapes);

    // Add misc symbols (U+2600-U+26FF)
    static const ImWchar miscSymbols[] = { 0x2600, 0x26FF, 0 };
    glyphRangeBuilder.AddRanges(miscSymbols);

    // Add arrows (U+2190-U+21FF)
    static const ImWchar arrows[] = { 0x2190, 0x21FF, 0 };
    glyphRangeBuilder.AddRanges(arrows);

    // Add Braille patterns (U+2800-U+28FF) - sometimes used for graphics
    static const ImWchar braille[] = { 0x2800, 0x28FF, 0 };
    glyphRangeBuilder.AddRanges(braille);

    // Add mathematical operators (U+2200-U+22FF)
    static const ImWchar mathOps[] = { 0x2200, 0x22FF, 0 };
    glyphRangeBuilder.AddRanges(mathOps);

    // Build the final ranges
    static ImVector<ImWchar> glyphRanges;
    glyphRangeBuilder.BuildRanges(&glyphRanges);

    // Try to load a monospace font for the terminal
#ifdef PLATFORM_WINDOWS
    const char* fontPaths[] = {
        "C:\\Windows\\Fonts\\consola.ttf",      // Consolas (good Unicode support)
        "C:\\Windows\\Fonts\\seguisym.ttf",     // Segoe UI Symbol (fallback for symbols)
        "C:\\Windows\\Fonts\\cour.ttf",         // Courier New
        "C:\\Windows\\Fonts\\lucon.ttf",        // Lucida Console
    };
#elif defined(PLATFORM_LINUX)
    const char* fontPaths[] = {
        "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf",
        "/usr/share/fonts/TTF/DejaVuSansMono.ttf",
        "/usr/share/fonts/truetype/liberation/LiberationMono-Regular.ttf",
    };
#elif defined(PLATFORM_MACOS)
    const char* fontPaths[] = {
        "/System/Library/Fonts/Menlo.ttc",
        "/System/Library/Fonts/Monaco.dfont",
        "/Library/Fonts/Courier New.ttf",
    };
#else
    const char* fontPaths[] = {};
#endif

    ImFontConfig fontConfig;
    fontConfig.OversampleH = 2;
    fontConfig.OversampleV = 2;

    // Load primary monospace font
    bool fontLoaded = false;
    for (const char* path : fontPaths) {
        ImFont* font = io.Fonts->AddFontFromFileTTF(path, 14.0f, &fontConfig, glyphRanges.Data);
        if (font) {
            fontLoaded = true;
            LOG_INFO_SRC("Loaded primary font: " + std::string(path), "App");
            break;
        }
    }

    if (!fontLoaded) {
        // Use default font
        io.Fonts->AddFontDefault();
        LOG_WARNING_SRC("Using default font (limited Unicode support)", "App");
    }

#ifdef PLATFORM_WINDOWS
    // Merge symbol fonts for better Unicode coverage
    // This adds symbols that may not be in the primary monospace font
    ImFontConfig mergeConfig;
    mergeConfig.MergeMode = true;
    mergeConfig.OversampleH = 2;
    mergeConfig.OversampleV = 2;

    // Extended symbol ranges for fallback fonts
    static const ImWchar symbolRanges[] = {
        0x2000, 0x206F,  // General Punctuation
        0x2100, 0x214F,  // Letterlike Symbols
        0x2190, 0x21FF,  // Arrows
        0x2200, 0x22FF,  // Mathematical Operators
        0x2300, 0x23FF,  // Miscellaneous Technical
        0x2400, 0x243F,  // Control Pictures
        0x2440, 0x245F,  // OCR
        0x2460, 0x24FF,  // Enclosed Alphanumerics
        0x2500, 0x257F,  // Box Drawing
        0x2580, 0x259F,  // Block Elements
        0x25A0, 0x25FF,  // Geometric Shapes
        0x2600, 0x26FF,  // Miscellaneous Symbols
        0x2700, 0x27BF,  // Dingbats
        0x2800, 0x28FF,  // Braille Patterns
        0x2900, 0x297F,  // Supplemental Arrows-B
        0x2B00, 0x2BFF,  // Miscellaneous Symbols and Arrows
        0xE000, 0xF8FF,  // Private Use Area (custom icons like Nerd Fonts)
        0
    };

    // Try Segoe UI Symbol for better symbol coverage
    const char* symbolFontPath = "C:\\Windows\\Fonts\\seguisym.ttf";
    if (io.Fonts->AddFontFromFileTTF(symbolFontPath, 14.0f, &mergeConfig, symbolRanges)) {
        LOG_INFO_SRC("Merged symbol font: " + std::string(symbolFontPath), "App");
    }

    // Try Segoe UI Emoji for emoji support
    const char* emojiFontPath = "C:\\Windows\\Fonts\\seguiemj.ttf";
    if (io.Fonts->AddFontFromFileTTF(emojiFontPath, 14.0f, &mergeConfig, symbolRanges)) {
        LOG_INFO_SRC("Merged emoji font: " + std::string(emojiFontPath), "App");
    }
#endif

    io.Fonts->Build();
}

void App::SetupStyle() {
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;

    // Get accent color from config
    const FColor& accent = m_config->GetConfig().theme.accent;
    float accent_r = accent.r;
    float accent_g = accent.g;
    float accent_b = accent.b;

    // Dark theme optimized for terminal viewing
    colors[ImGuiCol_WindowBg] = ImVec4(0.08f, 0.08f, 0.10f, 1.00f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.10f, 0.10f, 0.12f, 1.00f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.12f, 0.12f, 0.14f, 0.95f);

    colors[ImGuiCol_Header] = ImVec4(0.18f, 0.18f, 0.20f, 1.00f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.26f, 0.26f, 0.28f, 1.00f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.30f, 0.30f, 0.33f, 1.00f);

    // Accent color from config
    colors[ImGuiCol_Button] = ImVec4(accent_r, accent_g, accent_b, 1.00f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(
        (std::min)(accent_r * 1.15f, 1.0f),
        (std::min)(accent_g * 1.15f, 1.0f),
        (std::min)(accent_b * 1.15f, 1.0f),
        1.00f
    );
    colors[ImGuiCol_ButtonActive] = ImVec4(
        (std::min)(accent_r * 1.30f, 1.0f),
        (std::min)(accent_g * 1.30f, 1.0f),
        (std::min)(accent_b * 1.30f, 1.0f),
        1.00f
    );

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
    colors[ImGuiCol_TabHovered] = ImVec4(accent_r, accent_g, accent_b, 0.80f);
    colors[ImGuiCol_TabActive] = ImVec4(accent_r, accent_g, accent_b, 1.00f);

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
            AgentsTracker::Instance().UpdateAllGitInfo();
            m_git_update_timer = 0.0f;
        }

        // Update agent statuses
        AgentsTracker::Instance().Update();

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
        ImVec2 content_size(static_cast<float>(window_width),
                           static_cast<float>(window_height) - menu_bar_height);

        // Sync grid with current agents
        m_grid_layout->SyncWithAgents(AgentsTracker::Instance().GetAgents());

        // Render grid layout
        m_grid_layout->Render(content_pos, content_size, m_config->GetConfig().theme.accent);

        // Handle keyboard shortcuts for grid navigation
        m_grid_layout->HandleKeyboardShortcuts();

        // Render dialogs
        if (m_show_add_agent_dialog) RenderAddAgentDialog();
        if (m_show_settings_dialog) RenderSettingsDialog();
        if (m_show_about_dialog) RenderAboutDialog();
        if (m_show_clear_agents_dialog) RenderClearAgentsDialog();
        if (m_show_output_log) OutputLog::Instance().Render(&m_show_output_log);
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
                m_config->GetConfig().agents = AgentsTracker::Instance().SaveAgents();
                m_config->Save(m_config_path);
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Clear All Agents...")) {
                m_show_clear_agents_dialog = true;
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
                for (auto* window : m_grid_layout->GetAllWindows()) {
                    if (window->GetAgent() && !window->IsTerminalRunning()) {
                        window->LaunchTerminal();
                    }
                }
            }
            if (ImGui::MenuItem("Stop All")) {
                for (auto* window : m_grid_layout->GetAllWindows()) {
                    if (window->IsTerminalRunning()) {
                        window->TerminateTerminal();
                    }
                }
            }
            if (ImGui::MenuItem("Restart All")) {
                for (auto* window : m_grid_layout->GetAllWindows()) {
                    if (window->GetAgent()) {
                        window->RestartTerminal();
                    }
                }
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Refresh Git Info")) {
                AgentsTracker::Instance().UpdateAllGitInfo();
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Clear Attention Flags")) {
                AgentsTracker::Instance().ClearAttentionFlags();
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Help")) {
            if (ImGui::MenuItem("Output Log", "Ctrl+Shift+L")) {
                m_show_output_log = !m_show_output_log;
            }
            ImGui::Separator();
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

        auto running = AgentsTracker::Instance().GetRunningAgents();
        size_t running_count = running.size();

        ImGui::Text("%zu/%zu agents", running_count, AgentsTracker::Instance().GetAgentCount());

        ImGui::EndMainMenuBar();
    }
}

void App::RenderAddAgentDialog() {
    // Open modal popup
    if (m_show_add_agent_dialog) {
        ImGui::OpenPopup("Add Agent");
    }

    ImGui::SetNextWindowSize(ImVec2(450, 280), ImGuiCond_FirstUseEver);

    bool open = true;
    if (ImGui::BeginPopupModal("Add Agent", &open, ImGuiWindowFlags_NoResize)) {
        ImGui::Text("Agent Name:");
        ImGui::InputText("##AgentName", m_new_agent_name, sizeof(m_new_agent_name));

        ImGui::Spacing();

        ImGui::Text("Working Directory:");
        ImGui::InputText("##WorkDir", m_new_agent_dir, sizeof(m_new_agent_dir));
        ImGui::SameLine();
        if (ImGui::Button("Browse...")) {
            if (BrowseForFolder(m_new_agent_dir, sizeof(m_new_agent_dir), "Select Working Directory")) {
                LOG_INFO_SRC("Selected directory: " + std::string(m_new_agent_dir), "App");
            }
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
                AgentsTracker::Instance().CreateAgent(
                    m_new_agent_name,
                    m_new_agent_dir,
                    static_cast<AgentType>(m_new_agent_type)
                );
                m_grid_layout->SyncWithAgents(AgentsTracker::Instance().GetAgents());
                m_show_add_agent_dialog = false;

                // Reset form
                strcpy(m_new_agent_name, "New Agent");
                m_new_agent_dir[0] = '\0';
                m_new_agent_type = 0;

                ImGui::CloseCurrentPopup();
            }
        }

        ImGui::SameLine();

        if (ImGui::Button("Cancel", ImVec2(100, 30))) {
            m_show_add_agent_dialog = false;
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }

    // Handle close via X button
    if (!open) {
        m_show_add_agent_dialog = false;
    }
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

                // ImGui ColorEdit3 requires a float[3] array
                float accent[3] = { theme.accent.r, theme.accent.g, theme.accent.b };
                if (ImGui::ColorEdit3("Accent Color", accent)) {
                    theme.accent.r = accent[0];
                    theme.accent.g = accent[1];
                    theme.accent.b = accent[2];
                }

                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        if (ImGui::Button("Save", ImVec2(100, 30))) {
            m_config->Save(m_config_path);
            m_grid_layout->SetConfig(m_config->GetGridConfig());
            SetupStyle();  // Re-apply style with new theme colors
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
    ImGui::SetNextWindowSize(ImVec2(400, 350), ImGuiCond_FirstUseEver);

    if (ImGui::Begin("About AgentSmith", &m_show_about_dialog)) {
        ImGui::TextColored(ImVec4(0.3f, 0.8f, 0.4f, 1.0f), "AgentSmith");
        ImGui::Text("Version 0.2.0 - Embedded Terminal Edition");

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::TextWrapped(
            "A mission control interface for managing multiple AI coding agents. "
            "Monitor Claude Code, Aider, and other AI assistants in a unified grid view "
            "with real-time git status and embedded terminal support."
        );

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::Text("New in 0.2.0:");
        ImGui::BulletText("True embedded terminals using ConPTY");
        ImGui::BulletText("ANSI color and escape sequence support");
        ImGui::BulletText("Per-agent auto-accept edits toggle");
        ImGui::BulletText("Fullscreen mode (double-click)");
        ImGui::BulletText("Agent info popup");

        ImGui::Spacing();
        ImGui::Text("Future Features:");
        ImGui::BulletText("Working file visualizer per agent");
        ImGui::BulletText("Token usage tracking");
        ImGui::BulletText("Diff viewer for changes");
        ImGui::BulletText("Agent communication/coordination");

        ImGui::Spacing();

        if (ImGui::Button("Close", ImVec2(-1, 30))) {
            m_show_about_dialog = false;
        }
    }
    ImGui::End();
}

void App::RenderClearAgentsDialog() {
    ImGui::OpenPopup("Clear All Agents");

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(400, 180));

    if (ImGui::BeginPopupModal("Clear All Agents", nullptr, ImGuiWindowFlags_NoResize)) {
        size_t agentCount = AgentsTracker::Instance().GetAgentCount();

        ImGui::TextWrapped("Are you sure you want to remove all %zu agent(s)?", agentCount);
        ImGui::Spacing();
        ImGui::TextColored(ImVec4(0.9f, 0.6f, 0.3f, 1.0f), "This will terminate all terminal sessions and clear the saved configuration.");

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        if (ImGui::Button("Clear All", ImVec2(120, 30))) {
            // Stop all agents first
            AgentsTracker::Instance().StopAllAgents();

            // Clear all agents from tracker (remove from back to front for efficiency)
            while (AgentsTracker::Instance().GetAgentCount() > 0) {
                AgentsTracker::Instance().RemoveAgentByIndex(0);
            }

            // Sync grid layout
            m_grid_layout->SyncWithAgents(AgentsTracker::Instance().GetAgents());

            // Save empty config
            m_config->GetConfig().agents.clear();
            m_config->Save(m_config_path);

            LOG_INFO_SRC("Cleared all agents", "App");

            m_show_clear_agents_dialog = false;
            ImGui::CloseCurrentPopup();
        }

        ImGui::SameLine();

        if (ImGui::Button("Cancel", ImVec2(120, 30))) {
            m_show_clear_agents_dialog = false;
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
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

    // Ctrl+Shift+L: Output Log (Shift to avoid conflict with terminal's Ctrl+L clear)
    if (io.KeyCtrl && io.KeyShift && ImGui::IsKeyPressed(ImGuiKey_L)) {
        m_show_output_log = !m_show_output_log;
    }

    // Grid layout handles its own shortcuts (Ctrl+Tab, Ctrl+1-9, etc.)
}

void App::Shutdown() {
    // Save configuration
    m_config->GetConfig().agents = AgentsTracker::Instance().SaveAgents();
    m_config->Save(m_config_path);

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

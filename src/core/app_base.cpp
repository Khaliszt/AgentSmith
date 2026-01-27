// C:\FarfadetsCorp\AgentSmith\src\core\app_base.cpp

#include "core/app_base.h"
#include "logging/log_macros.h"
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <iostream>
#include <thread>

namespace smith::core {

AppBase::AppBase() = default;
AppBase::~AppBase() = default;

int AppBase::Run(const Config& config) {
    if (!Initialize(config)) {
        return 1;
    }

    if (!OnStartUp()) {
        Shutdown();
        return 1;
    }

    MainLoop(config);

    OnShutDown();
    Shutdown();

    return 0;
}

bool AppBase::Initialize(const Config& config) {
    // Set error callback
    glfwSetErrorCallback(ErrorCallback);

    // Initialize GLFW
    if (!glfwInit()) {
        OnError("Failed to initialize GLFW");
        return false;
    }

    // OpenGL 3.3 Core
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // Window hints
    glfwWindowHint(GLFW_DECORATED, config.decorated ? GLFW_TRUE : GLFW_FALSE);
    glfwWindowHint(GLFW_RESIZABLE, config.resizable ? GLFW_TRUE : GLFW_FALSE);
    glfwWindowHint(GLFW_FLOATING, config.alwaysOnTop ? GLFW_TRUE : GLFW_FALSE);
    glfwWindowHint(GLFW_MAXIMIZED, config.maximized ? GLFW_TRUE : GLFW_FALSE);

    // Create window
    m_window = glfwCreateWindow(
        config.width, config.height,
        config.title.c_str(),
        nullptr, nullptr
    );

    if (!m_window) {
        OnError("Failed to create GLFW window");
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(m_window);
    glfwSwapInterval(config.vsync ? 1 : 0);

    // Set window callbacks
    glfwSetWindowUserPointer(m_window, this);
    glfwSetFramebufferSizeCallback(m_window, FramebufferCallback);
    glfwSetDropCallback(m_window, DropCallback);
    glfwSetWindowFocusCallback(m_window, FocusCallback);

    // Get framebuffer size for DPI scaling
    int fbWidth, fbHeight;
    glfwGetFramebufferSize(m_window, &fbWidth, &fbHeight);
    glfwGetWindowSize(m_window, &m_windowWidth, &m_windowHeight);
    m_dpiScale = static_cast<float>(fbWidth) / static_cast<float>(m_windowWidth);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();

    // Note: Docking and Viewports require ImGui docking branch
    // For Phase 1, we use standard ImGui
    (void)config.enableDocking;
    (void)config.enableViewports;

    // Set ImGui ini file path
    if (!config.imguiIniPath.empty()) {
        io.IniFilename = config.imguiIniPath.c_str();
    }

    // Setup platform/renderer bindings
    ImGui_ImplGlfw_InitForOpenGL(m_window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    // Load fonts
    LoadFonts(config);

    // Apply style
    ApplyStyle();

    m_startTime = std::chrono::steady_clock::now();
    m_lastFrameTime = m_startTime;
    m_initialized = true;

    return true;
}

void AppBase::MainLoop(const Config& config) {
    while (!glfwWindowShouldClose(m_window) && !m_exitRequested) {
        // Calculate delta time
        auto currentTime = std::chrono::steady_clock::now();
        std::chrono::duration<float> deltaSeconds = currentTime - m_lastFrameTime;
        m_deltaTime = deltaSeconds.count();
        m_lastFrameTime = currentTime;

        // Poll events
        glfwPollEvents();

        // Update logic
        OnUpdate(m_deltaTime);

        // Begin frame
        BeginFrame();

        // Render UI
        OnImGuiRender();

        // End frame
        EndFrame();

        m_frameCount++;

        // Frame rate limiting
        if (config.targetFps > 0) {
            float targetFrameTime = 1.0f / config.targetFps;
            std::chrono::duration<float> elapsedSeconds =
                std::chrono::steady_clock::now() - currentTime;

            if (elapsedSeconds.count() < targetFrameTime) {
                float sleepTime = targetFrameTime - elapsedSeconds.count();
                std::this_thread::sleep_for(
                    std::chrono::duration<float>(sleepTime)
                );
            }
        }
    }
}

void AppBase::Shutdown() {
    if (m_initialized) {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
    }

    if (m_window) {
        glfwDestroyWindow(m_window);
        m_window = nullptr;
    }

    glfwTerminate();
}

void AppBase::BeginFrame() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // Note: Docking disabled - requires ImGui docking branch
    // TODO: Switch to ImGui docking branch in Phase 1 completion
}

void AppBase::EndFrame() {
    // Rendering
    ImGui::Render();
    int display_w, display_h;
    glfwGetFramebufferSize(m_window, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    // Note: Viewports disabled - requires ImGui docking branch

    glfwSwapBuffers(m_window);
}

void AppBase::LoadFonts(const Config& config) {
    ImGuiIO& io = ImGui::GetIO();

    // Load main font
    if (!config.fontPath.empty()) {
        io.Fonts->AddFontFromFileTTF(config.fontPath.c_str(), config.fontSize);
    } else {
        // Use default font with specified size
        ImFontConfig fontConfig;
        fontConfig.SizePixels = config.fontSize;
        io.Fonts->AddFontDefault(&fontConfig);
    }

    // Load fallback fonts
    for (const auto& fallbackPath : config.fallbackFontPaths) {
        io.Fonts->AddFontFromFileTTF(fallbackPath.c_str(), config.fontSize);
    }

    io.Fonts->Build();
}

void AppBase::ApplyStyle() {
    // Catppuccin Macchiato inspired style
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;

    colors[ImGuiCol_WindowBg] = ImVec4(0.14f, 0.15f, 0.23f, 1.00f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.18f, 0.20f, 0.25f, 1.00f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.14f, 0.15f, 0.23f, 0.94f);
    colors[ImGuiCol_Border] = ImVec4(0.36f, 0.38f, 0.47f, 0.50f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.18f, 0.20f, 0.25f, 1.00f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.22f, 0.24f, 0.29f, 1.00f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.27f, 0.29f, 0.35f, 1.00f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.14f, 0.15f, 0.23f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.18f, 0.20f, 0.25f, 1.00f);
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.18f, 0.20f, 0.25f, 1.00f);
    colors[ImGuiCol_Button] = ImVec4(0.54f, 0.68f, 0.96f, 0.40f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.54f, 0.68f, 0.96f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.47f, 0.58f, 0.82f, 1.00f);
    colors[ImGuiCol_Header] = ImVec4(0.54f, 0.68f, 0.96f, 0.31f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.54f, 0.68f, 0.96f, 0.80f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.54f, 0.68f, 0.96f, 1.00f);
    colors[ImGuiCol_Tab] = ImVec4(0.18f, 0.20f, 0.25f, 1.00f);
    colors[ImGuiCol_TabHovered] = ImVec4(0.54f, 0.68f, 0.96f, 0.80f);
    colors[ImGuiCol_TabActive] = ImVec4(0.54f, 0.68f, 0.96f, 1.00f);
    colors[ImGuiCol_Text] = ImVec4(0.79f, 0.83f, 0.96f, 1.00f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);

    style.WindowRounding = 5.0f;
    style.FrameRounding = 3.0f;
    style.ScrollbarRounding = 3.0f;
    style.GrabRounding = 3.0f;
    style.TabRounding = 3.0f;
}

double AppBase::GetTotalTime() const {
    auto currentTime = std::chrono::steady_clock::now();
    std::chrono::duration<double> elapsed = currentTime - m_startTime;
    return elapsed.count();
}

void AppBase::OnError(const std::string& message) {
    std::cerr << "AppBase Error: " << message << std::endl;
}

// === Static Callbacks ===

void AppBase::ErrorCallback(int error, const char* description) {
    std::cerr << "GLFW Error " << error << ": " << description << std::endl;
}

void AppBase::FramebufferCallback(GLFWwindow* window, int width, int height) {
    auto* app = static_cast<AppBase*>(glfwGetWindowUserPointer(window));
    if (app) {
        app->m_windowWidth = width;
        app->m_windowHeight = height;
        app->OnResize(width, height);
    }
}

void AppBase::DropCallback(GLFWwindow* window, int count, const char** paths) {
    auto* app = static_cast<AppBase*>(glfwGetWindowUserPointer(window));
    if (app) {
        std::vector<std::string> pathsVec;
        for (int i = 0; i < count; i++) {
            pathsVec.push_back(paths[i]);
        }
        app->OnFileDrop(pathsVec);
    }
}

void AppBase::FocusCallback(GLFWwindow* window, int focused) {
    auto* app = static_cast<AppBase*>(glfwGetWindowUserPointer(window));
    if (app) {
        app->m_focused = (focused == GLFW_TRUE);
        app->OnFocusChanged(app->m_focused);
    }
}

} // namespace smith::core

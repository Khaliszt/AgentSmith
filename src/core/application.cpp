// C:\FarfadetsCorp\AgentSmith\src\core\application.cpp

#include "core/application.h"
#include "logging/logger.h"
#include "logging/log_sink.h"
#include "logging/log_macros.h"
#include <filesystem>
#include <imgui.h>

#ifdef _WIN32
#include <Windows.h>
#include <libloaderapi.h>
#endif

namespace smith::core {

Application& Application::Instance() {
    static Application instance;
    return instance;
}

int Application::Main(int argc, char** argv) {
    (void)argc;
    (void)argv;

    Application& app = Instance();

    AppBase::Config config;
    config.title = "AgentSmith v2.0";
    config.width = 1920;
    config.height = 1080;
    config.maximized = true;
    config.enableDocking = true;
    config.fontSize = 16.0f;

    return app.Run(config);
}

Application::Application() = default;
Application::~Application() = default;

bool Application::OnStartUp() {
    DiscoverPaths();
    InitializeLogging();

    SMITH_INFO(logging::Category::Core, "AgentSmith v2.0 starting");
    SMITH_INFO(logging::Category::Core, "Executable directory: {}", m_exeDir);
    SMITH_INFO(logging::Category::Core, "Resources directory: {}", m_resourcesDir);
    SMITH_INFO(logging::Category::Core, "Config path: {}", m_configPath);

    return true;
}

void Application::OnUpdate(float dt) {
    (void)dt;
    // Update logic will go here
}

void Application::OnImGuiRender() {
    // Main menu bar
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Exit", "Alt+F4")) {
                RequestExit();
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("View")) {
            ImGui::MenuItem("Demo Window", nullptr, &m_showDemoWindow);
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Help")) {
            if (ImGui::MenuItem("About")) {
                // TODO: Show about dialog
            }
            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }

    // Show demo window if enabled
    if (m_showDemoWindow) {
        ImGui::ShowDemoWindow(&m_showDemoWindow);
    }

    // Main content window
    ImGui::Begin("AgentSmith", nullptr, ImGuiWindowFlags_NoCollapse);
    ImGui::Text("AgentSmith v2.0 - Phase 1: Foundation");
    ImGui::Separator();
    ImGui::Text("Logging system: Active");
    ImGui::Text("AppBase framework: Active");
    ImGui::Text("Frame time: %.3f ms (%.1f FPS)", GetDeltaTime() * 1000.0f, 1.0f / GetDeltaTime());
    ImGui::Text("Total time: %.1f seconds", GetTotalTime());
    ImGui::End();
}

void Application::OnShutDown() {
    SaveState();
    SMITH_INFO(logging::Category::Core, "AgentSmith shutting down");
    logging::Logger::Instance().Flush();
}

void Application::OnResize(int width, int height) {
    SMITH_DEBUG(logging::Category::Core, "Window resized to {}x{}", width, height);
}

void Application::OnFileDrop(const std::vector<std::string>& paths) {
    SMITH_INFO(logging::Category::Core, "Files dropped: {}", paths.size());
    for (const auto& path : paths) {
        SMITH_DEBUG(logging::Category::Core, "  - {}", path);
    }
}

void Application::OnError(const std::string& message) {
    SMITH_ERROR(logging::Category::Core, "Application error: {}", message);
    AppBase::OnError(message);
}

void Application::DiscoverPaths() {
#ifdef _WIN32
    char exePath[MAX_PATH];
    GetModuleFileNameA(nullptr, exePath, MAX_PATH);
    std::filesystem::path exeFullPath(exePath);
    m_exeDir = exeFullPath.parent_path().string();
#else
    // For Unix-like systems
    m_exeDir = std::filesystem::current_path().string();
#endif

    m_resourcesDir = m_exeDir + "/resources";
    m_configPath = m_exeDir + "/config.json";
}

void Application::InitializeLogging() {
    auto& logger = logging::Logger::Instance();

    // Add console sink for development
    auto consoleSink = std::make_shared<logging::ConsoleSink>(true);
    logger.AddSink(consoleSink);

    // Add file sink
    std::string logPath = m_exeDir + "/agentsmith.log";
    auto fileSink = std::make_shared<logging::FileSink>(logPath, 10*1024*1024, 3);
    logger.AddSink(fileSink);

    // Add ImGui sink for in-app log viewer
    auto imguiSink = std::make_shared<logging::ImGuiSink>(1000);
    logger.AddSink(imguiSink);

    // Set log level
    logger.SetMinLevel(logging::LogLevel::Debug);

    SMITH_INFO(logging::Category::Core, "Logging system initialized");
}

void Application::SaveState() {
    // State saving will be implemented when config system is ready
}

} // namespace smith::core

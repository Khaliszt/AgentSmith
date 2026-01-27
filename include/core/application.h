// C:\FarfadetsCorp\AgentSmith\include\core\application.h

#pragma once

#include "core/app_base.h"
#include <memory>

namespace smith {
    namespace config { class ConfigManager; }
    namespace logging { class Logger; }
    namespace agent { class AgentTracker; class AgentProviderRegistry; }
    namespace ui { class GridLayout; class MenuBar; class OutputLogPanel; }
}

namespace smith::core {

/**
 * Application - AgentSmith main application singleton
 *
 * Owns all subsystems and coordinates their lifecycle.
 */
class Application final : public AppBase {
public:
    /**
     * Get singleton instance
     */
    static Application& Instance();

    /**
     * Main entry point (call from main())
     */
    static int Main(int argc, char** argv);

    // === Paths ===

    const std::string& GetExecutableDir() const { return m_exeDir; }
    const std::string& GetResourcesDir() const { return m_resourcesDir; }
    const std::string& GetConfigPath() const { return m_configPath; }

protected:
    bool OnStartUp() override;
    void OnUpdate(float dt) override;
    void OnImGuiRender() override;
    void OnShutDown() override;
    void OnResize(int width, int height) override;
    void OnFileDrop(const std::vector<std::string>& paths) override;
    void OnError(const std::string& message) override;

private:
    Application();
    ~Application();

    void DiscoverPaths();
    void InitializeLogging();
    void SaveState();

    // Paths
    std::string m_exeDir;
    std::string m_resourcesDir;
    std::string m_configPath;

    // Runtime state
    bool m_showDemoWindow = false;
};

// === Global Accessors (convenience) ===

inline Application& App() { return Application::Instance(); }

} // namespace smith::core

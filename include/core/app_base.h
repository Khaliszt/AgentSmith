// C:\FarfadetsCorp\AgentSmith\include\core\app_base.h

#pragma once

#include <string>
#include <chrono>
#include <memory>
#include <vector>

struct GLFWwindow;

namespace smith::core {

/**
 * AppBase - Reusable Dear ImGui application framework
 *
 * Handles all boilerplate:
 * - GLFW window creation with error handling
 * - OpenGL 3.3 Core context
 * - Dear ImGui with docking enabled
 * - Main loop with configurable frame rate
 * - Graceful shutdown
 *
 * Thread Safety: Main thread only (enforced)
 */
class AppBase {
public:
    struct Config {
        // Window settings
        std::string title = "AgentSmith";
        int width = 1920;
        int height = 1080;
        bool vsync = true;
        bool maximized = false;
        bool decorated = true;
        bool resizable = true;
        bool alwaysOnTop = false;

        // ImGui settings
        std::string imguiIniPath = "";
        bool enableDocking = true;
        bool enableViewports = false;  // Multi-window (experimental)

        // Font settings
        float fontSize = 16.0f;
        std::string fontPath = "";
        std::vector<std::string> fallbackFontPaths = {};

        // Performance
        int targetFps = 60;  // 0 = unlimited
        bool reduceWhenUnfocused = true;
    };

    AppBase();
    virtual ~AppBase();

    // Delete copy/move
    AppBase(const AppBase&) = delete;
    AppBase& operator=(const AppBase&) = delete;

    /**
     * Initialize and run the application
     * @param config Application configuration
     * @return Exit code (0 = success)
     */
    int Run(const Config& config);

    /**
     * Request graceful exit
     */
    void RequestExit() { m_exitRequested = true; }
    bool IsExitRequested() const { return m_exitRequested; }

    // === Accessors ===

    GLFWwindow* GetWindow() const { return m_window; }
    float GetDeltaTime() const { return m_deltaTime; }
    double GetTotalTime() const;
    int GetWindowWidth() const { return m_windowWidth; }
    int GetWindowHeight() const { return m_windowHeight; }
    float GetDpiScale() const { return m_dpiScale; }
    bool IsFocused() const { return m_focused; }
    int GetFrameCount() const { return m_frameCount; }

protected:
    // === Override Points ===

    /**
     * Called once after initialization, before main loop
     * Use for: loading configuration, creating subsystems
     * @return false to abort startup
     */
    virtual bool OnStartUp() { return true; }

    /**
     * Called every frame before rendering
     * Use for: non-UI updates, polling, timers
     * @param dt Delta time in seconds
     */
    virtual void OnUpdate(float dt) { (void)dt; }

    /**
     * Called every frame for ImGui rendering
     * Use for: all UI code
     */
    virtual void OnImGuiRender() {}

    /**
     * Called before shutdown
     * Use for: saving state, cleanup
     */
    virtual void OnShutDown() {}

    /**
     * Called on window resize
     */
    virtual void OnResize(int width, int height) { (void)width; (void)height; }

    /**
     * Called on file drop
     */
    virtual void OnFileDrop(const std::vector<std::string>& paths) { (void)paths; }

    /**
     * Called on focus change
     */
    virtual void OnFocusChanged(bool focused) { (void)focused; }

    /**
     * Called for unhandled errors (can override for custom handling)
     */
    virtual void OnError(const std::string& message);

private:
    bool Initialize(const Config& config);
    void MainLoop(const Config& config);
    void Shutdown();

    void BeginFrame();
    void EndFrame();
    void LoadFonts(const Config& config);
    void ApplyStyle();

    // GLFW callbacks
    static void ErrorCallback(int error, const char* description);
    static void FramebufferCallback(GLFWwindow* window, int w, int h);
    static void DropCallback(GLFWwindow* window, int count, const char** paths);
    static void FocusCallback(GLFWwindow* window, int focused);

    GLFWwindow* m_window = nullptr;
    bool m_exitRequested = false;
    bool m_initialized = false;
    bool m_focused = true;

    int m_windowWidth = 0;
    int m_windowHeight = 0;
    float m_dpiScale = 1.0f;
    float m_deltaTime = 0.0f;
    int m_frameCount = 0;

    std::chrono::steady_clock::time_point m_startTime;
    std::chrono::steady_clock::time_point m_lastFrameTime;
};

} // namespace smith::core

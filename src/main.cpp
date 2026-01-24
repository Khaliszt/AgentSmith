//=============================================================================
// AgentSmith - Multi-Agent Viewer and Controller
//
// A mission control interface for managing multiple AI coding agents
// with embedded terminals and real-time status monitoring.
//=============================================================================

#include "app.h"
#include "output_log.h"

#ifdef PLATFORM_WINDOWS
#include <windows.h>
#endif

// Standard main entry point (used by non-Windows or console mode)
int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    AgentSmith::OutputLog::Instance().Info("AgentSmith starting...", "Main");

    AgentSmith::App app;

    if (!app.Init()) {
        AgentSmith::OutputLog::Instance().Error("Failed to initialize AgentSmith", "Main");
        return 1;
    }

    AgentSmith::OutputLog::Instance().Info("Initialization complete", "Main");

    app.Run();
    app.Shutdown();

    AgentSmith::OutputLog::Instance().Info("AgentSmith shutdown complete", "Main");

    return 0;
}

#ifdef PLATFORM_WINDOWS
// Windows GUI entry point (no console window)
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    (void)hInstance;
    (void)hPrevInstance;
    (void)lpCmdLine;
    (void)nCmdShow;

    return main(0, nullptr);
}
#endif

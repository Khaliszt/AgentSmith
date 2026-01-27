//=============================================================================
// AgentSmith v2.0 - Multi-Agent Mission Control
//
// A mission control interface for managing multiple AI coding agents
// with embedded terminals and real-time status monitoring.
//=============================================================================

#include "core/application.h"

int main(int argc, char** argv) {
    return smith::core::Application::Main(argc, argv);
}

#ifdef _WIN32
#include <Windows.h>
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    return main(__argc, __argv);
}
#endif

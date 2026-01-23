//=============================================================================
// AgentSmith - Multi-Agent Viewer and Controller
// 
// A mission control interface for managing multiple AI coding agents
// with embedded terminals and real-time status monitoring.
//=============================================================================

#include "app.h"
#include <iostream>

int main(int argc, char* argv[]) {
    AgentSmith::App app;
    
    if (!app.Init()) {
        std::cerr << "Failed to initialize AgentSmith" << std::endl;
        return 1;
    }
    
    app.Run();
    app.Shutdown();
    
    return 0;
}

#pragma once

#include "types.h"
#include <string>

namespace AgentSmith {

//=============================================================================
// Config Manager
//
// Handles loading and saving application configuration to JSON.
//=============================================================================

class ConfigManager {
public:
    ConfigManager();
    ~ConfigManager() = default;
    
    // Load/save configuration
    bool Load(const std::string& filepath);
    bool Save(const std::string& filepath);
    
    // Access configuration
    AppConfig& GetConfig() { return m_config; }
    const AppConfig& GetConfig() const { return m_config; }
    
    // Quick access helpers
    GridConfig& GetGridConfig() { return m_config.grid; }
    
private:
    AppConfig m_config;
    std::string m_config_path;
    
    void SetDefaults();
};

} // namespace AgentSmith

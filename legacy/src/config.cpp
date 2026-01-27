#include "config.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>

using json = nlohmann::json;

namespace AgentSmith {

ConfigManager::ConfigManager() {
    SetDefaults();
}

void ConfigManager::SetDefaults() {
    m_config.window_width = 1920;
    m_config.window_height = 1080;
    m_config.fullscreen = false;
    m_config.dark_mode = true;
    
    m_config.grid.rows = 2;
    m_config.grid.cols = 2;
    m_config.grid.panel_padding = 4.0f;
    m_config.grid.info_overlay_height = 28.0f;
    m_config.grid.show_git_info = true;
    m_config.grid.show_status_indicator = true;
    m_config.grid.show_file_activity = false;
    
#ifdef PLATFORM_WINDOWS
    m_config.default_terminal_command = "wt.exe";
    m_config.default_shell = "powershell.exe";
#elif defined(PLATFORM_LINUX)
    m_config.default_terminal_command = "gnome-terminal";
    m_config.default_shell = "bash";
#elif defined(PLATFORM_MACOS)
    m_config.default_terminal_command = "Terminal.app";
    m_config.default_shell = "zsh";
#endif
    
    m_config.theme.accent = FColor(0.3f, 0.7f, 0.4f);

    // Terminal theme defaults
    m_config.default_terminal_theme = "Catppuccin Macchiato";
    m_config.agent_themes.claude_code = "";
    m_config.agent_themes.aider = "";
    m_config.agent_themes.cursor = "";
    m_config.agent_themes.chatgpt = "";
    m_config.agent_themes.grok = "";
    m_config.agent_themes.custom = "";
}

bool ConfigManager::Load(const std::string& filepath) {
    m_config_path = filepath;
    
    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "Config file not found, using defaults: " << filepath << std::endl;
        return false;
    }
    
    try {
        json j;
        file >> j;
        
        // Window settings
        if (j.contains("window")) {
            auto& w = j["window"];
            if (w.contains("width")) m_config.window_width = w["width"];
            if (w.contains("height")) m_config.window_height = w["height"];
            if (w.contains("fullscreen")) m_config.fullscreen = w["fullscreen"];
            if (w.contains("dark_mode")) m_config.dark_mode = w["dark_mode"];
        }
        
        // Grid settings
        if (j.contains("grid")) {
            auto& g = j["grid"];
            if (g.contains("rows")) m_config.grid.rows = g["rows"];
            if (g.contains("cols")) m_config.grid.cols = g["cols"];
            if (g.contains("panel_padding")) m_config.grid.panel_padding = g["panel_padding"];
            if (g.contains("info_overlay_height")) m_config.grid.info_overlay_height = g["info_overlay_height"];
            if (g.contains("show_git_info")) m_config.grid.show_git_info = g["show_git_info"];
            if (g.contains("show_status_indicator")) m_config.grid.show_status_indicator = g["show_status_indicator"];
            if (g.contains("show_file_activity")) m_config.grid.show_file_activity = g["show_file_activity"];
        }
        
        // Terminal settings
        if (j.contains("terminal")) {
            auto& t = j["terminal"];
            if (t.contains("command")) m_config.default_terminal_command = t["command"];
            if (t.contains("shell")) m_config.default_shell = t["shell"];
            if (t.contains("default_theme")) m_config.default_terminal_theme = t["default_theme"];
        }

        // Agent-type specific themes
        if (j.contains("agent_themes")) {
            auto& at = j["agent_themes"];
            if (at.contains("claude_code")) m_config.agent_themes.claude_code = at["claude_code"];
            if (at.contains("aider")) m_config.agent_themes.aider = at["aider"];
            if (at.contains("cursor")) m_config.agent_themes.cursor = at["cursor"];
            if (at.contains("chatgpt")) m_config.agent_themes.chatgpt = at["chatgpt"];
            if (at.contains("grok")) m_config.agent_themes.grok = at["grok"];
            if (at.contains("custom")) m_config.agent_themes.custom = at["custom"];
        }

        // Theme
        if (j.contains("theme")) {
            auto& th = j["theme"];
            if (th.contains("accent")) {
                auto& a = th["accent"];
                if (a.is_array() && a.size() >= 3) {
                    m_config.theme.accent.r = a[0];
                    m_config.theme.accent.g = a[1];
                    m_config.theme.accent.b = a[2];
                }
            }
        }
        
        // Agents
        if (j.contains("agents") && j["agents"].is_array()) {
            m_config.agents.clear();
            
            for (const auto& agent_json : j["agents"]) {
                Agent agent;
                
                if (agent_json.contains("id")) agent.id = agent_json["id"];
                if (agent_json.contains("name")) agent.name = agent_json["name"];
                if (agent_json.contains("working_directory")) agent.working_directory = agent_json["working_directory"];
                if (agent_json.contains("command")) agent.command = agent_json["command"];
                if (agent_json.contains("type")) agent.type = static_cast<AgentType>(agent_json["type"].get<int>());
                if (agent_json.contains("grid_row")) agent.grid_row = agent_json["grid_row"];
                if (agent_json.contains("grid_col")) agent.grid_col = agent_json["grid_col"];
                
                if (agent_json.contains("args") && agent_json["args"].is_array()) {
                    for (const auto& arg : agent_json["args"]) {
                        agent.args.push_back(arg);
                    }
                }

                if (agent_json.contains("terminal_theme")) {
                    agent.terminal_theme = agent_json["terminal_theme"];
                }

                m_config.agents.push_back(std::move(agent));
            }
        }
        
        return true;
        
    } catch (const json::exception& e) {
        std::cerr << "Error parsing config: " << e.what() << std::endl;
        return false;
    }
}

bool ConfigManager::Save(const std::string& filepath) {
    json j;
    
    // Window settings
    j["window"] = {
        {"width", m_config.window_width},
        {"height", m_config.window_height},
        {"fullscreen", m_config.fullscreen},
        {"dark_mode", m_config.dark_mode}
    };
    
    // Grid settings
    j["grid"] = {
        {"rows", m_config.grid.rows},
        {"cols", m_config.grid.cols},
        {"panel_padding", m_config.grid.panel_padding},
        {"info_overlay_height", m_config.grid.info_overlay_height},
        {"show_git_info", m_config.grid.show_git_info},
        {"show_status_indicator", m_config.grid.show_status_indicator},
        {"show_file_activity", m_config.grid.show_file_activity}
    };
    
    // Terminal settings
    j["terminal"] = {
        {"command", m_config.default_terminal_command},
        {"shell", m_config.default_shell},
        {"default_theme", m_config.default_terminal_theme}
    };

    // Agent-type specific themes
    j["agent_themes"] = {
        {"claude_code", m_config.agent_themes.claude_code},
        {"aider", m_config.agent_themes.aider},
        {"cursor", m_config.agent_themes.cursor},
        {"chatgpt", m_config.agent_themes.chatgpt},
        {"grok", m_config.agent_themes.grok},
        {"custom", m_config.agent_themes.custom}
    };
    
    // Theme
    j["theme"] = {
        {"accent", {m_config.theme.accent.r, m_config.theme.accent.g, m_config.theme.accent.b}}
    };
    
    // Agents
    j["agents"] = json::array();
    for (const auto& agent : m_config.agents) {
        json agent_json;
        agent_json["id"] = agent.id;
        agent_json["name"] = agent.name;
        agent_json["working_directory"] = agent.working_directory;
        agent_json["command"] = agent.command;
        agent_json["type"] = static_cast<int>(agent.type);
        agent_json["grid_row"] = agent.grid_row;
        agent_json["grid_col"] = agent.grid_col;
        agent_json["args"] = agent.args;
        if (!agent.terminal_theme.empty()) {
            agent_json["terminal_theme"] = agent.terminal_theme;
        }
        j["agents"].push_back(agent_json);
    }
    
    std::ofstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "Failed to save config: " << filepath << std::endl;
        return false;
    }
    
    file << j.dump(2);
    return true;
}

} // namespace AgentSmith

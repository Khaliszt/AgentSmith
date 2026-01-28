// C:\FarfadetsCorp\AgentSmith\src\terminal\terminal_theme.cpp

#include "terminal/terminal_theme.h"
#include <sstream>
#include <iomanip>

namespace smith::terminal {

std::string TerminalTheme::ToJson() const {
    std::ostringstream oss;
    oss << "{\n";
    oss << "  \"background\": \"" << background << "\",\n";
    oss << "  \"foreground\": \"" << foreground << "\",\n";
    oss << "  \"cursor\": \"" << cursor << "\",\n";
    oss << "  \"selection\": \"" << selection << "\",\n";
    oss << "  \"black\": \"" << ansi[0] << "\",\n";
    oss << "  \"red\": \"" << ansi[1] << "\",\n";
    oss << "  \"green\": \"" << ansi[2] << "\",\n";
    oss << "  \"yellow\": \"" << ansi[3] << "\",\n";
    oss << "  \"blue\": \"" << ansi[4] << "\",\n";
    oss << "  \"magenta\": \"" << ansi[5] << "\",\n";
    oss << "  \"cyan\": \"" << ansi[6] << "\",\n";
    oss << "  \"white\": \"" << ansi[7] << "\",\n";
    oss << "  \"brightBlack\": \"" << ansi[8] << "\",\n";
    oss << "  \"brightRed\": \"" << ansi[9] << "\",\n";
    oss << "  \"brightGreen\": \"" << ansi[10] << "\",\n";
    oss << "  \"brightYellow\": \"" << ansi[11] << "\",\n";
    oss << "  \"brightBlue\": \"" << ansi[12] << "\",\n";
    oss << "  \"brightMagenta\": \"" << ansi[13] << "\",\n";
    oss << "  \"brightCyan\": \"" << ansi[14] << "\",\n";
    oss << "  \"brightWhite\": \"" << ansi[15] << "\"\n";
    oss << "}";
    return oss.str();
}

uint32_t TerminalTheme::HexToImGui(const std::string& hexColor) {
    if (hexColor.empty() || hexColor[0] != '#' || hexColor.length() != 7) {
        return 0xFF000000;  // Default to black if invalid
    }

    // Parse hex color "#RRGGBB"
    std::string hexStr = hexColor.substr(1);
    uint32_t rgb = 0;
    std::istringstream iss(hexStr);
    iss >> std::hex >> rgb;

    // Extract RGB components
    uint8_t r = (rgb >> 16) & 0xFF;
    uint8_t g = (rgb >> 8) & 0xFF;
    uint8_t b = rgb & 0xFF;

    // ImGui uses ABGR format (alpha, blue, green, red)
    return 0xFF000000 | (b << 16) | (g << 8) | r;
}

TerminalTheme TerminalTheme::GetDefault() {
    // Catppuccin Macchiato theme
    TerminalTheme theme;
    theme.name = "Catppuccin Macchiato";
    theme.background = "#24273a";
    theme.foreground = "#cad3f5";
    theme.cursor = "#f4dbd6";
    theme.selection = "#5b6078";

    // ANSI colors (normal)
    theme.ansi[0] = "#494d64";  // black
    theme.ansi[1] = "#ed8796";  // red
    theme.ansi[2] = "#a6da95";  // green
    theme.ansi[3] = "#eed49f";  // yellow
    theme.ansi[4] = "#8aadf4";  // blue
    theme.ansi[5] = "#f5bde6";  // magenta
    theme.ansi[6] = "#8bd5ca";  // cyan
    theme.ansi[7] = "#b8c0e0";  // white

    // ANSI colors (bright)
    theme.ansi[8] = "#5b6078";  // bright black
    theme.ansi[9] = "#ed8796";  // bright red
    theme.ansi[10] = "#a6da95"; // bright green
    theme.ansi[11] = "#eed49f"; // bright yellow
    theme.ansi[12] = "#8aadf4"; // bright blue
    theme.ansi[13] = "#f5bde6"; // bright magenta
    theme.ansi[14] = "#8bd5ca"; // bright cyan
    theme.ansi[15] = "#a5adcb"; // bright white

    return theme;
}

} // namespace smith::terminal

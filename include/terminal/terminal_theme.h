// C:\FarfadetsCorp\AgentSmith\include\terminal\terminal_theme.h

#pragma once

#include <string>
#include <cstdint>

namespace smith::terminal {

/**
 * @brief Terminal color theme
 *
 * Defines colors for terminal background, foreground, cursor, selection,
 * and ANSI color palette (16 colors: 0-7 normal, 8-15 bright).
 */
struct TerminalTheme {
    std::string name;

    // Base colors (hex format: "#RRGGBB")
    std::string background;   // Background color
    std::string foreground;   // Default text color
    std::string cursor;       // Cursor color
    std::string selection;    // Selection background

    // ANSI color palette (16 colors)
    // 0-7: black, red, green, yellow, blue, magenta, cyan, white
    // 8-15: bright variants
    std::string ansi[16];

    /**
     * @brief Convert theme to JSON string for xterm.js
     *
     * @return JSON representation of theme
     */
    std::string ToJson() const;

    /**
     * @brief Convert hex color to ImGui ABGR uint32_t
     *
     * @param hexColor Hex color string ("#RRGGBB")
     * @return ABGR color value
     */
    static uint32_t HexToImGui(const std::string& hexColor);

    /**
     * @brief Get background color as ImGui uint32_t
     *
     * @return Background color in ABGR format
     */
    uint32_t GetBackgroundImGui() const {
        return HexToImGui(background);
    }

    /**
     * @brief Get foreground color as ImGui uint32_t
     *
     * @return Foreground color in ABGR format
     */
    uint32_t GetForegroundImGui() const {
        return HexToImGui(foreground);
    }

    /**
     * @brief Get default "Catppuccin Macchiato" theme
     *
     * @return Default terminal theme
     */
    static TerminalTheme GetDefault();
};

} // namespace smith::terminal

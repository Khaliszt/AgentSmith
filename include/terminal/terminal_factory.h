// C:\FarfadetsCorp\AgentSmith\include\terminal\terminal_factory.h

#pragma once

#include "terminal_interface.h"
#include <memory>
#include <string>

#ifdef _WIN32
#include <Windows.h>
#endif

namespace smith::terminal {

/**
 * @brief Terminal backend types
 */
enum class TerminalBackend {
    Auto,      // Auto-detect best available
    WebView2,  // WebView2 + xterm.js (preferred)
    ImGui      // ImGui fallback (deferred to Phase 5)
};

/**
 * @brief Factory for creating terminal instances
 *
 * The factory automatically selects the best available terminal backend
 * based on system capabilities:
 * - WebView2 + xterm.js (preferred, if WebView2 runtime is installed)
 * - ImGui fallback (for systems without WebView2, Phase 5)
 *
 * During Phase 3, if WebView2 is unavailable, the factory temporarily
 * falls back to ConPTYTerminal directly until the ImGui fallback is
 * implemented in Phase 5.
 */
class TerminalFactory {
public:
    /**
     * @brief Check if WebView2 runtime is available
     *
     * Queries the Windows system for the WebView2 runtime installation.
     * WebView2 is available on Windows 10 1803+ and Windows 11.
     *
     * @return True if WebView2 runtime is installed
     */
    static bool IsWebView2Available();

    /**
     * @brief Get WebView2 version string
     *
     * Returns the version of the installed WebView2 runtime, or an empty
     * string if WebView2 is not available.
     *
     * @return WebView2 version (e.g., "110.0.1587.50") or empty string
     */
    static std::string GetWebView2Version();

    /**
     * @brief Create terminal with specified or auto-detected backend
     *
     * Creates a terminal instance using the specified backend, or automatically
     * selects the best available backend if TerminalBackend::Auto is used.
     *
     * Auto-selection logic:
     * 1. Check if WebView2 is available
     * 2. If available, create WebViewTerminal
     * 3. Otherwise, fall back to ConPTYTerminal (temporary until Phase 5)
     *
     * @param backend Terminal backend to use (default: Auto)
     * @param parentHwnd Parent window handle for WebView2 (optional)
     * @return Unique pointer to terminal instance, or nullptr on failure
     */
    static std::unique_ptr<ITerminal> Create(
        TerminalBackend backend = TerminalBackend::Auto,
        void* parentHwnd = nullptr);

    /**
     * @brief Convert backend enum to string
     *
     * @param backend Terminal backend
     * @return Backend name as string
     */
    static const char* BackendToString(TerminalBackend backend);

    /**
     * @brief Get the currently selected backend (for diagnostics)
     *
     * Returns the backend that would be selected with Auto mode.
     *
     * @return Detected backend type
     */
    static TerminalBackend DetectBestBackend();
};

} // namespace smith::terminal

// C:\FarfadetsCorp\AgentSmith\src\terminal\terminal_factory.cpp

#include "terminal/terminal_factory.h"
#include "terminal/conpty_terminal.h"
#include "terminal/webview_terminal.h"
#include "logging/logger.h"

#ifdef _WIN32
#include <WebView2.h>
#include <comdef.h>
#endif

namespace smith::terminal {

bool TerminalFactory::IsWebView2Available() {
#ifdef _WIN32
    LPWSTR version = nullptr;
    HRESULT hr = GetAvailableCoreWebView2BrowserVersionString(nullptr, &version);
    bool available = SUCCEEDED(hr) && version != nullptr;

    if (version) {
        CoTaskMemFree(version);
    }

    return available;
#else
    return false;
#endif
}

std::string TerminalFactory::GetWebView2Version() {
#ifdef _WIN32
    LPWSTR version = nullptr;
    HRESULT hr = GetAvailableCoreWebView2BrowserVersionString(nullptr, &version);

    if (SUCCEEDED(hr) && version != nullptr) {
        // Convert LPWSTR to std::string
        int size_needed = WideCharToMultiByte(CP_UTF8, 0, version, -1, nullptr, 0, nullptr, nullptr);
        std::string result(size_needed - 1, 0);
        WideCharToMultiByte(CP_UTF8, 0, version, -1, &result[0], size_needed, nullptr, nullptr);

        CoTaskMemFree(version);
        return result;
    }

    return "";
#else
    return "";
#endif
}

const char* TerminalFactory::BackendToString(TerminalBackend backend) {
    switch (backend) {
        case TerminalBackend::Auto:     return "Auto";
        case TerminalBackend::WebView2: return "WebView2";
        case TerminalBackend::ImGui:    return "ImGui";
        default:                        return "Unknown";
    }
}

TerminalBackend TerminalFactory::DetectBestBackend() {
#ifdef _WIN32
    if (IsWebView2Available()) {
        return TerminalBackend::WebView2;
    }
#endif

    // Fall back to ImGui (or ConPTY temporarily in Phase 3)
    return TerminalBackend::ImGui;
}

std::unique_ptr<ITerminal> TerminalFactory::Create(
    TerminalBackend backend,
    void* parentHwnd)
{
    // Auto-detect backend if requested
    if (backend == TerminalBackend::Auto) {
        backend = DetectBestBackend();
        SMITH_INFO(logging::Category::Terminal,
                   "Auto-detected terminal backend: {}", BackendToString(backend));
    }

    // Create terminal based on backend
    switch (backend) {
        case TerminalBackend::WebView2: {
#ifdef _WIN32
            if (IsWebView2Available()) {
                std::string version = GetWebView2Version();
                SMITH_INFO(logging::Category::Terminal,
                           "Creating WebView2 terminal (version: {})", version);

                // WebViewTerminal requires a parent HWND for WebView2 hosting
                if (!parentHwnd) {
                    SMITH_WARN(logging::Category::Terminal,
                               "No parent HWND provided for WebView2, falling back to ConPTY");
                    auto terminal = std::make_unique<ConPTYTerminal>();
                    return terminal;
                }

                auto terminal = std::make_unique<WebViewTerminal>();
                SMITH_INFO(logging::Category::Terminal, "Created WebViewTerminal");
                return terminal;
            } else {
                SMITH_WARN(logging::Category::Terminal,
                           "WebView2 runtime not available, falling back to ConPTY");
                auto terminal = std::make_unique<ConPTYTerminal>();
                return terminal;
            }
#else
            SMITH_ERROR(logging::Category::Terminal,
                        "WebView2 only available on Windows");
#endif
            // Fall through to ImGui fallback
            [[fallthrough]];
        }

        case TerminalBackend::ImGui: {
            // Phase 5: ImGui terminal fallback
            // For now, use ConPTY as fallback
            SMITH_INFO(logging::Category::Terminal,
                       "ImGui terminal fallback not yet implemented (Phase 5), using ConPTY");

            auto terminal = std::make_unique<ConPTYTerminal>();
            SMITH_INFO(logging::Category::Terminal, "Created ConPTYTerminal fallback");
            return terminal;
        }

        default:
            SMITH_ERROR(logging::Category::Terminal,
                        "Unknown terminal backend: {}", static_cast<int>(backend));
            return nullptr;
    }
}

} // namespace smith::terminal

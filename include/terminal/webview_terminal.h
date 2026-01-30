// C:\FarfadetsCorp\AgentSmith\include\terminal\webview_terminal.h

#pragma once

#include "terminal_interface.h"
#include "terminal_theme.h"
#include "conpty_terminal.h"
#include <memory>
#include <string>
#include <vector>
#include <mutex>
#include <atomic>

#ifdef _WIN32
#include <Windows.h>
#include <wrl.h>
#include <wil/com.h>
#include "WebView2.h"
#endif

namespace smith::terminal {

/**
 * @brief WebView2 + xterm.js Terminal Implementation
 *
 * This terminal combines:
 * - WebView2: Renders xterm.js for professional terminal display
 * - ConPTY: Manages process lifecycle and I/O
 * - JavaScript Bridge: Connects the two (C++ ↔ xterm.js)
 *
 * Data flow:
 * - User input: xterm.js → JS postMessage → C++ → ConPTY → Process
 * - Process output: Process → ConPTY → C++ callback → JS terminalApi.write → xterm.js
 *
 * Requirements:
 * - Windows 10 version 1809+ (for ConPTY)
 * - WebView2 Runtime installed (Edge 90+)
 * - resources/web/ directory with xterm.js files
 */
class WebViewTerminal : public ITerminal {
public:
    WebViewTerminal();
    ~WebViewTerminal() override;

    // Disable copy
    WebViewTerminal(const WebViewTerminal&) = delete;
    WebViewTerminal& operator=(const WebViewTerminal&) = delete;

    /**
     * @brief Initialize WebView2 control
     *
     * This must be called before Launch(). It creates the WebView2 environment
     * and controller, then loads terminal.html from the resources directory.
     *
     * @param parentHwnd Parent window handle for WebView2
     * @param resourcesPath Path to resources/web/ directory
     * @return True if initialization succeeded
     */
    bool Initialize(HWND parentHwnd, const std::string& resourcesPath);

    /**
     * @brief Check if WebView2 is ready
     *
     * @return True if WebView2 is initialized and terminal.html loaded
     */
    bool IsInitialized() const { return m_initialized; }

    // === ITerminal Interface - Process Lifecycle ===

    bool Launch(const std::string& command,
               const std::vector<std::string>& args,
               const std::string& workingDir,
               int cols, int rows) override;

    void Terminate() override;
    bool IsRunning() const override;
    int GetExitCode() const override;
    int GetProcessId() const override;

    // === ITerminal Interface - I/O ===

    void Write(const std::string& text) override;
    void Write(const char* data, size_t length) override;
    void SetOutputCallback(OutputCallback callback) override;
    void SetExitCallback(ExitCallback callback) override;
    void SetErrorCallback(ErrorCallback callback) override;

    // === ITerminal Interface - Display Size ===

    void Resize(int cols, int rows) override;
    int GetCols() const override;
    int GetRows() const override;
    void Clear() override;

    // === ITerminal Interface - Rendering ===

#ifdef _WIN32
    HWND GetNativeHandle() const override;
#endif
    void SetBounds(int x, int y, int width, int height) override;
    void SetVisible(bool visible) override;
    void Focus() override;

    // === ITerminal Interface - Capabilities ===

    bool HasNativeRendering() const override { return true; }
    bool SupportsSelection() const override { return true; }

    // === ITerminal Interface - Clipboard ===

    std::string GetSelectedText() const override;
    void ClearSelection() override;
    void SelectAll() override;
    void Copy() override;
    void Paste(const std::string& text) override;

    // === ITerminal Interface - Scrolling ===

    void Scroll(int lines) override;
    void ScrollToTop() override;
    void ScrollToBottom() override;

    // === ITerminal Interface - Theme ===

    void SetTheme(const TerminalTheme& theme) override;
    const TerminalTheme& GetTheme() const override;

private:
#ifdef _WIN32
    // WebView2 COM interfaces (using wil::com_ptr for RAII)
    wil::com_ptr<ICoreWebView2Environment> m_webViewEnvironment;
    wil::com_ptr<ICoreWebView2Controller> m_webViewController;
    wil::com_ptr<ICoreWebView2> m_webView;

    HWND m_parentHwnd = nullptr;

    // Event registration tokens for cleanup
    EventRegistrationToken m_navigationToken{};
    EventRegistrationToken m_messageToken{};
#endif

    // ConPTY backend for process management
    std::unique_ptr<ConPTYTerminal> m_conpty;

    // State
    std::atomic<bool> m_initialized{false};
    std::atomic<bool> m_webViewReady{false};  // Set when JS sends 'ready' message
    std::string m_resourcesPath;

    // Bounds for WebView2 window
    int m_x = 0;
    int m_y = 0;
    int m_width = 800;
    int m_height = 600;

    // Terminal size (passed through to ConPTY)
    int m_cols = 120;
    int m_rows = 30;

    // Theme
    TerminalTheme m_theme;

    // Thread safety for JavaScript execution
    mutable std::mutex m_jsMutex;

    // Internal methods - WebView2 setup
    void OnWebView2EnvironmentCreated(HRESULT result,
                                      ICoreWebView2Environment* environment);
    void OnWebView2ControllerCreated(HRESULT result,
                                     ICoreWebView2Controller* controller);
    void OnNavigationCompleted(ICoreWebView2* sender,
                              ICoreWebView2NavigationCompletedEventArgs* args);
    void OnWebMessageReceived(ICoreWebView2* sender,
                             ICoreWebView2WebMessageReceivedEventArgs* args);

    // Internal methods - JavaScript bridge
    void ExecuteJavaScript(const std::string& script);
    void WriteToXterm(const char* data, size_t length);
    void HandleInputFromXterm(const std::string& input);
    void HandleResizeFromXterm(int cols, int rows);

    // Internal methods - ConPTY callbacks
    void OnConPTYOutput(const std::string& output);
    void OnConPTYExit(int exitCode);
    void OnConPTYError(const std::string& error);

    // User callbacks (forwarded from ConPTY)
    OutputCallback m_userOutputCallback;
    ExitCallback m_userExitCallback;
    ErrorCallback m_userErrorCallback;
    mutable std::mutex m_callbackMutex;

    // Utility
    std::string EscapeForJavaScript(const std::string& text) const;
    std::string WStringToString(const std::wstring& wstr) const;
    std::wstring StringToWString(const std::string& str) const;
};

} // namespace smith::terminal

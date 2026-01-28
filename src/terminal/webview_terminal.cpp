// C:\FarfadetsCorp\AgentSmith\src\terminal\webview_terminal.cpp

#include "terminal/webview_terminal.h"
#include "logging/logger.h"
#include <sstream>
#include <iomanip>
#include <algorithm>

#ifdef _WIN32
#include <ShlObj.h>
#include <nlohmann/json.hpp>

using namespace Microsoft::WRL;
#endif

namespace smith::terminal {

WebViewTerminal::WebViewTerminal() {
    // Initialize ConPTY backend
    m_conpty = std::make_unique<ConPTYTerminal>();

    // Set up ConPTY callbacks to forward to our JavaScript bridge
    m_conpty->SetOutputCallback([this](const std::string& output) {
        OnConPTYOutput(output);
    });

    m_conpty->SetExitCallback([this](int exitCode) {
        OnConPTYExit(exitCode);
    });

    m_conpty->SetErrorCallback([this](const std::string& error) {
        OnConPTYError(error);
    });

    // Initialize with default theme
    m_theme = TerminalTheme::GetDefault();

    SMITH_INFO(smith::logging::Category::Terminal, "WebViewTerminal created");
}

WebViewTerminal::~WebViewTerminal() {
    Terminate();

#ifdef _WIN32
    // Clean up WebView2
    if (m_webViewController) {
        m_webViewController->Close();
        m_webViewController = nullptr;
    }

    if (m_hwnd) {
        DestroyWindow(m_hwnd);
        m_hwnd = nullptr;
    }
#endif

    SMITH_INFO(smith::logging::Category::Terminal, "WebViewTerminal destroyed");
}

#ifdef _WIN32

bool WebViewTerminal::Initialize(HWND parentHwnd, const std::string& resourcesPath) {
    if (m_initialized) {
        SMITH_WARN(smith::logging::Category::WebView, "WebViewTerminal already initialized");
        return true;
    }

    m_parentHwnd = parentHwnd;
    m_resourcesPath = resourcesPath;

    SMITH_INFO(smith::logging::Category::WebView, "Initializing WebView2 with resources path: {}", resourcesPath);

    // Create WebView2 environment
    auto envCallback = Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
        [this](HRESULT result, ICoreWebView2Environment* environment) -> HRESULT {
            OnWebView2EnvironmentCreated(result, environment);
            return S_OK;
        });

    // Use default user data folder
    HRESULT hr = CreateCoreWebView2EnvironmentWithOptions(
        nullptr,  // Browser executable folder (use default Edge)
        nullptr,  // User data folder (use default)
        nullptr,  // Environment options
        envCallback.Get());

    if (FAILED(hr)) {
        SMITH_ERROR(smith::logging::Category::WebView, "Failed to create WebView2 environment: 0x{:X}", static_cast<unsigned>(hr));
        return false;
    }

    // Note: initialization completes asynchronously via callbacks
    return true;
}

void WebViewTerminal::OnWebView2EnvironmentCreated(HRESULT result,
                                                    ICoreWebView2Environment* environment) {
    if (FAILED(result)) {
        SMITH_ERROR(smith::logging::Category::WebView, "WebView2 environment creation failed: 0x{:X}", static_cast<unsigned>(result));
        return;
    }

    m_webViewEnvironment = environment;
    SMITH_INFO(smith::logging::Category::WebView, "WebView2 environment created");

    // Create WebView2 controller
    auto controllerCallback = Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
        [this](HRESULT result, ICoreWebView2Controller* controller) -> HRESULT {
            OnWebView2ControllerCreated(result, controller);
            return S_OK;
        });

    HRESULT hr = m_webViewEnvironment->CreateCoreWebView2Controller(m_parentHwnd, controllerCallback.Get());

    if (FAILED(hr)) {
        SMITH_ERROR(smith::logging::Category::WebView, "Failed to create WebView2 controller: 0x{:X}", static_cast<unsigned>(hr));
    }
}

void WebViewTerminal::OnWebView2ControllerCreated(HRESULT result,
                                                   ICoreWebView2Controller* controller) {
    if (FAILED(result)) {
        SMITH_ERROR(smith::logging::Category::WebView, "WebView2 controller creation failed: 0x{:X}", static_cast<unsigned>(result));
        return;
    }

    m_webViewController = controller;
    m_webViewController->get_CoreWebView2(&m_webView);

    SMITH_INFO(smith::logging::Category::WebView, "WebView2 controller created");

    // Get the HWND for the WebView2 window
    m_webViewController->get_ParentWindow(&m_hwnd);

    // Set initial bounds
    RECT bounds = { m_x, m_y, m_x + m_width, m_y + m_height };
    m_webViewController->put_Bounds(bounds);

    // Register navigation completed handler
    m_webView->add_NavigationCompleted(
        Callback<ICoreWebView2NavigationCompletedEventHandler>(
            [this](ICoreWebView2* sender, ICoreWebView2NavigationCompletedEventArgs* args) -> HRESULT {
                OnNavigationCompleted(sender, args);
                return S_OK;
            }).Get(),
        nullptr);

    // Register web message handler (for JS → C++ communication)
    m_webView->add_WebMessageReceived(
        Callback<ICoreWebView2WebMessageReceivedEventHandler>(
            [this](ICoreWebView2* sender, ICoreWebView2WebMessageReceivedEventArgs* args) -> HRESULT {
                OnWebMessageReceived(sender, args);
                return S_OK;
            }).Get(),
        nullptr);

    // Navigate to terminal.html
    std::wstring htmlPath = StringToWString(m_resourcesPath + "/terminal.html");

    // Convert to file:// URL
    std::wstring fileUrl = L"file:///" + htmlPath;
    // Replace backslashes with forward slashes
    std::replace(fileUrl.begin(), fileUrl.end(), L'\\', L'/');

    SMITH_INFO(smith::logging::Category::WebView, "Navigating to: {}", WStringToString(fileUrl));

    m_webView->Navigate(fileUrl.c_str());

    m_initialized = true;
}

void WebViewTerminal::OnNavigationCompleted(ICoreWebView2* sender,
                                            ICoreWebView2NavigationCompletedEventArgs* args) {
    BOOL success = FALSE;
    args->get_IsSuccess(&success);

    if (success) {
        SMITH_INFO(smith::logging::Category::WebView, "WebView2 navigation completed successfully");

        // Apply current theme
        SetTheme(m_theme);
    } else {
        SMITH_ERROR(smith::logging::Category::WebView, "WebView2 navigation failed");

        COREWEBVIEW2_WEB_ERROR_STATUS errorStatus;
        args->get_WebErrorStatus(&errorStatus);
        SMITH_ERROR(smith::logging::Category::WebView, "Navigation error status: {}", static_cast<int>(errorStatus));
    }
}

void WebViewTerminal::OnWebMessageReceived(ICoreWebView2* sender,
                                           ICoreWebView2WebMessageReceivedEventArgs* args) {
    wil::unique_cotaskmem_string messageRaw;
    args->get_WebMessageAsJson(&messageRaw);

    std::string messageJson = WStringToString(messageRaw.get());

    try {
        auto json = nlohmann::json::parse(messageJson);
        std::string type = json["type"];

        if (type == "ready") {
            SMITH_INFO(smith::logging::Category::WebView, "xterm.js terminal ready");
            m_webViewReady = true;

            // Apply theme again now that terminal is ready
            SetTheme(m_theme);
        }
        else if (type == "input") {
            std::string data = json["data"];
            HandleInputFromXterm(data);
        }
        else if (type == "resize") {
            int cols = json["cols"];
            int rows = json["rows"];
            HandleResizeFromXterm(cols, rows);
        }
        else {
            SMITH_WARN(smith::logging::Category::WebView, "Unknown message type: {}", type);
        }
    }
    catch (const std::exception& e) {
        SMITH_ERROR(smith::logging::Category::WebView, "Failed to parse web message: {}", e.what());
    }
}

#else
// Non-Windows stub implementation

bool WebViewTerminal::Initialize(HWND parentHwnd, const std::string& resourcesPath) {
    (void)parentHwnd;
    (void)resourcesPath;
    SMITH_ERROR(smith::logging::Category::WebView, "WebView2 not available on this platform");
    return false;
}

#endif

// === ITerminal Interface - Process Lifecycle ===

bool WebViewTerminal::Launch(const std::string& command,
                              const std::vector<std::string>& args,
                              const std::string& workingDir,
                              int cols,
                              int rows) {
    if (!m_initialized) {
        SMITH_ERROR(smith::logging::Category::Terminal, "WebViewTerminal not initialized - call Initialize() first");
        return false;
    }

    m_cols = cols;
    m_rows = rows;

    SMITH_INFO(smith::logging::Category::Terminal, "Launching process: {} with {}x{} terminal", command, cols, rows);

    // Launch via ConPTY
    bool success = m_conpty->Launch(command, args, workingDir, cols, rows);

    if (!success) {
        SMITH_ERROR(smith::logging::Category::Terminal, "Failed to launch process via ConPTY");
    }

    return success;
}

void WebViewTerminal::Terminate() {
    if (m_conpty) {
        m_conpty->Terminate();
    }
}

bool WebViewTerminal::IsRunning() const {
    return m_conpty && m_conpty->IsRunning();
}

int WebViewTerminal::GetExitCode() const {
    return m_conpty ? m_conpty->GetExitCode() : -1;
}

int WebViewTerminal::GetProcessId() const {
    return m_conpty ? m_conpty->GetProcessId() : -1;
}

// === ITerminal Interface - I/O ===

void WebViewTerminal::Write(const std::string& text) {
    if (m_conpty) {
        m_conpty->Write(text);
    }
}

void WebViewTerminal::Write(const char* data, size_t length) {
    if (m_conpty) {
        m_conpty->Write(data, length);
    }
}

void WebViewTerminal::SetOutputCallback(OutputCallback callback) {
    std::lock_guard<std::mutex> lock(m_callbackMutex);
    m_userOutputCallback = std::move(callback);
}

void WebViewTerminal::SetExitCallback(ExitCallback callback) {
    std::lock_guard<std::mutex> lock(m_callbackMutex);
    m_userExitCallback = std::move(callback);
}

void WebViewTerminal::SetErrorCallback(ErrorCallback callback) {
    std::lock_guard<std::mutex> lock(m_callbackMutex);
    m_userErrorCallback = std::move(callback);
}

// === ITerminal Interface - Display Size ===

void WebViewTerminal::Resize(int cols, int rows) {
    m_cols = cols;
    m_rows = rows;

    // Resize ConPTY
    if (m_conpty) {
        m_conpty->Resize(cols, rows);
    }

    // Resize xterm.js via JavaScript
    if (m_webViewReady) {
        std::ostringstream js;
        js << "window.terminalApi.resize(" << cols << ", " << rows << ");";
        ExecuteJavaScript(js.str());
    }
}

int WebViewTerminal::GetCols() const {
    return m_cols;
}

int WebViewTerminal::GetRows() const {
    return m_rows;
}

void WebViewTerminal::Clear() {
    if (m_webViewReady) {
        ExecuteJavaScript("window.terminalApi.clear();");
    }

    // Also clear ConPTY (sends ANSI clear sequence)
    if (m_conpty) {
        m_conpty->Clear();
    }
}

// === ITerminal Interface - Rendering ===

#ifdef _WIN32
HWND WebViewTerminal::GetNativeHandle() const {
    return m_hwnd;
}
#endif

void WebViewTerminal::SetBounds(int x, int y, int width, int height) {
    m_x = x;
    m_y = y;
    m_width = width;
    m_height = height;

#ifdef _WIN32
    if (m_webViewController) {
        RECT bounds = { x, y, x + width, y + height };
        m_webViewController->put_Bounds(bounds);
    }
#endif
}

void WebViewTerminal::SetVisible(bool visible) {
#ifdef _WIN32
    if (m_webViewController) {
        m_webViewController->put_IsVisible(visible ? TRUE : FALSE);
    }
#else
    (void)visible;
#endif
}

void WebViewTerminal::Focus() {
    if (m_webViewReady) {
        ExecuteJavaScript("window.terminalApi.focus();");
    }
}

// === ITerminal Interface - Clipboard ===

std::string WebViewTerminal::GetSelectedText() const {
    // This would require synchronous JS execution, which is complex
    // For now, return empty - selection is handled by xterm.js internally
    return "";
}

void WebViewTerminal::ClearSelection() {
    if (m_webViewReady) {
        ExecuteJavaScript("window.terminalApi.clearSelection();");
    }
}

void WebViewTerminal::SelectAll() {
    if (m_webViewReady) {
        ExecuteJavaScript("window.terminalApi.selectAll();");
    }
}

void WebViewTerminal::Copy() {
    // xterm.js handles copy via browser's clipboard API
    // We can trigger it via JavaScript
    if (m_webViewReady) {
        ExecuteJavaScript("document.execCommand('copy');");
    }
}

void WebViewTerminal::Paste(const std::string& text) {
    // Send pasted text as input to the process
    Write(text);
}

// === ITerminal Interface - Scrolling ===

void WebViewTerminal::Scroll(int lines) {
    (void)lines;
    // xterm.js handles scrolling internally via mouse/keyboard
    // We don't need to expose this explicitly
}

void WebViewTerminal::ScrollToTop() {
    if (m_webViewReady) {
        ExecuteJavaScript("window.terminalApi.scrollToTop();");
    }
}

void WebViewTerminal::ScrollToBottom() {
    if (m_webViewReady) {
        ExecuteJavaScript("window.terminalApi.scrollToBottom();");
    }
}

// === ITerminal Interface - Theme ===

void WebViewTerminal::SetTheme(const TerminalTheme& theme) {
    m_theme = theme;

    // Apply theme to ConPTY (stores it for retrieval)
    if (m_conpty) {
        m_conpty->SetTheme(theme);
    }

    // Apply theme to xterm.js
    if (m_webViewReady) {
        std::string themeJson = theme.ToJson();
        std::string js = "window.terminalApi.setTheme(" + themeJson + ");";
        ExecuteJavaScript(js);

        SMITH_INFO(smith::logging::Category::Terminal, "Applied theme: {}", theme.name);
    }
}

const TerminalTheme& WebViewTerminal::GetTheme() const {
    return m_theme;
}

// === Internal Methods - JavaScript Bridge ===

void WebViewTerminal::ExecuteJavaScript(const std::string& script) {
#ifdef _WIN32
    if (!m_webView) {
        return;
    }

    std::lock_guard<std::mutex> lock(m_jsMutex);

    std::wstring wideScript = StringToWString(script);
    m_webView->ExecuteScript(wideScript.c_str(), nullptr);
#else
    (void)script;
#endif
}

void WebViewTerminal::WriteToXterm(const char* data, size_t length) {
    if (!m_webViewReady) {
        return;
    }

    // Escape the data for JavaScript
    std::string text(data, length);
    std::string escaped = EscapeForJavaScript(text);

    // Call terminalApi.write()
    std::string js = "window.terminalApi.write(\"" + escaped + "\");";
    ExecuteJavaScript(js);
}

void WebViewTerminal::HandleInputFromXterm(const std::string& input) {
    // Forward input to ConPTY (which sends it to the process)
    if (m_conpty) {
        m_conpty->Write(input);
    }
}

void WebViewTerminal::HandleResizeFromXterm(int cols, int rows) {
    // xterm.js has resized (e.g., via FitAddon)
    // Update our size and resize ConPTY to match
    m_cols = cols;
    m_rows = rows;

    if (m_conpty) {
        m_conpty->Resize(cols, rows);
    }

    SMITH_DEBUG(smith::logging::Category::Terminal, "Terminal resized to {}x{}", cols, rows);
}

// === Internal Methods - ConPTY Callbacks ===

void WebViewTerminal::OnConPTYOutput(const std::string& output) {
    // Forward output to xterm.js
    WriteToXterm(output.c_str(), output.length());

    // Also forward to user callback if set
    {
        std::lock_guard<std::mutex> lock(m_callbackMutex);
        if (m_userOutputCallback) {
            m_userOutputCallback(output);
        }
    }
}

void WebViewTerminal::OnConPTYExit(int exitCode) {
    SMITH_INFO(smith::logging::Category::Terminal, "Process exited with code {}", exitCode);

    std::lock_guard<std::mutex> lock(m_callbackMutex);
    if (m_userExitCallback) {
        m_userExitCallback(exitCode);
    }
}

void WebViewTerminal::OnConPTYError(const std::string& error) {
    SMITH_ERROR(smith::logging::Category::Terminal, "ConPTY error: {}", error);

    std::lock_guard<std::mutex> lock(m_callbackMutex);
    if (m_userErrorCallback) {
        m_userErrorCallback(error);
    }
}

// === Utility Methods ===

std::string WebViewTerminal::EscapeForJavaScript(const std::string& text) const {
    std::ostringstream escaped;

    for (char c : text) {
        switch (c) {
            case '\\': escaped << "\\\\"; break;
            case '\"': escaped << "\\\""; break;
            case '\'': escaped << "\\'"; break;
            case '\n': escaped << "\\n"; break;
            case '\r': escaped << "\\r"; break;
            case '\t': escaped << "\\t"; break;
            case '\b': escaped << "\\b"; break;
            case '\f': escaped << "\\f"; break;
            default:
                if (c < 0x20) {
                    // Escape control characters
                    escaped << "\\x" << std::hex << std::setw(2) << std::setfill('0')
                            << static_cast<int>(static_cast<unsigned char>(c));
                } else {
                    escaped << c;
                }
                break;
        }
    }

    return escaped.str();
}

std::string WebViewTerminal::WStringToString(const std::wstring& wstr) const {
#ifdef _WIN32
    if (wstr.empty()) return "";

    int sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), static_cast<int>(wstr.length()),
                                         nullptr, 0, nullptr, nullptr);
    std::string result(sizeNeeded, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), static_cast<int>(wstr.length()),
                        &result[0], sizeNeeded, nullptr, nullptr);
    return result;
#else
    (void)wstr;
    return "";
#endif
}

std::wstring WebViewTerminal::StringToWString(const std::string& str) const {
#ifdef _WIN32
    if (str.empty()) return L"";

    int sizeNeeded = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), static_cast<int>(str.length()),
                                         nullptr, 0);
    std::wstring result(sizeNeeded, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), static_cast<int>(str.length()),
                        &result[0], sizeNeeded);
    return result;
#else
    (void)str;
    return L"";
#endif
}

} // namespace smith::terminal

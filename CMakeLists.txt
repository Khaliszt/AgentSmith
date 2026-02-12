cmake_minimum_required(VERSION 3.16)
project(AgentSmith VERSION 2.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Output binaries to bin/ folder
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR}/bin)

# Options
option(SMITH_BUILD_TESTS "Build tests" ON)
option(SMITH_USE_WEBVIEW2 "Enable WebView2" ON)

# Find required packages
find_package(OpenGL REQUIRED)

# Include cmake modules
include(cmake/CompilerFlags.cmake)
include(cmake/Dependencies.cmake)
include(cmake/Version.cmake)

# Dependencies are now loaded from cmake/Dependencies.cmake

# Create ImGui library
add_library(imgui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)

target_include_directories(imgui PUBLIC
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
)

target_link_libraries(imgui PUBLIC
    glfw
    OpenGL::GL
)

# === smith_lib - Static library with all v2.0 code ===

add_library(smith_lib STATIC
    # Core framework
    src/core/app_base.cpp
    src/core/application.cpp

    # Logging system
    src/logging/logger.cpp
    src/logging/log_sinks.cpp

    # Agent system (Phase 2)
    src/agent/agent.cpp
    src/agent/agent_utils.cpp
    src/agent/agent_registry.cpp
    src/agent/agent_tracker.cpp
    src/agent/conversation.cpp
    src/agent/providers/claude_provider.cpp

    # Terminal system (Phase 3)
    src/terminal/terminal_theme.cpp
    src/terminal/conpty_terminal.cpp
    src/terminal/webview_terminal.cpp
    src/terminal/terminal_factory.cpp

    # Network system (Phase 4)
    src/network/http_client.cpp
    src/network/api_client.cpp
    src/network/rate_limiter.cpp
)

target_include_directories(smith_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_BINARY_DIR}/generated
)

target_link_libraries(smith_lib PUBLIC
    imgui
    nlohmann_json::nlohmann_json
    spdlog::spdlog
    httplib::httplib
)

# Platform-specific linking for library
if(WIN32)
    target_link_libraries(smith_lib PUBLIC
        dwmapi
        user32
        shell32
        kernel32  # Required for ConPTY APIs
        ole32     # Required for COM (WebView2)
        oleaut32  # Required for COM (WebView2)
    )

    # Add WebView2 SDK if available
    if(SMITH_USE_WEBVIEW2)
        target_include_directories(smith_lib PUBLIC
            ${webview2_SOURCE_DIR}/build/native/include
            ${wil_SOURCE_DIR}/include
        )
        target_link_libraries(smith_lib PUBLIC
            ${webview2_SOURCE_DIR}/build/native/x64/WebView2LoaderStatic.lib
        )
    endif()
elseif(UNIX AND NOT APPLE)
    find_package(X11 REQUIRED)
    target_link_libraries(smith_lib PUBLIC
        ${X11_LIBRARIES}
        pthread
    )
elseif(APPLE)
    target_link_libraries(smith_lib PUBLIC
        "-framework Cocoa"
        "-framework AppKit"
        pthread
    )
endif()

# === Main executable ===

add_executable(agent_smith WIN32
    src/main.cpp
)

target_link_libraries(agent_smith PRIVATE
    smith_lib
)

# Copy config template to bin directory (next to executable)
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/config.json.template
    ${CMAKE_SOURCE_DIR}/bin/config.json
    COPYONLY
)

# === Tests (conditional) ===

if(SMITH_BUILD_TESTS)
    include(cmake/Testing.cmake)
endif()

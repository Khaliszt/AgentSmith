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
)

target_include_directories(smith_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(smith_lib PUBLIC
    imgui
    nlohmann_json::nlohmann_json
    spdlog::spdlog
)

# Platform-specific linking for library
if(PLATFORM_WINDOWS)
    target_link_libraries(smith_lib PUBLIC
        dwmapi
        user32
        shell32
    )
elseif(PLATFORM_LINUX)
    find_package(X11 REQUIRED)
    target_link_libraries(smith_lib PUBLIC
        ${X11_LIBRARIES}
        pthread
    )
elseif(PLATFORM_MACOS)
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

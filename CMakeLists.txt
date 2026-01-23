cmake_minimum_required(VERSION 3.16)
project(AgentSmith VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Output binaries to bin/ folder
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR}/bin)

# Platform detection
if(WIN32)
    set(PLATFORM_WINDOWS TRUE)
    add_definitions(-DPLATFORM_WINDOWS)
    add_definitions(-DUNICODE -D_UNICODE)
elseif(UNIX AND NOT APPLE)
    set(PLATFORM_LINUX TRUE)
    add_definitions(-DPLATFORM_LINUX)
elseif(APPLE)
    set(PLATFORM_MACOS TRUE)
    add_definitions(-DPLATFORM_MACOS)
endif()

# Find required packages
find_package(OpenGL REQUIRED)

# Fetch dependencies
include(FetchContent)

# GLFW
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.3.9
)
FetchContent_MakeAvailable(glfw)

# Dear ImGui
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.90.1
)
FetchContent_MakeAvailable(imgui)

# nlohmann/json
FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
)
FetchContent_MakeAvailable(json)

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

# Main executable
add_executable(agent_smith
    src/main.cpp
    src/app.cpp
    src/agent_panel.cpp
    src/agent_manager.cpp
    src/terminal_embed.cpp
    src/git_utils.cpp
    src/config.cpp
    src/grid_layout.cpp
)

target_include_directories(agent_smith PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Platform-specific linking
if(PLATFORM_WINDOWS)
    target_link_libraries(agent_smith PRIVATE
        imgui
        nlohmann_json::nlohmann_json
        dwmapi
        user32
        shell32
    )
elseif(PLATFORM_LINUX)
    find_package(X11 REQUIRED)
    target_link_libraries(agent_smith PRIVATE
        imgui
        nlohmann_json::nlohmann_json
        ${X11_LIBRARIES}
        pthread
    )
elseif(PLATFORM_MACOS)
    target_link_libraries(agent_smith PRIVATE
        imgui
        nlohmann_json::nlohmann_json
        "-framework Cocoa"
        "-framework AppKit"
        pthread
    )
endif()

# Copy config template to bin directory (next to executable)
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/config.json.template
    ${CMAKE_SOURCE_DIR}/bin/config.json
    COPYONLY
)

# C:\FarfadetsCorp\AgentSmith\cmake\Dependencies.cmake

include(FetchContent)
set(FETCHCONTENT_QUIET OFF)

# === Existing Dependencies (keep versions consistent) ===

# GLFW
FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.3.9
)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

# Dear ImGui
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.90.1
)

# nlohmann/json
FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
)

# === New Dependencies ===

# spdlog (includes fmt)
FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG v1.12.0
)

# GoogleTest
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

# cpp-httplib (header-only HTTP client)
FetchContent_Declare(
    httplib
    GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
    GIT_TAG v0.14.3
)
set(HTTPLIB_REQUIRE_OPENSSL OFF CACHE BOOL "" FORCE)

# WebView2 (Windows only)
if(WIN32)
    FetchContent_Declare(
        webview2
        URL https://www.nuget.org/api/v2/package/Microsoft.Web.WebView2/1.0.2210.55
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )

    # Windows Implementation Library (COM helpers)
    FetchContent_Declare(
        wil
        GIT_REPOSITORY https://github.com/microsoft/wil.git
        GIT_TAG v1.0.231216.1
    )
    set(WIL_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(WIL_BUILD_PACKAGING OFF CACHE BOOL "" FORCE)
endif()

# Make dependencies available
FetchContent_MakeAvailable(glfw imgui json spdlog googletest httplib)
if(WIN32)
    FetchContent_MakeAvailable(webview2 wil)
endif()

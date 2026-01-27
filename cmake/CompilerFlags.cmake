# C:\FarfadetsCorp\AgentSmith\cmake\CompilerFlags.cmake

# Compiler-specific settings
if(MSVC)
    # MSVC settings
    add_compile_options(
        /W4                 # Warning level 4
        /permissive-        # Standards conformance
        /utf-8              # Source file encoding
        /MP                 # Multi-processor compilation
    )

    # Disable specific warnings
    add_compile_options(
        /wd4100             # Unreferenced formal parameter
        /wd4201             # Nameless struct/union
    )

    # Debug-specific
    if(CMAKE_BUILD_TYPE STREQUAL "Debug" OR CMAKE_CONFIGURATION_TYPES)
        # For multi-config generators (Visual Studio), don't force flags here
        # Let the generator handle it
    endif()

elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(
        -Wall -Wextra -Wpedantic
        -Wno-unused-parameter
    )

    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_compile_options(-g -O0)
    else()
        add_compile_options(-O2 -DNDEBUG)
    endif()
endif()

# Platform definitions
if(WIN32)
    add_definitions(-DPLATFORM_WINDOWS -DUNICODE -D_UNICODE)
    add_definitions(-DNOMINMAX)  # Prevent Windows.h min/max macros
elseif(UNIX AND NOT APPLE)
    add_definitions(-DPLATFORM_LINUX)
elseif(APPLE)
    add_definitions(-DPLATFORM_MACOS)
endif()

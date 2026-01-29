# cmake/Version.cmake
# Extracts git version information and generates version header

# Get git commit hash
execute_process(
    COMMAND git rev-parse --short=8 HEAD
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_COMMIT_HASH
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
)

# Get git branch name
execute_process(
    COMMAND git rev-parse --abbrev-ref HEAD
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_BRANCH
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
)

# Check if working directory is dirty
execute_process(
    COMMAND git status --porcelain
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_STATUS
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
)

if(GIT_STATUS)
    set(GIT_DIRTY "-dirty")
else()
    set(GIT_DIRTY "")
endif()

# Fallback values if git not available
if(NOT GIT_COMMIT_HASH)
    set(GIT_COMMIT_HASH "unknown")
endif()

if(NOT GIT_BRANCH)
    set(GIT_BRANCH "unknown")
endif()

# Build timestamp
string(TIMESTAMP BUILD_TIMESTAMP "%Y-%m-%d %H:%M:%S")

# Version components from project
set(SMITH_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(SMITH_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(SMITH_VERSION_PATCH ${PROJECT_VERSION_PATCH})
set(SMITH_VERSION_STRING "${PROJECT_VERSION}")
set(SMITH_VERSION_FULL "${PROJECT_VERSION}+${GIT_COMMIT_HASH}${GIT_DIRTY}")

message(STATUS "AgentSmith version: ${SMITH_VERSION_FULL}")
message(STATUS "Git branch: ${GIT_BRANCH}")
message(STATUS "Build timestamp: ${BUILD_TIMESTAMP}")

# Configure the version header
configure_file(
    ${CMAKE_SOURCE_DIR}/include/core/version.h.in
    ${CMAKE_BINARY_DIR}/generated/core/version.h
    @ONLY
)

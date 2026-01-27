# C:\FarfadetsCorp\AgentSmith\cmake\Testing.cmake

enable_testing()

# Test executable
add_executable(agent_smith_tests
    tests/test_main.cpp
    # Unit tests will be added here as they're created
)

target_include_directories(agent_smith_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/tests
)

target_link_libraries(agent_smith_tests PRIVATE
    smith_lib
    GTest::gtest
    GTest::gmock
)

# Register with CTest
include(GoogleTest)
gtest_discover_tests(agent_smith_tests)

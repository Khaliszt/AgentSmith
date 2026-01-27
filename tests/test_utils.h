// C:\FarfadetsCorp\AgentSmith\tests\test_utils.h

#pragma once

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <string>
#include <filesystem>

namespace smith::testing {

// Get path to test fixtures
inline std::filesystem::path GetFixturesPath() {
    return std::filesystem::path(__FILE__).parent_path() / "fixtures";
}

// Read fixture file contents
std::string ReadFixture(const std::string& filename);

} // namespace smith::testing

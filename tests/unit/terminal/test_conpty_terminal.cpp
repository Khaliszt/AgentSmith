// C:\FarfadetsCorp\AgentSmith\tests\unit\terminal\test_conpty_terminal.cpp

#include <gtest/gtest.h>
#include "terminal/conpty_terminal.h"
#include "terminal/terminal_theme.h"

using namespace smith::terminal;

class ConPTYTerminalTest : public ::testing::Test {
protected:
    void SetUp() override {
        // No setup needed
    }

    void TearDown() override {
        // No teardown needed
    }
};

TEST_F(ConPTYTerminalTest, ConstructorInitializes) {
    ConPTYTerminal terminal;

    EXPECT_FALSE(terminal.IsRunning());
    EXPECT_EQ(terminal.GetProcessId(), -1);
    EXPECT_EQ(terminal.GetExitCode(), -1);
    EXPECT_EQ(terminal.GetCols(), 120);
    EXPECT_EQ(terminal.GetRows(), 30);
}

TEST_F(ConPTYTerminalTest, DefaultThemeIsSet) {
    ConPTYTerminal terminal;

    const TerminalTheme& theme = terminal.GetTheme();
    EXPECT_EQ(theme.name, "Catppuccin Macchiato");
    EXPECT_EQ(theme.background, "#24273a");
    EXPECT_EQ(theme.foreground, "#cad3f5");
}

TEST_F(ConPTYTerminalTest, ThemeCanBeSet) {
    ConPTYTerminal terminal;

    TerminalTheme customTheme;
    customTheme.name = "Custom Theme";
    customTheme.background = "#000000";
    customTheme.foreground = "#ffffff";

    terminal.SetTheme(customTheme);

    const TerminalTheme& retrievedTheme = terminal.GetTheme();
    EXPECT_EQ(retrievedTheme.name, "Custom Theme");
    EXPECT_EQ(retrievedTheme.background, "#000000");
    EXPECT_EQ(retrievedTheme.foreground, "#ffffff");
}

TEST_F(ConPTYTerminalTest, ResizeUpdatesSize) {
    ConPTYTerminal terminal;

    terminal.Resize(80, 25);

    EXPECT_EQ(terminal.GetCols(), 80);
    EXPECT_EQ(terminal.GetRows(), 25);
}

TEST_F(ConPTYTerminalTest, HasCorrectCapabilities) {
    ConPTYTerminal terminal;

    EXPECT_FALSE(terminal.HasNativeRendering());
    EXPECT_FALSE(terminal.SupportsSelection());
}

// Note: We don't test Launch() here as it requires a real process to launch
// Integration tests will cover actual process launching

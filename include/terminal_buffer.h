#pragma once

#include <string>
#include <vector>
#include <deque>
#include <cstdint>
#include <mutex>

struct ImVec2;
struct ImDrawList;

namespace AgentSmith {

//=============================================================================
// Terminal Cell
//
// Represents a single character cell in the terminal buffer.
//=============================================================================

struct TerminalCell {
    char32_t character = U' ';
    uint32_t fg_color = 0xFFE0E0E0;  // Light gray text
    uint32_t bg_color = 0x00000000;  // Transparent background
    uint8_t attributes = 0;          // Bold, italic, underline, etc.

    // Attribute flags
    static constexpr uint8_t ATTR_BOLD      = 0x01;
    static constexpr uint8_t ATTR_DIM       = 0x02;
    static constexpr uint8_t ATTR_ITALIC    = 0x04;
    static constexpr uint8_t ATTR_UNDERLINE = 0x08;
    static constexpr uint8_t ATTR_BLINK     = 0x10;
    static constexpr uint8_t ATTR_REVERSE   = 0x20;
    static constexpr uint8_t ATTR_HIDDEN    = 0x40;
    static constexpr uint8_t ATTR_STRIKE    = 0x80;

    bool operator==(const TerminalCell& other) const {
        return character == other.character &&
               fg_color == other.fg_color &&
               bg_color == other.bg_color &&
               attributes == other.attributes;
    }
};

//=============================================================================
// Terminal Line
//
// A single line of terminal cells.
//=============================================================================

struct TerminalLine {
    std::vector<TerminalCell> cells;
    bool wrapped = false;  // Line was wrapped from previous line

    TerminalLine(int cols = 80) : cells(cols) {}

    void Resize(int cols) {
        cells.resize(cols);
    }

    void Clear(const TerminalCell& clearCell = TerminalCell()) {
        std::fill(cells.begin(), cells.end(), clearCell);
        wrapped = false;
    }
};

//=============================================================================
// Terminal Buffer
//
// Text buffer with ANSI escape sequence parsing.
// Stores a grid of TerminalCells, handles cursor, scrolling, scrollback.
//=============================================================================

class TerminalBuffer {
public:
    TerminalBuffer(int cols = 120, int rows = 30);
    ~TerminalBuffer();

    // Process raw input from PTY (contains ANSI sequences)
    void ProcessInput(const char* data, size_t length);

    // Resize the terminal
    void Resize(int cols, int rows);

    // Clear the screen
    void Clear();

    // Get terminal dimensions
    int GetCols() const { return m_cols; }
    int GetRows() const { return m_rows; }

    // Cursor position (0-based)
    int GetCursorX() const { return m_cursorX; }
    int GetCursorY() const { return m_cursorY; }

    // Scrollback
    int GetScrollbackSize() const { return static_cast<int>(m_scrollback.size()); }
    int GetScrollOffset() const { return m_scrollOffset; }
    void SetScrollOffset(int offset);
    void ScrollToBottom();

    // Render to ImGui draw list
    void Render(ImDrawList* drawList, const ImVec2& pos, const ImVec2& size,
                float charWidth, float charHeight, bool showCursor = true);

    // Get a line (0 = top of visible area, negative = scrollback)
    const TerminalLine* GetLine(int index) const;

    // Selection (future feature)
    std::string GetSelectedText() const;

    // Thread-safe access for rendering while receiving data
    void Lock() { m_mutex.lock(); }
    void Unlock() { m_mutex.unlock(); }

private:
    // Terminal dimensions
    int m_cols;
    int m_rows;

    // Screen buffer (visible area)
    std::vector<TerminalLine> m_screen;

    // Scrollback buffer
    std::deque<TerminalLine> m_scrollback;
    int m_maxScrollback = 10000;
    int m_scrollOffset = 0;  // 0 = bottom, positive = scrolled up

    // Cursor state
    int m_cursorX = 0;
    int m_cursorY = 0;
    bool m_cursorVisible = true;

    // Current text attributes
    TerminalCell m_currentAttrs;

    // Saved cursor position (for save/restore)
    int m_savedCursorX = 0;
    int m_savedCursorY = 0;

    // Scroll region
    int m_scrollTop = 0;
    int m_scrollBottom = 0;  // 0 means use m_rows

    // ANSI parser state
    enum class ParserState {
        Normal,
        Escape,     // Just received ESC
        CSI,        // Control Sequence Introducer (ESC [)
        OSC,        // Operating System Command (ESC ])
        OSCString,  // Reading OSC string
        DCS,        // Device Control String
        Charset     // Charset selection (ESC ( or ESC ))
    };

    ParserState m_parserState = ParserState::Normal;
    std::string m_escapeBuffer;
    std::string m_oscBuffer;
    char m_oscTerminator = '\0';

    // Threading
    std::mutex m_mutex;

    // Internal methods
    void ProcessChar(char32_t ch);
    void ProcessControlChar(char c);
    void ProcessEscapeSequence();
    void ProcessCSI();
    void ProcessOSC();
    void ProcessSGR(const std::vector<int>& params);

    void PutChar(char32_t ch);
    void NewLine();
    void CarriageReturn();
    void Tab();
    void Backspace();
    void Bell();

    void MoveCursor(int x, int y);
    void MoveCursorRelative(int dx, int dy);
    void SaveCursor();
    void RestoreCursor();

    void ScrollUp(int lines = 1);
    void ScrollDown(int lines = 1);

    void EraseInLine(int mode);
    void EraseInDisplay(int mode);
    void InsertLines(int count);
    void DeleteLines(int count);
    void InsertChars(int count);
    void DeleteChars(int count);

    void SetScrollRegion(int top, int bottom);
    void ResetAttributes();

    // Color helpers
    uint32_t AnsiColorToRGBA(int colorIndex, bool bright = false);
    uint32_t Parse256Color(int index);
    uint32_t ParseRGBColor(int r, int g, int b);

    // Parse CSI parameters
    std::vector<int> ParseCSIParams(const std::string& seq);

    // Ensure cursor is within bounds
    void ClampCursor();
};

} // namespace AgentSmith

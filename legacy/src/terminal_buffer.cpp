#include "terminal_buffer.h"
#include "output_log.h"
#include <imgui.h>
#include <algorithm>
#include <cstring>
#include <cctype>

namespace AgentSmith {

TerminalBuffer::TerminalBuffer(int cols, int rows)
    : m_cols(cols), m_rows(rows) {
    m_screen.resize(rows, TerminalLine(cols));
    m_scrollBottom = rows;

    // Set default theme
    m_theme = TerminalThemes::CatppuccinMacchiato();

    ResetAttributes();
}

void TerminalBuffer::SetTheme(const TerminalTheme& theme) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_theme = theme;

    // Update default attributes to use new theme colors
    m_currentAttrs.fg_color = m_theme.foreground;
    m_currentAttrs.bg_color = 0x00000000;  // Transparent (use theme bg in render)
}

TerminalBuffer::~TerminalBuffer() = default;

void TerminalBuffer::ProcessInput(const char* data, size_t length) {
    std::lock_guard<std::mutex> lock(m_mutex);

    for (size_t i = 0; i < length; ++i) {
        unsigned char c = static_cast<unsigned char>(data[i]);

        // Handle UTF-8 continuation bytes ONLY when in Normal state and expecting them
        if (m_parserState == ParserState::Normal && m_utf8Remaining > 0) {
            if ((c & 0xC0) == 0x80) {
                // Valid continuation byte
                m_utf8Codepoint = (m_utf8Codepoint << 6) | (c & 0x3F);
                m_utf8Remaining--;
                if (m_utf8Remaining == 0) {
                    // Complete codepoint - output it
                    PutChar(m_utf8Codepoint);
                }
                continue;
            } else {
                // Invalid continuation - reset and process this byte normally
                m_utf8Remaining = 0;
                m_utf8Codepoint = 0;
            }
        }

        switch (m_parserState) {
            case ParserState::Normal:
                if (c == 0x1B) {  // ESC
                    // Cancel any pending UTF-8 sequence
                    m_utf8Remaining = 0;
                    m_utf8Codepoint = 0;
                    m_parserState = ParserState::Escape;
                    m_escapeBuffer.clear();
                } else if (c < 0x20) {
                    ProcessControlChar(static_cast<char>(c));
                } else if (c < 0x80) {
                    // Regular ASCII (0x20-0x7F)
                    PutChar(static_cast<char32_t>(c));
                } else if ((c & 0xE0) == 0xC0) {
                    // 2-byte UTF-8 sequence (110xxxxx)
                    m_utf8Codepoint = c & 0x1F;
                    m_utf8Remaining = 1;
                } else if ((c & 0xF0) == 0xE0) {
                    // 3-byte UTF-8 sequence (1110xxxx)
                    m_utf8Codepoint = c & 0x0F;
                    m_utf8Remaining = 2;
                } else if ((c & 0xF8) == 0xF0) {
                    // 4-byte UTF-8 sequence (11110xxx)
                    m_utf8Codepoint = c & 0x07;
                    m_utf8Remaining = 3;
                } else if ((c & 0xC0) == 0x80) {
                    // Unexpected continuation byte - ignore
                } else {
                    // Invalid byte, show replacement character
                    PutChar(0xFFFD);
                }
                break;

            case ParserState::Escape:
                if (c == '[') {
                    m_parserState = ParserState::CSI;
                    m_escapeBuffer.clear();
                } else if (c == ']') {
                    m_parserState = ParserState::OSC;
                    m_oscBuffer.clear();
                } else if (c == '(' || c == ')') {
                    m_parserState = ParserState::Charset;
                } else if (c == 'P') {
                    m_parserState = ParserState::DCS;
                } else if (c == '7') {
                    SaveCursor();
                    m_parserState = ParserState::Normal;
                } else if (c == '8') {
                    RestoreCursor();
                    m_parserState = ParserState::Normal;
                } else if (c == 'D') {
                    // Index (move down, scroll if needed)
                    if (m_cursorY >= m_scrollBottom - 1) {
                        ScrollUp(1);
                    } else {
                        m_cursorY++;
                    }
                    m_parserState = ParserState::Normal;
                } else if (c == 'M') {
                    // Reverse Index (move up, scroll if needed)
                    if (m_cursorY <= m_scrollTop) {
                        ScrollDown(1);
                    } else {
                        m_cursorY--;
                    }
                    m_parserState = ParserState::Normal;
                } else if (c == 'E') {
                    // Next Line
                    NewLine();
                    CarriageReturn();
                    m_parserState = ParserState::Normal;
                } else if (c == 'c') {
                    // Reset
                    Clear();
                    m_parserState = ParserState::Normal;
                } else {
                    // Unknown escape, ignore
                    m_parserState = ParserState::Normal;
                }
                break;

            case ParserState::CSI:
                if (c >= 0x40 && c <= 0x7E) {
                    // Final byte - execute the sequence
                    m_escapeBuffer += static_cast<char>(c);
                    ProcessCSI();
                    m_parserState = ParserState::Normal;
                } else if (c >= 0x20 && c <= 0x3F) {
                    // Parameter or intermediate byte
                    m_escapeBuffer += static_cast<char>(c);
                } else {
                    // Invalid, abort
                    m_parserState = ParserState::Normal;
                }
                break;

            case ParserState::OSC:
                if (c == 0x07) {  // BEL terminates OSC
                    ProcessOSC();
                    m_parserState = ParserState::Normal;
                } else if (c == 0x1B) {
                    // Could be ESC \ (ST) terminator
                    m_oscTerminator = 0x1B;
                    m_parserState = ParserState::OSCString;
                } else {
                    m_oscBuffer += static_cast<char>(c);
                }
                break;

            case ParserState::OSCString:
                if (c == '\\' && m_oscTerminator == 0x1B) {
                    ProcessOSC();
                    m_parserState = ParserState::Normal;
                } else {
                    // Not a valid terminator, add to buffer
                    if (m_oscTerminator) {
                        m_oscBuffer += m_oscTerminator;
                    }
                    m_oscBuffer += static_cast<char>(c);
                    m_oscTerminator = '\0';
                    m_parserState = ParserState::OSC;
                }
                break;

            case ParserState::DCS:
                // Skip DCS sequences until ST (ESC \)
                if (c == 0x1B) {
                    m_oscTerminator = 0x1B;
                } else if (c == '\\' && m_oscTerminator == 0x1B) {
                    m_parserState = ParserState::Normal;
                    m_oscTerminator = '\0';
                } else {
                    m_oscTerminator = '\0';
                }
                break;

            case ParserState::Charset:
                // Ignore charset selection
                m_parserState = ParserState::Normal;
                break;
        }
    }
}

void TerminalBuffer::ProcessControlChar(char c) {
    switch (c) {
        case '\n':  // LF
            NewLine();
            break;
        case '\r':  // CR
            CarriageReturn();
            break;
        case '\t':  // TAB
            Tab();
            break;
        case '\b':  // BS
            Backspace();
            break;
        case 0x07:  // BEL
            Bell();
            break;
        case 0x0E:  // SO (Shift Out) - ignore
        case 0x0F:  // SI (Shift In) - ignore
            break;
        default:
            break;
    }
}

void TerminalBuffer::ProcessCSI() {
    if (m_escapeBuffer.empty()) return;

    char finalChar = m_escapeBuffer.back();
    std::string paramStr = m_escapeBuffer.substr(0, m_escapeBuffer.size() - 1);

    // Check for private mode prefix
    bool privateMode = false;
    if (!paramStr.empty() && paramStr[0] == '?') {
        privateMode = true;
        paramStr = paramStr.substr(1);
    }

    std::vector<int> params = ParseCSIParams(paramStr);

    switch (finalChar) {
        case 'A':  // CUU - Cursor Up
            MoveCursorRelative(0, -(params.empty() ? 1 : params[0]));
            break;

        case 'B':  // CUD - Cursor Down
            MoveCursorRelative(0, params.empty() ? 1 : params[0]);
            break;

        case 'C':  // CUF - Cursor Forward
            MoveCursorRelative(params.empty() ? 1 : params[0], 0);
            break;

        case 'D':  // CUB - Cursor Backward
            MoveCursorRelative(-(params.empty() ? 1 : params[0]), 0);
            break;

        case 'E':  // CNL - Cursor Next Line
            m_cursorX = 0;
            MoveCursorRelative(0, params.empty() ? 1 : params[0]);
            break;

        case 'F':  // CPL - Cursor Previous Line
            m_cursorX = 0;
            MoveCursorRelative(0, -(params.empty() ? 1 : params[0]));
            break;

        case 'G':  // CHA - Cursor Horizontal Absolute
            m_cursorX = (params.empty() ? 1 : params[0]) - 1;
            ClampCursor();
            break;

        case 'H':  // CUP - Cursor Position
        case 'f':  // HVP - same as CUP
            {
                int row = (params.size() > 0 && params[0] > 0) ? params[0] : 1;
                int col = (params.size() > 1 && params[1] > 0) ? params[1] : 1;
                MoveCursor(col - 1, row - 1);
            }
            break;

        case 'J':  // ED - Erase in Display
            EraseInDisplay(params.empty() ? 0 : params[0]);
            break;

        case 'K':  // EL - Erase in Line
            EraseInLine(params.empty() ? 0 : params[0]);
            break;

        case 'L':  // IL - Insert Lines
            InsertLines(params.empty() ? 1 : params[0]);
            break;

        case 'M':  // DL - Delete Lines
            DeleteLines(params.empty() ? 1 : params[0]);
            break;

        case 'P':  // DCH - Delete Characters
            DeleteChars(params.empty() ? 1 : params[0]);
            break;

        case '@':  // ICH - Insert Characters
            InsertChars(params.empty() ? 1 : params[0]);
            break;

        case 'S':  // SU - Scroll Up
            ScrollUp(params.empty() ? 1 : params[0]);
            break;

        case 'T':  // SD - Scroll Down
            ScrollDown(params.empty() ? 1 : params[0]);
            break;

        case 'd':  // VPA - Vertical Position Absolute
            m_cursorY = (params.empty() ? 1 : params[0]) - 1;
            ClampCursor();
            break;

        case 'm':  // SGR - Select Graphic Rendition
            ProcessSGR(params);
            break;

        case 'r':  // DECSTBM - Set Top and Bottom Margins
            {
                int top = (params.size() > 0 && params[0] > 0) ? params[0] : 1;
                int bottom = (params.size() > 1 && params[1] > 0) ? params[1] : m_rows;
                SetScrollRegion(top - 1, bottom);
            }
            break;

        case 's':  // SCOSC - Save Cursor Position
            SaveCursor();
            break;

        case 'u':  // SCORC - Restore Cursor Position
            RestoreCursor();
            break;

        case 'h':  // SM - Set Mode
            if (privateMode) {
                // Handle private modes like ?25h (show cursor)
                for (int p : params) {
                    if (p == 25) m_cursorVisible = true;
                }
            }
            break;

        case 'l':  // RM - Reset Mode
            if (privateMode) {
                for (int p : params) {
                    if (p == 25) m_cursorVisible = false;
                }
            }
            break;

        case 'n':  // DSR - Device Status Report (ignore)
            break;

        case 'c':  // DA - Device Attributes (ignore)
            break;

        default:
            // Unknown CSI sequence
            break;
    }
}

void TerminalBuffer::ProcessOSC() {
    // OSC sequences set window title, etc.
    // For now, just ignore them
}

void TerminalBuffer::ProcessSGR(const std::vector<int>& params) {
    if (params.empty()) {
        ResetAttributes();
        return;
    }

    for (size_t i = 0; i < params.size(); ++i) {
        int p = params[i];

        if (p == 0) {
            ResetAttributes();
        } else if (p == 1) {
            m_currentAttrs.attributes |= TerminalCell::ATTR_BOLD;
        } else if (p == 2) {
            m_currentAttrs.attributes |= TerminalCell::ATTR_DIM;
        } else if (p == 3) {
            m_currentAttrs.attributes |= TerminalCell::ATTR_ITALIC;
        } else if (p == 4) {
            m_currentAttrs.attributes |= TerminalCell::ATTR_UNDERLINE;
        } else if (p == 5 || p == 6) {
            m_currentAttrs.attributes |= TerminalCell::ATTR_BLINK;
        } else if (p == 7) {
            m_currentAttrs.attributes |= TerminalCell::ATTR_REVERSE;
        } else if (p == 8) {
            m_currentAttrs.attributes |= TerminalCell::ATTR_HIDDEN;
        } else if (p == 9) {
            m_currentAttrs.attributes |= TerminalCell::ATTR_STRIKE;
        } else if (p == 21 || p == 22) {
            m_currentAttrs.attributes &= ~(TerminalCell::ATTR_BOLD | TerminalCell::ATTR_DIM);
        } else if (p == 23) {
            m_currentAttrs.attributes &= ~TerminalCell::ATTR_ITALIC;
        } else if (p == 24) {
            m_currentAttrs.attributes &= ~TerminalCell::ATTR_UNDERLINE;
        } else if (p == 25) {
            m_currentAttrs.attributes &= ~TerminalCell::ATTR_BLINK;
        } else if (p == 27) {
            m_currentAttrs.attributes &= ~TerminalCell::ATTR_REVERSE;
        } else if (p == 28) {
            m_currentAttrs.attributes &= ~TerminalCell::ATTR_HIDDEN;
        } else if (p == 29) {
            m_currentAttrs.attributes &= ~TerminalCell::ATTR_STRIKE;
        } else if (p >= 30 && p <= 37) {
            // Foreground color
            bool bright = (m_currentAttrs.attributes & TerminalCell::ATTR_BOLD) != 0;
            m_currentAttrs.fg_color = AnsiColorToRGBA(p - 30, bright);
        } else if (p == 38) {
            // Extended foreground color
            if (i + 1 < params.size()) {
                if (params[i + 1] == 5 && i + 2 < params.size()) {
                    // 256 color mode
                    m_currentAttrs.fg_color = Parse256Color(params[i + 2]);
                    i += 2;
                } else if (params[i + 1] == 2 && i + 4 < params.size()) {
                    // RGB color mode
                    m_currentAttrs.fg_color = ParseRGBColor(params[i + 2], params[i + 3], params[i + 4]);
                    i += 4;
                }
            }
        } else if (p == 39) {
            // Default foreground (use theme)
            m_currentAttrs.fg_color = m_theme.foreground;
        } else if (p >= 40 && p <= 47) {
            // Background color
            m_currentAttrs.bg_color = AnsiColorToRGBA(p - 40, false);
        } else if (p == 48) {
            // Extended background color
            if (i + 1 < params.size()) {
                if (params[i + 1] == 5 && i + 2 < params.size()) {
                    m_currentAttrs.bg_color = Parse256Color(params[i + 2]);
                    i += 2;
                } else if (params[i + 1] == 2 && i + 4 < params.size()) {
                    m_currentAttrs.bg_color = ParseRGBColor(params[i + 2], params[i + 3], params[i + 4]);
                    i += 4;
                }
            }
        } else if (p == 49) {
            // Default background
            m_currentAttrs.bg_color = 0x00000000;
        } else if (p >= 90 && p <= 97) {
            // Bright foreground color
            m_currentAttrs.fg_color = AnsiColorToRGBA(p - 90, true);
        } else if (p >= 100 && p <= 107) {
            // Bright background color
            m_currentAttrs.bg_color = AnsiColorToRGBA(p - 100, true);
        }
    }
}

std::vector<int> TerminalBuffer::ParseCSIParams(const std::string& seq) {
    std::vector<int> params;
    std::string current;

    for (char c : seq) {
        if (c >= '0' && c <= '9') {
            current += c;
        } else if (c == ';' || c == ':') {
            params.push_back(current.empty() ? 0 : std::stoi(current));
            current.clear();
        }
    }

    if (!current.empty()) {
        params.push_back(std::stoi(current));
    }

    return params;
}

void TerminalBuffer::PutChar(char32_t ch) {
    if (m_cursorX >= m_cols) {
        // Wrap to next line
        m_cursorX = 0;
        NewLine();
        if (m_cursorY > 0) {
            m_screen[m_cursorY - 1].wrapped = true;
        }
    }

    if (m_cursorY >= 0 && m_cursorY < m_rows &&
        m_cursorX >= 0 && m_cursorX < m_cols) {
        auto& cell = m_screen[m_cursorY].cells[m_cursorX];
        cell.character = ch;
        cell.fg_color = m_currentAttrs.fg_color;
        cell.bg_color = m_currentAttrs.bg_color;
        cell.attributes = m_currentAttrs.attributes;

        // DEBUG: Uncomment to log first few characters stored (causes slowdown)
        // static int charCount = 0;
        // if (charCount < 20 && ch >= 32 && ch < 127) {
        //     LOG_DEBUG_SRC("PutChar '" + std::string(1, static_cast<char>(ch)) + "' at (" +
        //                   std::to_string(m_cursorX) + "," + std::to_string(m_cursorY) + ")", "TermBuf");
        //     charCount++;
        // }
    }

    m_cursorX++;
}

void TerminalBuffer::NewLine() {
    m_cursorY++;
    if (m_cursorY >= m_scrollBottom) {
        ScrollUp(1);
        m_cursorY = m_scrollBottom - 1;
    }
}

void TerminalBuffer::CarriageReturn() {
    m_cursorX = 0;
}

void TerminalBuffer::Tab() {
    // Move to next tab stop (every 8 columns)
    m_cursorX = ((m_cursorX / 8) + 1) * 8;
    if (m_cursorX >= m_cols) {
        m_cursorX = m_cols - 1;
    }
}

void TerminalBuffer::Backspace() {
    if (m_cursorX > 0) {
        m_cursorX--;
    }
}

void TerminalBuffer::Bell() {
    // Could trigger a visual bell or sound
}

void TerminalBuffer::MoveCursor(int x, int y) {
    m_cursorX = x;
    m_cursorY = y;
    ClampCursor();
}

void TerminalBuffer::MoveCursorRelative(int dx, int dy) {
    m_cursorX += dx;
    m_cursorY += dy;
    ClampCursor();
}

void TerminalBuffer::SaveCursor() {
    m_savedCursorX = m_cursorX;
    m_savedCursorY = m_cursorY;
}

void TerminalBuffer::RestoreCursor() {
    m_cursorX = m_savedCursorX;
    m_cursorY = m_savedCursorY;
    ClampCursor();
}

void TerminalBuffer::ScrollUp(int lines) {
    lines = (std::min)(lines, m_scrollBottom - m_scrollTop);

    // Move lines to scrollback
    for (int i = 0; i < lines; ++i) {
        if (m_scrollTop == 0) {
            // Only add to scrollback from top of screen
            m_scrollback.push_back(std::move(m_screen[m_scrollTop + i]));
            if (static_cast<int>(m_scrollback.size()) > m_maxScrollback) {
                m_scrollback.pop_front();
            }
        }
    }

    // Shift lines up within scroll region
    for (int y = m_scrollTop; y < m_scrollBottom - lines; ++y) {
        m_screen[y] = std::move(m_screen[y + lines]);
    }

    // Clear bottom lines
    for (int y = m_scrollBottom - lines; y < m_scrollBottom; ++y) {
        m_screen[y] = TerminalLine(m_cols);
    }
}

void TerminalBuffer::ScrollDown(int lines) {
    lines = (std::min)(lines, m_scrollBottom - m_scrollTop);

    // Shift lines down within scroll region
    for (int y = m_scrollBottom - 1; y >= m_scrollTop + lines; --y) {
        m_screen[y] = std::move(m_screen[y - lines]);
    }

    // Clear top lines
    for (int y = m_scrollTop; y < m_scrollTop + lines; ++y) {
        m_screen[y] = TerminalLine(m_cols);
    }
}

void TerminalBuffer::EraseInLine(int mode) {
    if (m_cursorY < 0 || m_cursorY >= m_rows) return;

    auto& line = m_screen[m_cursorY];
    TerminalCell clearCell;
    clearCell.bg_color = m_currentAttrs.bg_color;

    switch (mode) {
        case 0:  // Cursor to end of line
            for (int x = m_cursorX; x < m_cols; ++x) {
                line.cells[x] = clearCell;
            }
            break;
        case 1:  // Start of line to cursor
            for (int x = 0; x <= m_cursorX && x < m_cols; ++x) {
                line.cells[x] = clearCell;
            }
            break;
        case 2:  // Entire line
            line.Clear(clearCell);
            break;
    }
}

void TerminalBuffer::EraseInDisplay(int mode) {
    TerminalCell clearCell;
    clearCell.bg_color = m_currentAttrs.bg_color;

    switch (mode) {
        case 0:  // Cursor to end of screen
            EraseInLine(0);
            for (int y = m_cursorY + 1; y < m_rows; ++y) {
                m_screen[y].Clear(clearCell);
            }
            break;
        case 1:  // Start of screen to cursor
            for (int y = 0; y < m_cursorY; ++y) {
                m_screen[y].Clear(clearCell);
            }
            EraseInLine(1);
            break;
        case 2:  // Entire screen
        case 3:  // Entire screen + scrollback
            for (int y = 0; y < m_rows; ++y) {
                m_screen[y].Clear(clearCell);
            }
            if (mode == 3) {
                m_scrollback.clear();
            }
            break;
    }
}

void TerminalBuffer::InsertLines(int count) {
    if (m_cursorY < m_scrollTop || m_cursorY >= m_scrollBottom) return;

    count = (std::min)(count, m_scrollBottom - m_cursorY);

    // Shift lines down
    for (int y = m_scrollBottom - 1; y >= m_cursorY + count; --y) {
        m_screen[y] = std::move(m_screen[y - count]);
    }

    // Clear inserted lines
    for (int y = m_cursorY; y < m_cursorY + count; ++y) {
        m_screen[y] = TerminalLine(m_cols);
    }
}

void TerminalBuffer::DeleteLines(int count) {
    if (m_cursorY < m_scrollTop || m_cursorY >= m_scrollBottom) return;

    count = (std::min)(count, m_scrollBottom - m_cursorY);

    // Shift lines up
    for (int y = m_cursorY; y < m_scrollBottom - count; ++y) {
        m_screen[y] = std::move(m_screen[y + count]);
    }

    // Clear bottom lines
    for (int y = m_scrollBottom - count; y < m_scrollBottom; ++y) {
        m_screen[y] = TerminalLine(m_cols);
    }
}

void TerminalBuffer::InsertChars(int count) {
    if (m_cursorY < 0 || m_cursorY >= m_rows) return;

    auto& line = m_screen[m_cursorY];
    count = (std::min)(count, m_cols - m_cursorX);

    // Shift characters right
    for (int x = m_cols - 1; x >= m_cursorX + count; --x) {
        line.cells[x] = line.cells[x - count];
    }

    // Clear inserted positions
    TerminalCell clearCell;
    clearCell.bg_color = m_currentAttrs.bg_color;
    for (int x = m_cursorX; x < m_cursorX + count; ++x) {
        line.cells[x] = clearCell;
    }
}

void TerminalBuffer::DeleteChars(int count) {
    if (m_cursorY < 0 || m_cursorY >= m_rows) return;

    auto& line = m_screen[m_cursorY];
    count = (std::min)(count, m_cols - m_cursorX);

    // Shift characters left
    for (int x = m_cursorX; x < m_cols - count; ++x) {
        line.cells[x] = line.cells[x + count];
    }

    // Clear end positions
    TerminalCell clearCell;
    clearCell.bg_color = m_currentAttrs.bg_color;
    for (int x = m_cols - count; x < m_cols; ++x) {
        line.cells[x] = clearCell;
    }
}

void TerminalBuffer::SetScrollRegion(int top, int bottom) {
    m_scrollTop = (std::max)(0, (std::min)(top, m_rows - 1));
    m_scrollBottom = (std::max)(m_scrollTop + 1, (std::min)(bottom, m_rows));
    MoveCursor(0, m_scrollTop);
}

void TerminalBuffer::ResetAttributes() {
    m_currentAttrs = TerminalCell();
    m_currentAttrs.fg_color = m_theme.foreground;
    m_currentAttrs.bg_color = 0x00000000;  // Transparent (use theme bg)
}

void TerminalBuffer::Resize(int cols, int rows) {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (cols == m_cols && rows == m_rows) return;

    // Resize existing lines
    for (auto& line : m_screen) {
        line.Resize(cols);
    }

    // Add or remove lines
    while (static_cast<int>(m_screen.size()) < rows) {
        m_screen.emplace_back(cols);
    }
    while (static_cast<int>(m_screen.size()) > rows) {
        // Move removed lines to scrollback if they have content
        auto& line = m_screen.front();
        bool hasContent = false;
        for (const auto& cell : line.cells) {
            if (cell.character != U' ') {
                hasContent = true;
                break;
            }
        }
        if (hasContent) {
            m_scrollback.push_back(std::move(line));
            if (static_cast<int>(m_scrollback.size()) > m_maxScrollback) {
                m_scrollback.pop_front();
            }
        }
        m_screen.erase(m_screen.begin());
    }

    m_cols = cols;
    m_rows = rows;
    m_scrollBottom = rows;

    ClampCursor();
}

void TerminalBuffer::Clear() {
    std::lock_guard<std::mutex> lock(m_mutex);

    for (auto& line : m_screen) {
        line.Clear();
    }

    m_cursorX = 0;
    m_cursorY = 0;
    m_scrollTop = 0;
    m_scrollBottom = m_rows;
    ResetAttributes();
}

void TerminalBuffer::SetScrollOffset(int offset) {
    m_scrollOffset = (std::max)(0, (std::min)(offset, static_cast<int>(m_scrollback.size())));
}

void TerminalBuffer::ScrollToBottom() {
    m_scrollOffset = 0;
}

const TerminalLine* TerminalBuffer::GetLine(int index) const {
    if (index < 0) {
        // Scrollback
        int scrollbackIdx = static_cast<int>(m_scrollback.size()) + index;
        if (scrollbackIdx >= 0 && scrollbackIdx < static_cast<int>(m_scrollback.size())) {
            return &m_scrollback[scrollbackIdx];
        }
        return nullptr;
    }

    if (index >= 0 && index < m_rows) {
        return &m_screen[index];
    }

    return nullptr;
}

std::string TerminalBuffer::GetSelectedText() const {
    // Future feature - selection support
    return "";
}

void TerminalBuffer::ClampCursor() {
    m_cursorX = (std::max)(0, (std::min)(m_cursorX, m_cols - 1));
    m_cursorY = (std::max)(0, (std::min)(m_cursorY, m_rows - 1));
}

uint32_t TerminalBuffer::AnsiColorToRGBA(int colorIndex, bool bright) {
    if (colorIndex < 0 || colorIndex > 7) {
        return m_theme.foreground;  // Default to theme foreground
    }
    // Use theme colors: 0-7 are normal, 8-15 are bright
    int index = bright ? (colorIndex + 8) : colorIndex;
    return m_theme.ansiColors[index];
}

uint32_t TerminalBuffer::Parse256Color(int index) {
    if (index < 0 || index > 255) {
        return m_theme.foreground;
    }

    if (index < 16) {
        // Standard colors from theme
        return m_theme.ansiColors[index];
    }

    if (index < 232) {
        // 216 color cube (6x6x6)
        index -= 16;
        int r = (index / 36) % 6;
        int g = (index / 6) % 6;
        int b = index % 6;

        r = r ? (r * 40 + 55) : 0;
        g = g ? (g * 40 + 55) : 0;
        b = b ? (b * 40 + 55) : 0;

        return 0xFF000000 | (b << 16) | (g << 8) | r;
    }

    // Grayscale (24 steps)
    int gray = (index - 232) * 10 + 8;
    return 0xFF000000 | (gray << 16) | (gray << 8) | gray;
}

uint32_t TerminalBuffer::ParseRGBColor(int r, int g, int b) {
    r = (std::max)(0, (std::min)(255, r));
    g = (std::max)(0, (std::min)(255, g));
    b = (std::max)(0, (std::min)(255, b));
    return 0xFF000000 | (b << 16) | (g << 8) | r;
}

void TerminalBuffer::Render(ImDrawList* drawList, const ImVec2& pos, const ImVec2& size,
                            float charWidth, float charHeight, bool showCursor) {
    // Calculate visible rows
    int visibleRows = static_cast<int>(size.y / charHeight);
    int startRow = -m_scrollOffset;

    // DEBUG: Uncomment to log render stats periodically (causes slowdown due to buffer scan)
    // static bool loggedOnce = false;
    // static int frameCount = 0;
    // frameCount++;
    // if (!loggedOnce || (frameCount % 300 == 0)) {
    //     int charCount = 0;
    //     for (int row = 0; row < m_rows; ++row) {
    //         for (int col = 0; col < m_cols && col < static_cast<int>(m_screen[row].cells.size()); ++col) {
    //             if (m_screen[row].cells[col].character > ' ') charCount++;
    //         }
    //     }
    //     LOG_DEBUG_SRC("Render: size=(" + std::to_string(static_cast<int>(size.x)) + "," +
    //                   std::to_string(static_cast<int>(size.y)) + ") charsInBuf=" + std::to_string(charCount), "TermBuf");
    //     loggedOnce = true;
    // }

    // Draw background using theme color
    drawList->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y), m_theme.background);

    // Draw each visible line
    for (int screenRow = 0; screenRow < visibleRows; ++screenRow) {
        int bufferRow = startRow + screenRow;
        const TerminalLine* line = nullptr;

        if (bufferRow < 0) {
            // Scrollback
            int scrollbackIdx = static_cast<int>(m_scrollback.size()) + bufferRow;
            if (scrollbackIdx >= 0 && scrollbackIdx < static_cast<int>(m_scrollback.size())) {
                line = &m_scrollback[scrollbackIdx];
            }
        } else if (bufferRow < m_rows) {
            line = &m_screen[bufferRow];
        }

        if (!line) continue;

        float y = pos.y + screenRow * charHeight;

        for (int col = 0; col < m_cols && col < static_cast<int>(line->cells.size()); ++col) {
            const auto& cell = line->cells[col];
            float x = pos.x + col * charWidth;

            // Draw background if not transparent
            uint32_t bg = cell.bg_color;
            if (cell.attributes & TerminalCell::ATTR_REVERSE) {
                bg = cell.fg_color;
            }
            if ((bg & 0xFF000000) != 0) {
                drawList->AddRectFilled(
                    ImVec2(x, y),
                    ImVec2(x + charWidth, y + charHeight),
                    bg
                );
            }

            // Draw character
            if (cell.character > ' ' && !(cell.attributes & TerminalCell::ATTR_HIDDEN)) {
                uint32_t fg = cell.fg_color;
                if (cell.attributes & TerminalCell::ATTR_REVERSE) {
                    fg = cell.bg_color ? cell.bg_color : 0xFF0F0F12;
                }
                if (cell.attributes & TerminalCell::ATTR_DIM) {
                    // Dim the color
                    int r = (fg & 0xFF) / 2;
                    int g = ((fg >> 8) & 0xFF) / 2;
                    int b = ((fg >> 16) & 0xFF) / 2;
                    fg = 0xFF000000 | (b << 16) | (g << 8) | r;
                }

                // Convert char32_t to string for ImGui
                char utf8[5] = {0};
                if (cell.character < 0x80) {
                    utf8[0] = static_cast<char>(cell.character);
                } else if (cell.character < 0x800) {
                    utf8[0] = static_cast<char>(0xC0 | (cell.character >> 6));
                    utf8[1] = static_cast<char>(0x80 | (cell.character & 0x3F));
                } else if (cell.character < 0x10000) {
                    utf8[0] = static_cast<char>(0xE0 | (cell.character >> 12));
                    utf8[1] = static_cast<char>(0x80 | ((cell.character >> 6) & 0x3F));
                    utf8[2] = static_cast<char>(0x80 | (cell.character & 0x3F));
                } else {
                    utf8[0] = static_cast<char>(0xF0 | (cell.character >> 18));
                    utf8[1] = static_cast<char>(0x80 | ((cell.character >> 12) & 0x3F));
                    utf8[2] = static_cast<char>(0x80 | ((cell.character >> 6) & 0x3F));
                    utf8[3] = static_cast<char>(0x80 | (cell.character & 0x3F));
                }

                drawList->AddText(ImVec2(x, y), fg, utf8);

                // Underline
                if (cell.attributes & TerminalCell::ATTR_UNDERLINE) {
                    drawList->AddLine(
                        ImVec2(x, y + charHeight - 1),
                        ImVec2(x + charWidth, y + charHeight - 1),
                        fg
                    );
                }

                // Strikethrough
                if (cell.attributes & TerminalCell::ATTR_STRIKE) {
                    drawList->AddLine(
                        ImVec2(x, y + charHeight / 2),
                        ImVec2(x + charWidth, y + charHeight / 2),
                        fg
                    );
                }
            }
        }
    }

    // Draw cursor
    if (showCursor && m_cursorVisible && m_scrollOffset == 0) {
        float cursorX = pos.x + m_cursorX * charWidth;
        float cursorY = pos.y + m_cursorY * charHeight;

        // Block cursor with blink (simplified - always on for now)
        drawList->AddRectFilled(
            ImVec2(cursorX, cursorY),
            ImVec2(cursorX + charWidth, cursorY + charHeight),
            IM_COL32(200, 200, 200, 180)
        );
    }
}

} // namespace AgentSmith

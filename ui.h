//
// ui.h - Terminal drawing helpers for the autocomplete interface
// (colors, boxes, bar charts, number formatting, input prompts).
// Author: Navjot Singh
//

#ifndef DSA_PROJECT2_UI_H
#define DSA_PROJECT2_UI_H

#include <cmath>
#include <cstddef>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <string>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN   // skip rarely used Windows headers (they clash with std::byte)
#endif
#include <windows.h>
#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif
#else
#include <unistd.h>
#endif

namespace ui {

// ---------------------------------------------------------------- settings

struct Settings {
    bool color;     // ANSI colors + screen clearing
    bool unicode;   // box-drawing characters (false = plain ASCII)
    Settings() : color(true), unicode(true) {}
};

inline Settings& settings() {
    static Settings s;
    return s;
}

// Turns on UTF-8 output and ANSI colors on Windows. If the terminal
// can't do colors (or output is redirected to a file), colors are disabled.
inline void initTerminal() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    if (!GetConsoleMode(out, &mode) ||
        !SetConsoleMode(out, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING)) {
        settings().color = false;
    }
#else
    if (!isatty(STDOUT_FILENO)) settings().color = false;
#endif
}

// ---------------------------------------------------------------- colors

inline std::string paint(const std::string& text, const char* code) {
    if (!settings().color) return text;
    return std::string("\033[") + code + "m" + text + "\033[0m";
}
inline std::string bold(const std::string& s)    { return paint(s, "1"); }
inline std::string dim(const std::string& s)     { return paint(s, "2"); }
inline std::string cyan(const std::string& s)    { return paint(s, "1;36"); }
inline std::string green(const std::string& s)   { return paint(s, "1;32"); }
inline std::string yellow(const std::string& s)  { return paint(s, "1;33"); }
inline std::string red(const std::string& s)     { return paint(s, "1;31"); }
inline std::string magenta(const std::string& s) { return paint(s, "1;35"); }

// ---------------------------------------------------------------- glyphs

struct Glyphs {
    const char* topLeft;  const char* topRight;
    const char* botLeft;  const char* botRight;
    const char* horiz;    const char* vert;
    const char* midLeft;  const char* midRight;
    const char* thin;     const char* colSep;  const char* cross;
    const char* barFull;  const char* barEmpty;
    const char* check;    const char* xmark;
    const char* arrow;    const char* dot;     const char* ellipsis;
    const char* cursor;
};

inline const Glyphs& glyph() {
    static const Glyphs uni = {
        "╔", "╗", "╚", "╝", "═", "║", "╠", "╣",
        "─", "│", "┼",
        "█", "░",
        "✔", "✘",
        "▸", "·", "…",
        "▌"
    };
    static const Glyphs ascii = {
        "+", "+", "+", "+", "=", "|", "+", "+",
        "-", "|", "+",
        "#", ".",
        "OK", "X",
        ">", "-", "~",
        "_"
    };
    return settings().unicode ? uni : ascii;
}

// ---------------------------------------------------------------- text width

// Number of characters a string takes on screen: skips ANSI color codes
// and counts each UTF-8 character once.
inline size_t visibleWidth(const std::string& s) {
    size_t width = 0;
    for (size_t i = 0; i < s.size(); i++) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c == 0x1B) {                         // skip "\033[ ... m"
            while (i < s.size() && s[i] != 'm') i++;
            continue;
        }
        if ((c & 0xC0) != 0x80) width++;         // not a UTF-8 continuation byte
    }
    return width;
}

inline std::string repeat(const std::string& piece, size_t n) {
    std::string out;
    for (size_t i = 0; i < n; i++) out += piece;
    return out;
}

inline std::string padRight(const std::string& s, size_t width) {
    size_t w = visibleWidth(s);
    return w >= width ? s : s + std::string(width - w, ' ');
}

inline std::string center(const std::string& s, size_t width) {
    size_t w = visibleWidth(s);
    if (w >= width) return s;
    size_t left = (width - w) / 2;
    return std::string(left, ' ') + s + std::string(width - w - left, ' ');
}

// Cuts a plain (uncolored) word down to maxLen characters, adding "…".
inline std::string truncate(const std::string& word, size_t maxLen) {
    if (word.size() <= maxLen || maxLen == 0) return word;
    return word.substr(0, maxLen - 1) + glyph().ellipsis;
}

// Colors the first prefixLen letters of a word so the typed part stands out.
inline std::string highlightPrefix(const std::string& word, size_t prefixLen) {
    if (prefixLen > word.size()) prefixLen = word.size();
    return cyan(word.substr(0, prefixLen)) + word.substr(prefixLen);
}

// ---------------------------------------------------------------- boxes

const size_t BOX_INNER = 58;   // characters between the two side borders
const char* const INDENT = "  ";

inline void boxTop() {
    std::cout << INDENT << glyph().topLeft << repeat(glyph().horiz, BOX_INNER)
              << glyph().topRight << "\n";
}
inline void boxDivider() {
    std::cout << INDENT << glyph().midLeft << repeat(glyph().horiz, BOX_INNER)
              << glyph().midRight << "\n";
}
inline void boxBottom() {
    std::cout << INDENT << glyph().botLeft << repeat(glyph().horiz, BOX_INNER)
              << glyph().botRight << "\n";
}
inline void boxLine(const std::string& content, bool centered = false) {
    std::string body = centered ? center(content, BOX_INNER - 2)
                                : padRight(content, BOX_INNER - 2);
    std::cout << INDENT << glyph().vert << " " << body << " " << glyph().vert << "\n";
}

// A one-line boxed title, used at the top of each screen.
inline void titleBox(const std::string& title) {
    boxTop();
    boxLine(bold(title), true);
    boxBottom();
}

inline void rule(size_t width = BOX_INNER + 2) {
    std::cout << INDENT << dim(repeat(glyph().thin, width)) << "\n";
}

inline void clearScreen() {
    if (settings().color) std::cout << "\033[2J\033[H";
    else std::cout << "\n\n";
}

// ---------------------------------------------------------------- charts

// A horizontal bar: value/maxValue of the width is filled.
inline std::string bar(double value, double maxValue, size_t width, const char* colorCode = "0") {
    size_t filled = 0;
    if (maxValue > 0) filled = static_cast<size_t>(std::round(value / maxValue * width));
    if (filled > width) filled = width;
    if (value > 0 && filled == 0) filled = 1;     // never hide a nonzero value
    return paint(repeat(glyph().barFull, filled), colorCode) + dim(repeat(glyph().barEmpty, width - filled));
}

// Loading bar that redraws itself on one line, e.g. "[#####.....]  50%"
inline void progress(const std::string& label, double fraction) {
    if (fraction < 0) fraction = 0;
    if (fraction > 1) fraction = 1;
    const size_t width = 30;
    size_t filled = static_cast<size_t>(fraction * width);
    std::cout << "\r" << INDENT << padRight(label, 16) << "["
              << cyan(repeat(glyph().barFull, filled))
              << dim(repeat(glyph().barEmpty, width - filled)) << "] "
              << std::setw(3) << static_cast<int>(fraction * 100 + 0.5) << "%" << std::flush;
}

// ---------------------------------------------------------------- formatting

// 370105 -> "370,105"
inline std::string withCommas(unsigned long long n) {
    std::string digits = std::to_string(n);
    std::string out;
    int count = 0;
    for (size_t i = digits.size(); i > 0; i--) {
        out.insert(out.begin(), digits[i - 1]);
        if (++count % 3 == 0 && i > 1) out.insert(out.begin(), ',');
    }
    return out;
}

// Picks sensible units/decimals: 0.42 µs, 0.0140 ms, 12.35 ms, 412 ms
inline std::string formatMs(double ms) {
    std::ostringstream os;
    os << std::fixed;
    if (ms < 0.01) {                                  // tiny times read better in microseconds
        os << std::setprecision(2) << ms * 1000.0 << (settings().unicode ? " µs" : " us");
        return os.str();
    }
    if (ms < 1.0) os << std::setprecision(4);
    else if (ms < 100.0) os << std::setprecision(2);
    else os << std::setprecision(0);
    os << ms << " ms";
    return os.str();
}

inline std::string formatMB(size_t bytes) {
    std::ostringstream os;
    os << std::fixed << std::setprecision(1) << bytes / (1024.0 * 1024.0) << " MB";
    return os.str();
}

inline std::string formatRatio(double r) {
    std::ostringstream os;
    os << std::fixed << std::setprecision(2) << r << "x";
    return os.str();
}

// ---------------------------------------------------------------- input

// Thrown when the input stream ends (Ctrl+D / Ctrl+Z) so the program can exit cleanly.
struct InputClosed {};

inline std::string readLine(const std::string& prompt) {
    std::cout << INDENT << prompt << std::flush;
    std::string line;
    if (!std::getline(std::cin, line)) throw InputClosed();
    if (!line.empty() && line[line.size() - 1] == '\r') line.erase(line.size() - 1);
    return line;
}

inline std::string trim(const std::string& s) {
    size_t a = s.find_first_not_of(" \t");
    if (a == std::string::npos) return "";
    size_t b = s.find_last_not_of(" \t");
    return s.substr(a, b - a + 1);
}

// Asks for a whole number in [lo, hi]. Pressing Enter keeps the default.
inline int readInt(const std::string& prompt, int defaultValue, int lo, int hi) {
    while (true) {
        std::string line = trim(readLine(prompt));
        if (line.empty()) return defaultValue;
        bool digitsOnly = line.find_first_not_of("0123456789") == std::string::npos;
        if (digitsOnly && line.size() <= 6) {
            int v = std::stoi(line);
            if (v >= lo && v <= hi) return v;
        }
        std::cout << INDENT << red("Please enter a number from " + std::to_string(lo) +
                                   " to " + std::to_string(hi) + ".") << "\n";
    }
}

inline void pause() {
    std::cout << "\n";
    readLine(dim("Press Enter to return to menu..."));
}

} // namespace ui

#endif //DSA_PROJECT2_UI_H

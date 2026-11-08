//
// live_input.h - Reads single key presses without waiting for Enter,
// used by the live type-ahead mode. Works on Windows, macOS and Linux.
// Author: Navjot Singh
//

#ifndef DSA_PROJECT2_LIVE_INPUT_H
#define DSA_PROJECT2_LIVE_INPUT_H

#ifdef _WIN32
#include <conio.h>
#include <io.h>
#include <cstdio>
#else
#include <poll.h>
#include <termios.h>
#include <unistd.h>
#endif

enum KeyType { KEY_CHAR, KEY_BACKSPACE, KEY_ENTER, KEY_ESCAPE, KEY_TAB, KEY_END_OF_INPUT, KEY_OTHER };

struct Key {
    KeyType type;
    char ch;        // only set for KEY_CHAR
    Key(KeyType t, char c = 0) : type(t), ch(c) {}
};

// True if the program is reading from a real keyboard (not a piped file).
inline bool keyboardAvailable() {
#ifdef _WIN32
    return _isatty(_fileno(stdin)) != 0;
#else
    return isatty(STDIN_FILENO) != 0;
#endif
}

// While one of these exists, key presses arrive one at a time and aren't echoed.
// The terminal goes back to normal when it is destroyed.
class RawKeyboard {
public:
#ifdef _WIN32
    RawKeyboard() {}
    ~RawKeyboard() {}
#else
    RawKeyboard() : active_(false) {
        if (tcgetattr(STDIN_FILENO, &old_) == 0) {
            termios raw = old_;
            // ISIG off so Ctrl+C comes through as a key instead of killing the
            // program and leaving the terminal stuck without echo.
            raw.c_lflag &= ~(ICANON | ECHO | ISIG);
            raw.c_cc[VMIN] = 1;
            raw.c_cc[VTIME] = 0;
            active_ = tcsetattr(STDIN_FILENO, TCSANOW, &raw) == 0;
        }
    }
    ~RawKeyboard() {
        if (active_) tcsetattr(STDIN_FILENO, TCSANOW, &old_);
    }
#endif
    RawKeyboard(const RawKeyboard&) = delete;
    RawKeyboard& operator=(const RawKeyboard&) = delete;

    Key read() {
#ifdef _WIN32
        int c = _getch();
        if (c == 0 || c == 224) { _getch(); return Key(KEY_OTHER); }   // arrows, F-keys
        if (c == 8) return Key(KEY_BACKSPACE);
        if (c == 13) return Key(KEY_ENTER);
        if (c == 27) return Key(KEY_ESCAPE);
        if (c == 9) return Key(KEY_TAB);
        if (c == 26 || c == 3) return Key(KEY_END_OF_INPUT);           // Ctrl+Z, Ctrl+C
        if (c >= 32 && c < 127) return Key(KEY_CHAR, static_cast<char>(c));
        return Key(KEY_OTHER);
#else
        unsigned char c;
        if (::read(STDIN_FILENO, &c, 1) != 1) return Key(KEY_END_OF_INPUT);
        if (c == 127 || c == 8) return Key(KEY_BACKSPACE);
        if (c == '\n' || c == '\r') return Key(KEY_ENTER);
        if (c == '\t') return Key(KEY_TAB);
        if (c == 4 || c == 3) return Key(KEY_END_OF_INPUT);            // Ctrl+D, Ctrl+C
        if (c == 27) {
            // Arrow keys send ESC followed by more bytes; a lone ESC means Escape.
            if (!moreBytesWaiting()) return Key(KEY_ESCAPE);
            while (moreBytesWaiting()) {
                unsigned char skip;
                if (::read(STDIN_FILENO, &skip, 1) != 1) break;
            }
            return Key(KEY_OTHER);
        }
        if (c >= 32 && c < 127) return Key(KEY_CHAR, static_cast<char>(c));
        return Key(KEY_OTHER);
#endif
    }

private:
#ifndef _WIN32
    termios old_;
    bool active_;

    static bool moreBytesWaiting() {
        pollfd p;
        p.fd = STDIN_FILENO;
        p.events = POLLIN;
        return poll(&p, 1, 30) > 0;
    }
#endif
};

#endif //DSA_PROJECT2_LIVE_INPUT_H

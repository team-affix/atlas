#include "infrastructure/stdin_try_read_byte.hpp"
#include <termios.h>
#include <unistd.h>

std::optional<char> stdin_try_read_byte::try_read_byte() {
    termios original;
    if (tcgetattr(STDIN_FILENO, &original) != 0)
        return std::nullopt;
    termios raw = original;
    raw.c_lflag &= static_cast<tcflag_t>(~(ICANON | ECHO));
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 0;
    if (tcsetattr(STDIN_FILENO, TCSANOW, &raw) != 0)
        return std::nullopt;
    unsigned char byte = 0;
    const ssize_t n = read(STDIN_FILENO, &byte, 1);
    tcsetattr(STDIN_FILENO, TCSANOW, &original);
    if (n != 1)
        return std::nullopt;
    return static_cast<char>(byte);
}

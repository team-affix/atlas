#include "infrastructure/stdin_is_tty.hpp"
#include <unistd.h>

bool stdin_is_tty::is_tty() const {
    return isatty(STDIN_FILENO);
}

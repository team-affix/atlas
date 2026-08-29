#ifndef STDIN_TRY_READ_BYTE_HPP
#define STDIN_TRY_READ_BYTE_HPP

#include <optional>

struct stdin_try_read_byte {
    std::optional<char> try_read_byte();
};

#endif

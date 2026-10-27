#include "infrastructure/hex_uint64.hpp"
#include <sstream>
#include <stdexcept>

uint64_t hex_uint64::parse_hex(const std::string& text) const {
    if (text.empty())
        throw std::runtime_error("empty hex uint64");
    uint64_t value = 0;
    for (char character : text) {
        const bool digit = character >= '0' && character <= '9';
        const bool lower = character >= 'a' && character <= 'f';
        const bool upper = character >= 'A' && character <= 'F';
        if (!digit && !lower && !upper)
            throw std::runtime_error("invalid hex digit in uint64: " + text);
        const uint64_t nibble = digit
            ? static_cast<uint64_t>(character - '0')
            : (lower ? static_cast<uint64_t>(character - 'a' + 10) : static_cast<uint64_t>(character - 'A' + 10));
        value = (value << 4) | nibble;
    }
    return value;
}

std::string hex_uint64::format_hex(uint64_t value) const {
    if (value == 0)
        return "0";
    std::ostringstream out;
    out << std::hex << value;
    return out.str();
}

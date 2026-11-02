#include "infrastructure/hex_uint64.hpp"
#include <sstream>
#include <stdexcept>

uint64_t hex_uint64::parse_hex(const std::string& text) const {
    constexpr size_t k_max_hex_digits = 16;
    if (text.empty())
        throw std::runtime_error("empty hex uint64");
    if (text.size() > k_max_hex_digits)
        throw std::runtime_error("hex uint64 has more than 16 digits: " + text);
    uint64_t value = 0;
    for (const char character : text) {
        uint64_t nibble = 0;
        if (character >= '0' && character <= '9')
            nibble = static_cast<uint64_t>(character - '0');
        else if (character >= 'a' && character <= 'f')
            nibble = static_cast<uint64_t>(character - 'a' + 10);
        else if (character >= 'A' && character <= 'F')
            nibble = static_cast<uint64_t>(character - 'A' + 10);
        else
            throw std::runtime_error("invalid hex digit in uint64: " + text);
        value = (value << 4) | nibble;
    }
    return value;
}

std::string hex_uint64::format_hex(uint64_t value) const {
    std::ostringstream out;
    out << std::hex << value;
    return out.str();
}

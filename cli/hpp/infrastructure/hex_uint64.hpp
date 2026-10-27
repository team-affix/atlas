#ifndef HEX_UINT64_HPP
#define HEX_UINT64_HPP

#include <cstdint>
#include <string>

struct hex_uint64 {
    uint64_t parse_hex(const std::string& text) const;
    std::string format_hex(uint64_t value) const;
};

#endif

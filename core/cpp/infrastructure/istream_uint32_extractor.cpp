#include "infrastructure/istream_uint32_extractor.hpp"
#include <stdexcept>

uint32_t istream_uint32_extractor::extract_uint32(std::istream& in) const {
    char bytes[4] = {};
    in.read(bytes, 4);
    if (!in || in.gcount() != 4)
        throw std::runtime_error("truncated binary stream");
    return static_cast<uint32_t>(static_cast<uint8_t>(bytes[0]))
        | (static_cast<uint32_t>(static_cast<uint8_t>(bytes[1])) << 8)
        | (static_cast<uint32_t>(static_cast<uint8_t>(bytes[2])) << 16)
        | (static_cast<uint32_t>(static_cast<uint8_t>(bytes[3])) << 24);
}

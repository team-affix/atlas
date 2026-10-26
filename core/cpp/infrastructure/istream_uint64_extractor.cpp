#include "infrastructure/istream_uint64_extractor.hpp"
#include <stdexcept>

uint64_t istream_uint64_extractor::extract_uint64(std::istream& in) const {
    char bytes[8] = {};
    in.read(bytes, 8);
    if (!in || in.gcount() != 8)
        throw std::runtime_error("truncated binary stream");
    return static_cast<uint64_t>(static_cast<uint8_t>(bytes[0]))
        | (static_cast<uint64_t>(static_cast<uint8_t>(bytes[1])) << 8)
        | (static_cast<uint64_t>(static_cast<uint8_t>(bytes[2])) << 16)
        | (static_cast<uint64_t>(static_cast<uint8_t>(bytes[3])) << 24)
        | (static_cast<uint64_t>(static_cast<uint8_t>(bytes[4])) << 32)
        | (static_cast<uint64_t>(static_cast<uint8_t>(bytes[5])) << 40)
        | (static_cast<uint64_t>(static_cast<uint8_t>(bytes[6])) << 48)
        | (static_cast<uint64_t>(static_cast<uint8_t>(bytes[7])) << 56);
}

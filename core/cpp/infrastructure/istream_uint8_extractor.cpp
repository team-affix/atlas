#include "infrastructure/istream_uint8_extractor.hpp"
#include <stdexcept>

uint8_t istream_uint8_extractor::extract_uint8(std::istream& in) const {
    char byte = 0;
    in.read(&byte, 1);
    if (!in || in.gcount() != 1)
        throw std::runtime_error("truncated binary stream");
    return static_cast<uint8_t>(byte);
}

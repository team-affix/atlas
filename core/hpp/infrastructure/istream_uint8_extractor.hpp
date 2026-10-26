#ifndef ISTREAM_UINT8_EXTRACTOR_HPP
#define ISTREAM_UINT8_EXTRACTOR_HPP

#include <cstdint>
#include <istream>

struct istream_uint8_extractor {
    uint8_t extract_uint8(std::istream& in) const;
};

#endif

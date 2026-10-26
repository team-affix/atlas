#ifndef ISTREAM_UINT32_EXTRACTOR_HPP
#define ISTREAM_UINT32_EXTRACTOR_HPP

#include <cstdint>
#include <istream>

struct istream_uint32_extractor {
    uint32_t extract_uint32(std::istream& in) const;
};

#endif

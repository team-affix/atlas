#ifndef ISTREAM_UINT64_EXTRACTOR_HPP
#define ISTREAM_UINT64_EXTRACTOR_HPP

#include <cstdint>
#include <istream>

struct istream_uint64_extractor {
    uint64_t extract_uint64(std::istream& in) const;
};

#endif

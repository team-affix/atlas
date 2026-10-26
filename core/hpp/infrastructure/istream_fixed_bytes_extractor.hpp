#ifndef ISTREAM_FIXED_BYTES_EXTRACTOR_HPP
#define ISTREAM_FIXED_BYTES_EXTRACTOR_HPP

#include <cstddef>
#include <istream>

struct istream_fixed_bytes_extractor {
    void extract_fixed_bytes(std::istream& in, char* dest, size_t count) const;
};

#endif

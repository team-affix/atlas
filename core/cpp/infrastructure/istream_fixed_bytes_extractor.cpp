#include "infrastructure/istream_fixed_bytes_extractor.hpp"
#include <stdexcept>

void istream_fixed_bytes_extractor::extract_fixed_bytes(std::istream& in, char* dest, size_t count) const {
    in.read(dest, static_cast<std::streamsize>(count));
    if (!in || static_cast<size_t>(in.gcount()) != count)
        throw std::runtime_error("truncated binary stream");
}

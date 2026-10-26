#include "infrastructure/ostream_fixed_bytes_inserter.hpp"

void ostream_fixed_bytes_inserter::insert_fixed_bytes(std::ostream& out, const char* bytes, size_t count) const {
    out.write(bytes, static_cast<std::streamsize>(count));
}

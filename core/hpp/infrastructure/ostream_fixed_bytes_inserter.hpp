#ifndef OSTREAM_FIXED_BYTES_INSERTER_HPP
#define OSTREAM_FIXED_BYTES_INSERTER_HPP

#include <cstddef>
#include <ostream>

struct ostream_fixed_bytes_inserter {
    void insert_fixed_bytes(std::ostream& out, const char* bytes, size_t count) const;
};

#endif

#ifndef OSTREAM_UINT64_INSERTER_HPP
#define OSTREAM_UINT64_INSERTER_HPP

#include <cstdint>
#include <ostream>

struct ostream_uint64_inserter {
    void insert_uint64(std::ostream& out, uint64_t value) const;
};

#endif

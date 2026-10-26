#ifndef OSTREAM_UINT32_INSERTER_HPP
#define OSTREAM_UINT32_INSERTER_HPP

#include <cstdint>
#include <ostream>

struct ostream_uint32_inserter {
    void insert_uint32(std::ostream& out, uint32_t value) const;
};

#endif

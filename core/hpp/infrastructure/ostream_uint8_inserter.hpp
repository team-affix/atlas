#ifndef OSTREAM_UINT8_INSERTER_HPP
#define OSTREAM_UINT8_INSERTER_HPP

#include <cstdint>
#include <ostream>

struct ostream_uint8_inserter {
    void insert_uint8(std::ostream& out, uint8_t value) const;
};

#endif

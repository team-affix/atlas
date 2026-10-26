#include "infrastructure/ostream_uint8_inserter.hpp"

void ostream_uint8_inserter::insert_uint8(std::ostream& out, uint8_t value) const {
    const char byte = static_cast<char>(value);
    out.write(&byte, 1);
}

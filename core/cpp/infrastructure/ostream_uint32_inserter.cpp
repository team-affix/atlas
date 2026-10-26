#include "infrastructure/ostream_uint32_inserter.hpp"

void ostream_uint32_inserter::insert_uint32(std::ostream& out, uint32_t value) const {
    const char bytes[4] = {
        static_cast<char>(value & 0xffu),
        static_cast<char>((value >> 8) & 0xffu),
        static_cast<char>((value >> 16) & 0xffu),
        static_cast<char>((value >> 24) & 0xffu),
    };
    out.write(bytes, 4);
}

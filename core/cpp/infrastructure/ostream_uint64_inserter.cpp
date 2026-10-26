#include "infrastructure/ostream_uint64_inserter.hpp"

void ostream_uint64_inserter::insert_uint64(std::ostream& out, uint64_t value) const {
    const char bytes[8] = {
        static_cast<char>(value & 0xffu),
        static_cast<char>((value >> 8) & 0xffu),
        static_cast<char>((value >> 16) & 0xffu),
        static_cast<char>((value >> 24) & 0xffu),
        static_cast<char>((value >> 32) & 0xffu),
        static_cast<char>((value >> 40) & 0xffu),
        static_cast<char>((value >> 48) & 0xffu),
        static_cast<char>((value >> 56) & 0xffu),
    };
    out.write(bytes, 8);
}

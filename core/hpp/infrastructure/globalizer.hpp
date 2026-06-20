#ifndef GLOBALIZER_HPP
#define GLOBALIZER_HPP

#include <cstdint>

struct globalizer {
    uint32_t globalize(uint32_t frame_offset, uint32_t local_index);
};

inline uint32_t globalizer::globalize(uint32_t frame_offset, uint32_t local_index) {
    return frame_offset + local_index;
}

#endif

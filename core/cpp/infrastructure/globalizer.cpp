#include "infrastructure/globalizer.hpp"

uint32_t globalizer::globalize(uint32_t frame_offset, uint32_t local_index) {
    return frame_offset + local_index;
}

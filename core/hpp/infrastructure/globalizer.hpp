#ifndef GLOBALIZER_HPP
#define GLOBALIZER_HPP

#include "interfaces/i_globalizer.hpp"

struct globalizer : i_globalizer {
    uint32_t globalize(uint32_t frame_offset, uint32_t local_index) override;
};

#endif

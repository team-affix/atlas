#ifndef I_GLOBALIZER_HPP
#define I_GLOBALIZER_HPP

#include <cstdint>

struct i_globalizer {
    virtual ~i_globalizer() = default;
    virtual uint32_t globalize(uint32_t frame_offset, uint32_t local_index) = 0;
};

#endif

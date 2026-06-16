#ifndef I_FRAME_ALLOCATOR_HPP
#define I_FRAME_ALLOCATOR_HPP

#include <cstdint>

struct i_frame_allocator {
    virtual ~i_frame_allocator() = default;
    virtual uint32_t bump(uint32_t n) = 0;
    virtual uint32_t peek() const = 0;
    virtual void reset() = 0;
};

#endif

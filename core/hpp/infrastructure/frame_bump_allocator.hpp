#ifndef FRAME_BUMP_ALLOCATOR_HPP
#define FRAME_BUMP_ALLOCATOR_HPP

#include <cstdint>

struct frame_bump_allocator {
    frame_bump_allocator(uint32_t initial);
    uint32_t bump(uint32_t n);
    uint32_t peek() const;
    void reset();
private:
    uint32_t initial_;
    uint32_t next_frame_offset_;
};

#endif

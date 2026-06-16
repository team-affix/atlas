#ifndef FRAME_BUMP_ALLOCATOR_HPP
#define FRAME_BUMP_ALLOCATOR_HPP

#include <cstdint>
#include "interfaces/i_frame_allocator.hpp"

struct frame_bump_allocator : i_frame_allocator {
    explicit frame_bump_allocator(uint32_t initial);
    uint32_t bump(uint32_t n) override;
    uint32_t peek() const override;
    void reset() override;
private:
    uint32_t initial_;
    uint32_t next_frame_offset_;
};

#endif

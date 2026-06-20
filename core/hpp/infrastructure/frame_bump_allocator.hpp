#ifndef FRAME_BUMP_ALLOCATOR_HPP
#define FRAME_BUMP_ALLOCATOR_HPP

#include <cstdint>

struct frame_bump_allocator {
    explicit frame_bump_allocator(uint32_t initial);
    uint32_t bump(uint32_t n);
    uint32_t peek() const;
    void reset();
private:
    uint32_t initial_;
    uint32_t next_frame_offset_;
};

inline frame_bump_allocator::frame_bump_allocator(uint32_t initial)
    : initial_(initial), next_frame_offset_(initial) {}

inline uint32_t frame_bump_allocator::bump(uint32_t n) {
    const uint32_t base = next_frame_offset_;
    next_frame_offset_ += n;
    return base;
}

inline uint32_t frame_bump_allocator::peek() const {
    return next_frame_offset_;
}

inline void frame_bump_allocator::reset() {
    next_frame_offset_ = initial_;
}

#endif

#include "infrastructure/frame_bump_allocator.hpp"

frame_bump_allocator::frame_bump_allocator(uint32_t initial)
    : initial_(initial), next_frame_offset_(initial) {}

uint32_t frame_bump_allocator::bump(uint32_t n) {
    const uint32_t base = next_frame_offset_;
    next_frame_offset_ += n;
    return base;
}

uint32_t frame_bump_allocator::peek() const {
    return next_frame_offset_;
}

void frame_bump_allocator::reset() {
    next_frame_offset_ = initial_;
}

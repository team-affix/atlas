#ifndef DBUCT_FRAME_BUMP_ALLOCATOR_HPP
#define DBUCT_FRAME_BUMP_ALLOCATOR_HPP

#include <cstdint>

// Delayed-backtracking variant of frame_bump_allocator.
//
// The bump cursor is part of the per-sim state: when a choice frame is rolled
// back, the offsets handed out to candidates activated in that frame become
// free again, so the cursor must rewind too. Otherwise a subsequent (re)activation
// would collide global keys against stale bind_map bindings. snapshot()/restore()
// capture and rewind the cursor exactly.
struct dbuct_frame_bump_allocator {
    using snapshot_t = uint32_t;

    explicit dbuct_frame_bump_allocator(uint32_t initial);

    uint32_t bump(uint32_t n);
    uint32_t peek() const;
    void reset();

    snapshot_t snapshot() const;
    void restore(snapshot_t s);

private:
    uint32_t initial_;
    uint32_t next_frame_offset_;
};

inline dbuct_frame_bump_allocator::dbuct_frame_bump_allocator(uint32_t initial)
    : initial_(initial), next_frame_offset_(initial) {}

inline uint32_t dbuct_frame_bump_allocator::bump(uint32_t n) {
    const uint32_t base = next_frame_offset_;
    next_frame_offset_ += n;
    return base;
}

inline uint32_t dbuct_frame_bump_allocator::peek() const { return next_frame_offset_; }

inline void dbuct_frame_bump_allocator::reset() { next_frame_offset_ = initial_; }

inline dbuct_frame_bump_allocator::snapshot_t dbuct_frame_bump_allocator::snapshot() const { return next_frame_offset_; }
inline void dbuct_frame_bump_allocator::restore(snapshot_t s) { next_frame_offset_ = s; }

#endif

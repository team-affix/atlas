#ifndef DBUCT_FRAME_BUMP_ALLOCATOR_HPP
#define DBUCT_FRAME_BUMP_ALLOCATOR_HPP

#include <cstdint>
#include <memory>
#include "infrastructure/backtrackable_add.hpp"
#include "infrastructure/tracked.hpp"

// Delayed-backtracking variant of frame_bump_allocator.
//
// The bump cursor is part of the per-sim state: when a choice frame is rolled
// back, the offsets handed out to candidates activated in that frame become
// free again, so the cursor must rewind too (otherwise a subsequent
// (re)activation would collide global keys against stale bind_map bindings).
// The cursor is trail-journalled (via the abstract ILogTrailAction): each bump
// logs a backtrackable add whose undo subtracts the same amount, rewinding the
// cursor exactly on pop.
template<typename ILogTrailAction>
struct dbuct_frame_bump_allocator {
    dbuct_frame_bump_allocator(ILogTrailAction& t, uint32_t initial);

    uint32_t bump(uint32_t n);
    uint32_t peek() const;

private:
    tracked<uint32_t, ILogTrailAction> next_frame_offset_;
};

template<typename ILogTrailAction>
dbuct_frame_bump_allocator<ILogTrailAction>::dbuct_frame_bump_allocator(ILogTrailAction& t, uint32_t initial)
    : next_frame_offset_(t, initial) {}

template<typename ILogTrailAction>
uint32_t dbuct_frame_bump_allocator<ILogTrailAction>::bump(uint32_t n) {
    const uint32_t base = next_frame_offset_.get();
    next_frame_offset_.mutate(std::make_unique<backtrackable_add<uint32_t>>(n));
    return base;
}

template<typename ILogTrailAction>
uint32_t dbuct_frame_bump_allocator<ILogTrailAction>::peek() const { return next_frame_offset_.get(); }

#endif

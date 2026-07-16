#include "infrastructure/solver_frame_depth_tracker.hpp"

solver_frame_depth_tracker::solver_frame_depth_tracker()
    : depth_(1) {}

void solver_frame_depth_tracker::push_frame() {
    ++depth_;
}

void solver_frame_depth_tracker::pop_frame() {
    DEBUG_ASSERT(depth_ > 1);
    --depth_;
}

size_t solver_frame_depth_tracker::solver_frame_depth() const {
    return depth_;
}

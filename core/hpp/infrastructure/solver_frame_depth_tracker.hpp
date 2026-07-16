#ifndef SOLVER_FRAME_DEPTH_TRACKER_HPP
#define SOLVER_FRAME_DEPTH_TRACKER_HPP

#include <cstddef>
#include "debug_assert.hpp"

struct solver_frame_depth_tracker {
    solver_frame_depth_tracker();

    void push_frame();
    void pop_frame();
    size_t solver_frame_depth() const;

private:
    size_t depth_;
};

#endif

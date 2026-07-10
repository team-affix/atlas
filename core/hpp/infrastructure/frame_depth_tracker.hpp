#ifndef FRAME_DEPTH_TRACKER_HPP
#define FRAME_DEPTH_TRACKER_HPP

#include <cstddef>

struct frame_depth_tracker {
    void push() { ++depth_; }
    void pop() { --depth_; }
    size_t depth() const { return depth_; }

private:
    size_t depth_ = 0;
};

#endif

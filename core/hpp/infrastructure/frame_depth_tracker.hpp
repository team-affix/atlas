#ifndef FRAME_DEPTH_TRACKER_HPP
#define FRAME_DEPTH_TRACKER_HPP

#include <cstddef>

struct frame_depth_tracker {
    void push();
    void pop();
    size_t depth() const;

private:
    size_t depth_ = 0;
};

#endif

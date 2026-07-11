#include "infrastructure/frame_depth_tracker.hpp"

void frame_depth_tracker::push() { ++depth_; }

void frame_depth_tracker::pop() { --depth_; }

size_t frame_depth_tracker::depth() const { return depth_; }

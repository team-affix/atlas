#include "infrastructure/frame_depth_tracker.hpp"

frame_depth_tracker::frame_depth_tracker() : depth_(1) {}


void frame_depth_tracker::push() { ++depth_; }

void frame_depth_tracker::pop() { --depth_; }

size_t frame_depth_tracker::depth() const { return depth_; }
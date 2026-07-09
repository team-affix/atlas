#ifndef DBUCT_FRAME_COUNT_HPP
#define DBUCT_FRAME_COUNT_HPP

#include <cstddef>

// Shared frame-depth counter for the dbuct camping stack. dbuct_sim moves it in
// lockstep with the CDCL learner's push_frame/pop_frame (one camp frame per trail
// frame), so depth() always mirrors the learner's internal frame_stack_ size.
// dbuct_avoidance_unit_boundary reads depth() to stamp each learned conflict with
// the frame index it was raised at; keeping this a standalone object (rather than
// dbuct_sim exposing depth()) breaks the otherwise-cyclic template graph
// aub -> sim -> cdcl -> aub.
struct dbuct_frame_count {
    // Starts at 1 to match the root frame the CDCL learner pushes in its ctor.
    size_t depth() const { return depth_; }
    void push() { ++depth_; }
    void pop() { --depth_; }

private:
    size_t depth_ = 1;
};

#endif

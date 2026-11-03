#ifndef PUD_UNFOLD_DEPTH_RECORDER_HPP
#define PUD_UNFOLD_DEPTH_RECORDER_HPP

#include <cstddef>
#include "value_objects/pud_body_goal_id.hpp"
#include "value_objects/pud_global_node_id.hpp"
#include "value_objects/pud_unfold_outcome.hpp"

// Wraps the unfold slot: adds the depth of every unfolded leaf to a running sum, then forwards the unfold.
// Dividing the change in the sum by the change in the unfold count gives the average unfold depth.
template<typename IGetNodeDepth, typename IUnfoldLeaf>
struct pud_unfold_depth_recorder {
    pud_unfold_depth_recorder(IGetNodeDepth& get_node_depth, IUnfoldLeaf& unfold_leaf);
    pud_unfold_outcome unfold(pud_global_node_id leaf, pud_body_goal_id goal);
    size_t depth_sum() const;
private:
    IGetNodeDepth& get_node_depth_;
    IUnfoldLeaf& unfold_leaf_;
    size_t depth_sum_;
};

#define PUD_UNFOLD_DEPTH_RECORDER_TEMPLATE template<typename IGetNodeDepth, typename IUnfoldLeaf>
#define PUD_UNFOLD_DEPTH_RECORDER pud_unfold_depth_recorder<IGetNodeDepth, IUnfoldLeaf>

PUD_UNFOLD_DEPTH_RECORDER_TEMPLATE
PUD_UNFOLD_DEPTH_RECORDER::pud_unfold_depth_recorder(IGetNodeDepth& get_node_depth, IUnfoldLeaf& unfold_leaf)
    : get_node_depth_(get_node_depth)
    , unfold_leaf_(unfold_leaf)
    , depth_sum_(0) {}

PUD_UNFOLD_DEPTH_RECORDER_TEMPLATE
pud_unfold_outcome PUD_UNFOLD_DEPTH_RECORDER::unfold(pud_global_node_id leaf, pud_body_goal_id goal) {
    depth_sum_ += get_node_depth_.get(leaf);
    return unfold_leaf_.unfold(leaf, goal);
}

PUD_UNFOLD_DEPTH_RECORDER_TEMPLATE
size_t PUD_UNFOLD_DEPTH_RECORDER::depth_sum() const {
    return depth_sum_;
}

#undef PUD_UNFOLD_DEPTH_RECORDER
#undef PUD_UNFOLD_DEPTH_RECORDER_TEMPLATE

#endif

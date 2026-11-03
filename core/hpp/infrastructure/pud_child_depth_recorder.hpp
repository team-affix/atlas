#ifndef PUD_CHILD_DEPTH_RECORDER_HPP
#define PUD_CHILD_DEPTH_RECORDER_HPP

#include "value_objects/pud_global_node_id.hpp"

// Fills the activate-child slot: records the child one deeper than its parent, then activates it.
// Wrapping activation keeps depth inside the one activation pass from the axioms down to the leaves.
template<typename IGetNodeDepth, typename IStoreNodeDepth, typename IActivateChild>
struct pud_child_depth_recorder {
    pud_child_depth_recorder(
        IGetNodeDepth& get_node_depth, IStoreNodeDepth& store_node_depth, IActivateChild& activate_child);
    bool activate_child(pud_global_node_id parent, pud_global_node_id child);
private:
    IGetNodeDepth& get_node_depth_;
    IStoreNodeDepth& store_node_depth_;
    IActivateChild& activate_child_;
};

#define PUD_CHILD_DEPTH_RECORDER_TEMPLATE template<typename IGetNodeDepth, typename IStoreNodeDepth, typename IActivateChild>
#define PUD_CHILD_DEPTH_RECORDER          pud_child_depth_recorder<IGetNodeDepth, IStoreNodeDepth, IActivateChild>

PUD_CHILD_DEPTH_RECORDER_TEMPLATE
PUD_CHILD_DEPTH_RECORDER::pud_child_depth_recorder(
    IGetNodeDepth& get_node_depth, IStoreNodeDepth& store_node_depth, IActivateChild& activate_child)
    : get_node_depth_(get_node_depth)
    , store_node_depth_(store_node_depth)
    , activate_child_(activate_child) {}

PUD_CHILD_DEPTH_RECORDER_TEMPLATE
bool PUD_CHILD_DEPTH_RECORDER::activate_child(pud_global_node_id parent, pud_global_node_id child) {
    store_node_depth_.store(child, get_node_depth_.get(parent) + 1);
    return activate_child_.activate_child(parent, child);
}

#undef PUD_CHILD_DEPTH_RECORDER
#undef PUD_CHILD_DEPTH_RECORDER_TEMPLATE

#endif

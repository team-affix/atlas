#ifndef PUD_ROOT_DEPTH_RECORDER_HPP
#define PUD_ROOT_DEPTH_RECORDER_HPP

#include "value_objects/pud_global_node_id.hpp"

// Fills the activate-root slot: records the root at depth 0, then activates it. Wrapping activation keeps
// depth inside the one activation pass from the axioms down to the leaves.
template<typename IStoreNodeDepth, typename IActivateRoot>
struct pud_root_depth_recorder {
    pud_root_depth_recorder(IStoreNodeDepth& store_node_depth, IActivateRoot& activate_root);
    bool activate_root(pud_global_node_id root);
private:
    IStoreNodeDepth& store_node_depth_;
    IActivateRoot& activate_root_;
};

template<typename IStoreNodeDepth, typename IActivateRoot>
pud_root_depth_recorder<IStoreNodeDepth, IActivateRoot>::pud_root_depth_recorder(
    IStoreNodeDepth& store_node_depth, IActivateRoot& activate_root)
    : store_node_depth_(store_node_depth)
    , activate_root_(activate_root) {}

template<typename IStoreNodeDepth, typename IActivateRoot>
bool pud_root_depth_recorder<IStoreNodeDepth, IActivateRoot>::activate_root(pud_global_node_id root) {
    store_node_depth_.store(root, 0);
    return activate_root_.activate_root(root);
}

#endif

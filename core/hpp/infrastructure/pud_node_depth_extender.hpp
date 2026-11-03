#ifndef PUD_NODE_DEPTH_EXTENDER_HPP
#define PUD_NODE_DEPTH_EXTENDER_HPP

#include <cstddef>
#include <vector>
#include "value_objects/pud_global_node_id.hpp"

// On unfold, records each new child one deeper than the unfolded parent.
template<typename IGetNodeDepth, typename IInsertNodeDepth>
struct pud_node_depth_extender {
    pud_node_depth_extender(IGetNodeDepth& get_node_depth, IInsertNodeDepth& insert_node_depth);
    void extend(pud_global_node_id parent, const std::vector<pud_global_node_id>& offspring);
private:
    IGetNodeDepth& get_node_depth_;
    IInsertNodeDepth& insert_node_depth_;
};

template<typename IGetNodeDepth, typename IInsertNodeDepth>
pud_node_depth_extender<IGetNodeDepth, IInsertNodeDepth>::pud_node_depth_extender(
    IGetNodeDepth& get_node_depth, IInsertNodeDepth& insert_node_depth)
    : get_node_depth_(get_node_depth)
    , insert_node_depth_(insert_node_depth) {}

template<typename IGetNodeDepth, typename IInsertNodeDepth>
void pud_node_depth_extender<IGetNodeDepth, IInsertNodeDepth>::extend(
    pud_global_node_id parent, const std::vector<pud_global_node_id>& offspring) {
    const size_t child_depth = get_node_depth_.get_depth(parent) + 1;
    for (const pud_global_node_id child : offspring)
        insert_node_depth_.insert(child, child_depth);
}

#endif

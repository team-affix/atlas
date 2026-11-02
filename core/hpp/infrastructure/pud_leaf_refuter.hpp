#ifndef PUD_LEAF_REFUTER_HPP
#define PUD_LEAF_REFUTER_HPP

#include <vector>
#include "value_objects/pud_global_node_id.hpp"

template<
    typename IMarkLeafRefuted,
    typename IPropagateRefutation,
    typename IDeactivateNode,
    typename IIsNodeLeaf,
    typename IIsNodeRefuted>
struct pud_leaf_refuter {
    pud_leaf_refuter(
        IMarkLeafRefuted& mark_leaf_refuted,
        IPropagateRefutation& propagate_refutation,
        IDeactivateNode& deactivate_node,
        IIsNodeLeaf& is_node_leaf,
        IIsNodeRefuted& is_node_refuted);
    std::vector<pud_global_node_id> refute(pud_global_node_id leaf);
private:
    IMarkLeafRefuted& mark_leaf_refuted_;
    IPropagateRefutation& propagate_refutation_;
    IDeactivateNode& deactivate_node_;
    IIsNodeLeaf& is_node_leaf_;
    IIsNodeRefuted& is_node_refuted_;
};

#define PUD_LEAF_REFUTER_TEMPLATE                                                          \
    template<typename IMarkLeafRefuted, typename IPropagateRefutation, typename IDeactivateNode, \
             typename IIsNodeLeaf, typename IIsNodeRefuted>
#define PUD_LEAF_REFUTER                                                                   \
    pud_leaf_refuter<IMarkLeafRefuted, IPropagateRefutation, IDeactivateNode, IIsNodeLeaf, IIsNodeRefuted>

PUD_LEAF_REFUTER_TEMPLATE
PUD_LEAF_REFUTER::pud_leaf_refuter(
    IMarkLeafRefuted& mark_leaf_refuted,
    IPropagateRefutation& propagate_refutation,
    IDeactivateNode& deactivate_node,
    IIsNodeLeaf& is_node_leaf,
    IIsNodeRefuted& is_node_refuted)
    : mark_leaf_refuted_(mark_leaf_refuted)
    , propagate_refutation_(propagate_refutation)
    , deactivate_node_(deactivate_node)
    , is_node_leaf_(is_node_leaf)
    , is_node_refuted_(is_node_refuted) {}

PUD_LEAF_REFUTER_TEMPLATE
std::vector<pud_global_node_id> PUD_LEAF_REFUTER::refute(pud_global_node_id leaf) {
    const bool left_alone = !is_node_leaf_.is_leaf(leaf) || is_node_refuted_.is_refuted(leaf);
    if (left_alone)
        return {};
    mark_leaf_refuted_.mark_refuted(leaf);
    propagate_refutation_.refute(leaf);
    std::vector<pud_global_node_id> refuted{leaf};
    for (pud_global_node_id stranded : deactivate_node_.deactivate(leaf)) {
        const std::vector<pud_global_node_id> cascaded = refute(stranded);
        refuted.insert(refuted.end(), cascaded.begin(), cascaded.end());
    }
    return refuted;
}

#undef PUD_LEAF_REFUTER
#undef PUD_LEAF_REFUTER_TEMPLATE

#endif

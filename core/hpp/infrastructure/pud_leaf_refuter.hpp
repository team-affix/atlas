#ifndef PUD_LEAF_REFUTER_HPP
#define PUD_LEAF_REFUTER_HPP

#include <vector>
#include "value_objects/pud_global_node_id.hpp"

// Refutes a leaf and everything that follows from it: the leaf is marked refuted, its death is
// propagated up the tree, it is deactivated, and every leaf the deactivation stranded on a query
// without a candidate is refuted in turn. Returns every leaf refuted, the given one first. A leaf
// already refuted is left alone, and so is an interior node: its children inherit the exhausted query
// and arrive refuted, or never needed it.
template<
    typename IMarkLeafRefuted,
    typename IPropagateRefutation,
    typename IDeactivateNode,
    typename ICheckNodeLeaf,
    typename ICheckLeafRefuted>
struct pud_leaf_refuter {
    pud_leaf_refuter(
        IMarkLeafRefuted& mark_leaf_refuted,
        IPropagateRefutation& propagate_refutation,
        IDeactivateNode& deactivate_node,
        ICheckNodeLeaf& check_node_leaf,
        ICheckLeafRefuted& check_leaf_refuted);
    std::vector<pud_global_node_id> refute(pud_global_node_id leaf);
private:
    IMarkLeafRefuted& mark_leaf_refuted_;
    IPropagateRefutation& propagate_refutation_;
    IDeactivateNode& deactivate_node_;
    ICheckNodeLeaf& check_node_leaf_;
    ICheckLeafRefuted& check_leaf_refuted_;
};

#define PUD_LEAF_REFUTER_TEMPLATE                                                          \
    template<typename IMarkLeafRefuted, typename IPropagateRefutation, typename IDeactivateNode, \
             typename ICheckNodeLeaf, typename ICheckLeafRefuted>
#define PUD_LEAF_REFUTER                                                                   \
    pud_leaf_refuter<IMarkLeafRefuted, IPropagateRefutation, IDeactivateNode, ICheckNodeLeaf, ICheckLeafRefuted>

PUD_LEAF_REFUTER_TEMPLATE
PUD_LEAF_REFUTER::pud_leaf_refuter(
    IMarkLeafRefuted& mark_leaf_refuted,
    IPropagateRefutation& propagate_refutation,
    IDeactivateNode& deactivate_node,
    ICheckNodeLeaf& check_node_leaf,
    ICheckLeafRefuted& check_leaf_refuted)
    : mark_leaf_refuted_(mark_leaf_refuted)
    , propagate_refutation_(propagate_refutation)
    , deactivate_node_(deactivate_node)
    , check_node_leaf_(check_node_leaf)
    , check_leaf_refuted_(check_leaf_refuted) {}

PUD_LEAF_REFUTER_TEMPLATE
std::vector<pud_global_node_id> PUD_LEAF_REFUTER::refute(pud_global_node_id leaf) {
    const bool left_alone = !check_node_leaf_.check_leaf(leaf) || check_leaf_refuted_.check_refuted(leaf);
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

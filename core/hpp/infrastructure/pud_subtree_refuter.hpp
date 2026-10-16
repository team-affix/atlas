#ifndef PUD_SUBTREE_REFUTER_HPP
#define PUD_SUBTREE_REFUTER_HPP

#include <vector>
#include "value_objects/pud_node_id.hpp"

// A refuted node's whole subtree is refuted, so an already-refuted node is skipped together
// with everything below it. Returns the leaves it refuted, all marked before any is returned.
template<
    typename ICheckNodeRefuted,
    typename IMarkRefuted,
    typename IGetNodeChildren,
    typename ICheckNodeLeaf>
struct pud_subtree_refuter {
    pud_subtree_refuter(
        ICheckNodeRefuted& check_node_refuted,
        IMarkRefuted& mark_refuted,
        IGetNodeChildren& get_node_children,
        ICheckNodeLeaf& check_node_leaf);
    std::vector<pud_node_id> refute(pud_node_id root);
private:
    ICheckNodeRefuted& check_node_refuted_;
    IMarkRefuted& mark_refuted_;
    IGetNodeChildren& get_node_children_;
    ICheckNodeLeaf& check_node_leaf_;
};

#define PUD_SUBTREE_REFUTER_TEMPLATE                                                       \
    template<typename ICheckNodeRefuted, typename IMarkRefuted, typename IGetNodeChildren, \
             typename ICheckNodeLeaf>
#define PUD_SUBTREE_REFUTER                                                                \
    pud_subtree_refuter<ICheckNodeRefuted, IMarkRefuted, IGetNodeChildren, ICheckNodeLeaf>

PUD_SUBTREE_REFUTER_TEMPLATE
PUD_SUBTREE_REFUTER::pud_subtree_refuter(
    ICheckNodeRefuted& check_node_refuted,
    IMarkRefuted& mark_refuted,
    IGetNodeChildren& get_node_children,
    ICheckNodeLeaf& check_node_leaf)
    : check_node_refuted_(check_node_refuted)
    , mark_refuted_(mark_refuted)
    , get_node_children_(get_node_children)
    , check_node_leaf_(check_node_leaf) {}

PUD_SUBTREE_REFUTER_TEMPLATE
std::vector<pud_node_id> PUD_SUBTREE_REFUTER::refute(pud_node_id root) {
    std::vector<pud_node_id> refuted_leaves;
    std::vector<pud_node_id> pending{root};
    while (!pending.empty()) {
        const pud_node_id node = pending.back();
        pending.pop_back();
        if (check_node_refuted_.check_refuted(node))
            continue;
        mark_refuted_.mark_refuted(node);
        if (check_node_leaf_.check_leaf(node)) {
            refuted_leaves.push_back(node);
            continue;
        }
        for (pud_node_id child : get_node_children_.get(node))
            pending.push_back(child);
    }
    return refuted_leaves;
}

#undef PUD_SUBTREE_REFUTER
#undef PUD_SUBTREE_REFUTER_TEMPLATE

#endif

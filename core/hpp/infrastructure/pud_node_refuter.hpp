#ifndef PUD_NODE_REFUTER_HPP
#define PUD_NODE_REFUTER_HPP

#include <variant>
#include <vector>
#include "value_objects/pud_forced_query_propagation.hpp"
#include "value_objects/pud_node_id.hpp"

// Refutes a node's subtree and everything that follows from it: each refuted query refutes the
// node that opened it. Every live leaf of a subtree is marked before any is invalidated.
template<
    typename IFindLiveLeaves,
    typename IMarkLeafRefuted,
    typename IPropagateRefutation,
    typename IEraseNodeDescent,
    typename IInvalidateLeaves,
    typename IGetQueryHeadNode>
struct pud_node_refuter {
    pud_node_refuter(
        IFindLiveLeaves& find_live_leaves,
        IMarkLeafRefuted& mark_leaf_refuted,
        IPropagateRefutation& propagate_refutation,
        IEraseNodeDescent& erase_node_descent,
        IInvalidateLeaves& invalidate_leaves,
        IGetQueryHeadNode& get_query_head_node);
    void refute(pud_node_id node);
private:
    IFindLiveLeaves& find_live_leaves_;
    IMarkLeafRefuted& mark_leaf_refuted_;
    IPropagateRefutation& propagate_refutation_;
    IEraseNodeDescent& erase_node_descent_;
    IInvalidateLeaves& invalidate_leaves_;
    IGetQueryHeadNode& get_query_head_node_;
};

#define PUD_NODE_REFUTER_TEMPLATE                                                          \
    template<typename IFindLiveLeaves, typename IMarkLeafRefuted, typename IPropagateRefutation, \
             typename IEraseNodeDescent, typename IInvalidateLeaves, typename IGetQueryHeadNode>
#define PUD_NODE_REFUTER                                                                   \
    pud_node_refuter<IFindLiveLeaves, IMarkLeafRefuted, IPropagateRefutation,              \
        IEraseNodeDescent, IInvalidateLeaves, IGetQueryHeadNode>

PUD_NODE_REFUTER_TEMPLATE
PUD_NODE_REFUTER::pud_node_refuter(
    IFindLiveLeaves& find_live_leaves,
    IMarkLeafRefuted& mark_leaf_refuted,
    IPropagateRefutation& propagate_refutation,
    IEraseNodeDescent& erase_node_descent,
    IInvalidateLeaves& invalidate_leaves,
    IGetQueryHeadNode& get_query_head_node)
    : find_live_leaves_(find_live_leaves)
    , mark_leaf_refuted_(mark_leaf_refuted)
    , propagate_refutation_(propagate_refutation)
    , erase_node_descent_(erase_node_descent)
    , invalidate_leaves_(invalidate_leaves)
    , get_query_head_node_(get_query_head_node) {}

PUD_NODE_REFUTER_TEMPLATE
void PUD_NODE_REFUTER::refute(pud_node_id node) {
    std::vector<pud_node_id> pending{node};
    while (!pending.empty()) {
        const pud_node_id refuted = pending.back();
        pending.pop_back();
        const std::vector<pud_node_id> live_leaves = find_live_leaves_.find_live_leaves(refuted);
        for (pud_node_id leaf : live_leaves) {
            mark_leaf_refuted_.mark_refuted(leaf);
            propagate_refutation_.refute(leaf);
            erase_node_descent_.erase(leaf);
        }
        for (const pud_forced_query_propagation& forced : invalidate_leaves_.invalidate(live_leaves)) {
            const pud_refuted_query* refuted_query = std::get_if<pud_refuted_query>(&forced);
            if (!refuted_query)
                continue;
            pending.push_back(get_query_head_node_.get(refuted_query->head_id));
        }
    }
}

#undef PUD_NODE_REFUTER
#undef PUD_NODE_REFUTER_TEMPLATE

#endif

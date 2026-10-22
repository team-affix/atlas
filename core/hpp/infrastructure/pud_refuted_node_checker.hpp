#ifndef PUD_REFUTED_NODE_CHECKER_HPP
#define PUD_REFUTED_NODE_CHECKER_HPP

#include "value_objects/pud_node_id.hpp"

template<typename ICheckNodeLeaf, typename ICheckLeafRefuted, typename IGetLiveChildCount>
struct pud_refuted_node_checker {
    pud_refuted_node_checker(
        ICheckNodeLeaf& check_node_leaf,
        ICheckLeafRefuted& check_leaf_refuted,
        IGetLiveChildCount& get_live_child_count);
    bool check_refuted(pud_node_id node) const;
private:
    ICheckNodeLeaf& check_node_leaf_;
    ICheckLeafRefuted& check_leaf_refuted_;
    IGetLiveChildCount& get_live_child_count_;
};

#define PUD_REFUTED_NODE_CHECKER_TEMPLATE                                                  \
    template<typename ICheckNodeLeaf, typename ICheckLeafRefuted, typename IGetLiveChildCount>
#define PUD_REFUTED_NODE_CHECKER                                                           \
    pud_refuted_node_checker<ICheckNodeLeaf, ICheckLeafRefuted, IGetLiveChildCount>

PUD_REFUTED_NODE_CHECKER_TEMPLATE
PUD_REFUTED_NODE_CHECKER::pud_refuted_node_checker(
    ICheckNodeLeaf& check_node_leaf,
    ICheckLeafRefuted& check_leaf_refuted,
    IGetLiveChildCount& get_live_child_count)
    : check_node_leaf_(check_node_leaf)
    , check_leaf_refuted_(check_leaf_refuted)
    , get_live_child_count_(get_live_child_count) {}

PUD_REFUTED_NODE_CHECKER_TEMPLATE
bool PUD_REFUTED_NODE_CHECKER::check_refuted(pud_node_id node) const {
    if (check_node_leaf_.check_leaf(node))
        return check_leaf_refuted_.check_refuted(node);
    return get_live_child_count_.get(node) == 0;
}

#undef PUD_REFUTED_NODE_CHECKER
#undef PUD_REFUTED_NODE_CHECKER_TEMPLATE

#endif

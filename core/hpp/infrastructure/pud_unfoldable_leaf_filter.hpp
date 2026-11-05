#ifndef PUD_UNFOLDABLE_LEAF_FILTER_HPP
#define PUD_UNFOLDABLE_LEAF_FILTER_HPP

#include "value_objects/pud_global_node_id.hpp"
#include "value_objects/pud_goal_list.hpp"

// Tells whether a node may be unfolded now: it is still an unrefuted leaf, it has an open goal, and the node
// limits allow it.
template<typename IIsNodeLeaf, typename IIsNodeRefuted, typename IGetNodeGoals, typename IAllowUnfold>
struct pud_unfoldable_leaf_filter {
    pud_unfoldable_leaf_filter(
        IIsNodeLeaf& is_node_leaf,
        IIsNodeRefuted& is_node_refuted,
        IGetNodeGoals& get_node_goals,
        IAllowUnfold& allow_unfold);
    bool is_unfoldable(pud_global_node_id node) const;
private:
    IIsNodeLeaf& is_node_leaf_;
    IIsNodeRefuted& is_node_refuted_;
    IGetNodeGoals& get_node_goals_;
    IAllowUnfold& allow_unfold_;
};

#define PUD_UNFOLDABLE_LEAF_FILTER_TEMPLATE                                                              \
    template<typename IIsNodeLeaf, typename IIsNodeRefuted, typename IGetNodeGoals, typename IAllowUnfold>
#define PUD_UNFOLDABLE_LEAF_FILTER                                                                       \
    pud_unfoldable_leaf_filter<IIsNodeLeaf, IIsNodeRefuted, IGetNodeGoals, IAllowUnfold>

PUD_UNFOLDABLE_LEAF_FILTER_TEMPLATE
PUD_UNFOLDABLE_LEAF_FILTER::pud_unfoldable_leaf_filter(
    IIsNodeLeaf& is_node_leaf,
    IIsNodeRefuted& is_node_refuted,
    IGetNodeGoals& get_node_goals,
    IAllowUnfold& allow_unfold)
    : is_node_leaf_(is_node_leaf)
    , is_node_refuted_(is_node_refuted)
    , get_node_goals_(get_node_goals)
    , allow_unfold_(allow_unfold) {}

PUD_UNFOLDABLE_LEAF_FILTER_TEMPLATE
bool PUD_UNFOLDABLE_LEAF_FILTER::is_unfoldable(pud_global_node_id node) const {
    if (!is_node_leaf_.is_leaf(node) || is_node_refuted_.is_refuted(node))
        return false;
    if (get_node_goals_.get(node).goals.empty())
        return false;
    return allow_unfold_.allow(node);
}

#undef PUD_UNFOLDABLE_LEAF_FILTER
#undef PUD_UNFOLDABLE_LEAF_FILTER_TEMPLATE

#endif

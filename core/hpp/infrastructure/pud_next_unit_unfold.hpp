#ifndef PUD_NEXT_UNIT_UNFOLD_HPP
#define PUD_NEXT_UNIT_UNFOLD_HPP

#include <optional>
#include <vector>
#include "value_objects/body_goal_id.hpp"
#include "value_objects/pud_mhq_head_id.hpp"
#include "value_objects/pud_node_id.hpp"
#include "value_objects/pud_unfold.hpp"

// Finds the next forced unfold: the first live leaf that still holds the oldest unit query head
// itself, together with the goal it holds it for. The search starts at the node that opened the
// query and skips every node where the query was forked, along with its descendants, since those
// only hold forks. A head that is gone, or that no live leaf holds any more, is popped; one leaf is
// returned at a time.
template<
    typename ICheckUnitQueriesEmpty,
    typename IFrontUnitQuery,
    typename IPopUnitQuery,
    typename ICheckQueryHead,
    typename IGetQueryHeadNode,
    typename ICheckNodeRefuted,
    typename ICheckQueryForked,
    typename ICheckNodeLeaf,
    typename IGetChildren,
    typename IGetNodeDescent>
struct pud_next_unit_unfold {
    pud_next_unit_unfold(
        ICheckUnitQueriesEmpty& check_unit_queries_empty,
        IFrontUnitQuery& front_unit_query,
        IPopUnitQuery& pop_unit_query,
        ICheckQueryHead& check_query_head,
        IGetQueryHeadNode& get_query_head_node,
        ICheckNodeRefuted& check_node_refuted,
        ICheckQueryForked& check_query_forked,
        ICheckNodeLeaf& check_node_leaf,
        IGetChildren& get_children,
        IGetNodeDescent& get_node_descent);
    std::optional<pud_unfold> get_next_unit_unfold();
private:
    std::optional<pud_unfold> find_holding_leaf(pud_mhq_head_id head_id);

    ICheckUnitQueriesEmpty& check_unit_queries_empty_;
    IFrontUnitQuery& front_unit_query_;
    IPopUnitQuery& pop_unit_query_;
    ICheckQueryHead& check_query_head_;
    IGetQueryHeadNode& get_query_head_node_;
    ICheckNodeRefuted& check_node_refuted_;
    ICheckQueryForked& check_query_forked_;
    ICheckNodeLeaf& check_node_leaf_;
    IGetChildren& get_children_;
    IGetNodeDescent& get_node_descent_;
};

#define PUD_NEXT_UNIT_UNFOLD_TEMPLATE                                                      \
    template<typename ICheckUnitQueriesEmpty, typename IFrontUnitQuery,                    \
             typename IPopUnitQuery, typename ICheckQueryHead, typename IGetQueryHeadNode, \
             typename ICheckNodeRefuted, typename ICheckQueryForked, typename ICheckNodeLeaf, \
             typename IGetChildren, typename IGetNodeDescent>
#define PUD_NEXT_UNIT_UNFOLD                                                               \
    pud_next_unit_unfold<ICheckUnitQueriesEmpty, IFrontUnitQuery, IPopUnitQuery,           \
        ICheckQueryHead, IGetQueryHeadNode, ICheckNodeRefuted, ICheckQueryForked,          \
        ICheckNodeLeaf, IGetChildren, IGetNodeDescent>

PUD_NEXT_UNIT_UNFOLD_TEMPLATE
PUD_NEXT_UNIT_UNFOLD::pud_next_unit_unfold(
    ICheckUnitQueriesEmpty& check_unit_queries_empty,
    IFrontUnitQuery& front_unit_query,
    IPopUnitQuery& pop_unit_query,
    ICheckQueryHead& check_query_head,
    IGetQueryHeadNode& get_query_head_node,
    ICheckNodeRefuted& check_node_refuted,
    ICheckQueryForked& check_query_forked,
    ICheckNodeLeaf& check_node_leaf,
    IGetChildren& get_children,
    IGetNodeDescent& get_node_descent)
    : check_unit_queries_empty_(check_unit_queries_empty)
    , front_unit_query_(front_unit_query)
    , pop_unit_query_(pop_unit_query)
    , check_query_head_(check_query_head)
    , get_query_head_node_(get_query_head_node)
    , check_node_refuted_(check_node_refuted)
    , check_query_forked_(check_query_forked)
    , check_node_leaf_(check_node_leaf)
    , get_children_(get_children)
    , get_node_descent_(get_node_descent) {}

PUD_NEXT_UNIT_UNFOLD_TEMPLATE
std::optional<pud_unfold> PUD_NEXT_UNIT_UNFOLD::get_next_unit_unfold() {
    while (!check_unit_queries_empty_.empty()) {
        const pud_mhq_head_id head_id = front_unit_query_.front();
        if (!check_query_head_.check_head(head_id)) {
            pop_unit_query_.pop();
            continue;
        }
        const std::optional<pud_unfold> unit_unfold = find_holding_leaf(head_id);
        if (unit_unfold.has_value())
            return unit_unfold;
        pop_unit_query_.pop();
    }
    return std::nullopt;
}

PUD_NEXT_UNIT_UNFOLD_TEMPLATE
std::optional<pud_unfold> PUD_NEXT_UNIT_UNFOLD::find_holding_leaf(pud_mhq_head_id head_id) {
    std::vector<pud_node_id> pending{get_query_head_node_.get(head_id)};
    while (!pending.empty()) {
        const pud_node_id node = pending.back();
        pending.pop_back();
        if (check_node_refuted_.check_refuted(node) || check_query_forked_.check_query_forked(node, head_id))
            continue;
        if (!check_node_leaf_.check_leaf(node)) {
            for (pud_node_id child : get_children_.get(node))
                pending.push_back(child);
            continue;
        }
        const std::optional<body_goal_id> goal = get_node_descent_.get(node).body_goal_pq().find_goal(head_id);
        if (!goal.has_value())
            continue;
        return pud_unfold{.leaf = node, .goal = goal.value()};
    }
    return std::nullopt;
}

#undef PUD_NEXT_UNIT_UNFOLD
#undef PUD_NEXT_UNIT_UNFOLD_TEMPLATE

#endif

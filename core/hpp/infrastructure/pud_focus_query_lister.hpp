#ifndef PUD_FOCUS_QUERY_LISTER_HPP
#define PUD_FOCUS_QUERY_LISTER_HPP

#include <algorithm>
#include <vector>
#include "value_objects/pud_active_state.hpp"
#include "value_objects/pud_body_goal_id.hpp"
#include "value_objects/pud_global_node_id.hpp"
#include "value_objects/pud_leaf_goal_query.hpp"
#include "value_objects/pud_mhq_head_id.hpp"
#include "debug_assert.hpp"

// Lists every body goal of every live, unrefuted leaf in the focus, in leaf order and then
// goal order, with whether its query still has candidates. A live leaf holds a query for every goal.
// A failed open or fork refutes the leaf, so it is no longer listed.
template<
    typename IGetLiveNodes,
    typename IContainsFocusNode,
    typename IIsNodeRefuted,
    typename IGetActiveState,
    typename IGetNodeGoals,
    typename IIsQueryExhausted>
struct pud_focus_query_lister {
    pud_focus_query_lister(
        IGetLiveNodes& get_live_nodes,
        IContainsFocusNode& contains_focus_node,
        IIsNodeRefuted& is_node_refuted,
        IGetActiveState& get_active_state,
        IGetNodeGoals& get_node_goals,
        IIsQueryExhausted& is_query_exhausted);
    std::vector<pud_leaf_goal_query> list() const;
private:
    IGetLiveNodes& get_live_nodes_;
    IContainsFocusNode& contains_focus_node_;
    IIsNodeRefuted& is_node_refuted_;
    IGetActiveState& get_active_state_;
    IGetNodeGoals& get_node_goals_;
    IIsQueryExhausted& is_query_exhausted_;
};

#define PUD_FOCUS_QUERY_LISTER_TEMPLATE                                                          \
    template<typename IGetLiveNodes, typename IContainsFocusNode, typename IIsNodeRefuted,         \
             typename IGetActiveState, typename IGetNodeGoals, typename IIsQueryExhausted>
#define PUD_FOCUS_QUERY_LISTER                                                                   \
    pud_focus_query_lister<IGetLiveNodes, IContainsFocusNode, IIsNodeRefuted, IGetActiveState, \
        IGetNodeGoals, IIsQueryExhausted>

PUD_FOCUS_QUERY_LISTER_TEMPLATE
PUD_FOCUS_QUERY_LISTER::pud_focus_query_lister(
    IGetLiveNodes& get_live_nodes,
    IContainsFocusNode& contains_focus_node,
    IIsNodeRefuted& is_node_refuted,
    IGetActiveState& get_active_state,
    IGetNodeGoals& get_node_goals,
    IIsQueryExhausted& is_query_exhausted)
    : get_live_nodes_(get_live_nodes)
    , contains_focus_node_(contains_focus_node)
    , is_node_refuted_(is_node_refuted)
    , get_active_state_(get_active_state)
    , get_node_goals_(get_node_goals)
    , is_query_exhausted_(is_query_exhausted) {}

PUD_FOCUS_QUERY_LISTER_TEMPLATE
std::vector<pud_leaf_goal_query> PUD_FOCUS_QUERY_LISTER::list() const {
    std::vector<pud_leaf_goal_query> queries;
    for (const pud_global_node_id node : get_live_nodes_.live()) {
        if (!contains_focus_node_.contains(node))
            continue;
        if (is_node_refuted_.is_refuted(node))
            continue;
        const pud_active_state& state = get_active_state_.get(node);
        std::vector<pud_body_goal_id> goals;
        for (const auto& [goal, goal_expr] : get_node_goals_.get(node).goals)
            goals.push_back(goal);
        std::sort(goals.begin(), goals.end());
        for (const pud_body_goal_id goal : goals) {
            const pud_mhq_head_id* head = state.queries.find(goal);
            DEBUG_ASSERT(head != nullptr);
            const bool has_candidates = !is_query_exhausted_.is_exhausted(*head);
            queries.push_back(
                pud_leaf_goal_query{.node = node, .goal = goal, .head = *head, .has_candidates = has_candidates});
        }
    }
    return queries;
}

#undef PUD_FOCUS_QUERY_LISTER
#undef PUD_FOCUS_QUERY_LISTER_TEMPLATE

#endif

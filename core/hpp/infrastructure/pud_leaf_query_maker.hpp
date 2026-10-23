#ifndef PUD_LEAF_QUERY_MAKER_HPP
#define PUD_LEAF_QUERY_MAKER_HPP

#include <cstdint>
#include <optional>
#include <vector>
#include <immer/map.hpp>
#include "value_objects/pud_body_goal.hpp"
#include "value_objects/pud_body_goal_id.hpp"
#include "value_objects/pud_goal_queries.hpp"
#include "value_objects/pud_mhq_head_id.hpp"
#include "value_objects/pud_node_id.hpp"
#include "value_objects/pud_ubs_state.hpp"

// Gives a new leaf a query for each open goal and records the leaf as holding it. A goal the leaf
// inherits keeps its parent's query unless its structure changed, in which case the query is forked
// into the leaf's environment; a goal the leaf adds gets a query of its own.
template<typename IOpenQuery, typename IForkQuery, typename IHoldQuery, typename IGetNodeGoals, typename IGetNodeLvc>
struct pud_leaf_query_maker {
    pud_leaf_query_maker(
        IOpenQuery& open_query,
        IForkQuery& fork_query,
        IHoldQuery& hold_query,
        IGetNodeGoals& get_node_goals,
        IGetNodeLvc& get_node_lvc);
    pud_goal_queries make_root_queries(pud_node_id root, const pud_ubs_state& bindings);
    pud_goal_queries make_child_queries(
        const immer::map<pud_body_goal_id, pud_mhq_head_id>& parent_queries,
        pud_node_id child,
        const pud_ubs_state& bindings,
        pud_body_goal_id closed_goal,
        const std::vector<pud_body_goal_id>& changed_goals);
private:
    void admit_leaf(
        pud_goal_queries& made,
        pud_node_id leaf,
        const pud_ubs_state& bindings,
        const immer::map<pud_body_goal_id, pud_mhq_head_id>& inherited);

    IOpenQuery& open_query_;
    IForkQuery& fork_query_;
    IHoldQuery& hold_query_;
    IGetNodeGoals& get_node_goals_;
    IGetNodeLvc& get_node_lvc_;
};

#define PUD_LEAF_QUERY_MAKER_TEMPLATE                                                      \
    template<typename IOpenQuery, typename IForkQuery, typename IHoldQuery,                \
             typename IGetNodeGoals, typename IGetNodeLvc>
#define PUD_LEAF_QUERY_MAKER pud_leaf_query_maker<IOpenQuery, IForkQuery, IHoldQuery, IGetNodeGoals, IGetNodeLvc>

PUD_LEAF_QUERY_MAKER_TEMPLATE
PUD_LEAF_QUERY_MAKER::pud_leaf_query_maker(
    IOpenQuery& open_query,
    IForkQuery& fork_query,
    IHoldQuery& hold_query,
    IGetNodeGoals& get_node_goals,
    IGetNodeLvc& get_node_lvc)
    : open_query_(open_query)
    , fork_query_(fork_query)
    , hold_query_(hold_query)
    , get_node_goals_(get_node_goals)
    , get_node_lvc_(get_node_lvc) {}

PUD_LEAF_QUERY_MAKER_TEMPLATE
pud_goal_queries PUD_LEAF_QUERY_MAKER::make_root_queries(pud_node_id root, const pud_ubs_state& bindings) {
    pud_goal_queries made{.heads = {}, .refuted = false};
    admit_leaf(made, root, bindings, {});
    return made;
}

PUD_LEAF_QUERY_MAKER_TEMPLATE
pud_goal_queries PUD_LEAF_QUERY_MAKER::make_child_queries(
    const immer::map<pud_body_goal_id, pud_mhq_head_id>& parent_queries,
    pud_node_id child,
    const pud_ubs_state& bindings,
    pud_body_goal_id closed_goal,
    const std::vector<pud_body_goal_id>& changed_goals) {
    const uint32_t lvc = get_node_lvc_.get(child);
    pud_goal_queries made{.heads = parent_queries.erase(closed_goal), .refuted = false};
    for (pud_body_goal_id goal : changed_goals) {
        const std::optional<pud_mhq_head_id> fork =
            fork_query_.try_fork(made.heads.at(goal), bindings, get_node_goals_.find(child, goal), lvc);
        made.refuted = made.refuted || !fork.has_value();
        made.heads = fork.has_value() ? made.heads.set(goal, fork.value()) : made.heads.erase(goal);
    }
    admit_leaf(made, child, bindings, parent_queries);
    return made;
}

// Opens a query for every open goal the leaf did not inherit, and makes the leaf hold every query.
PUD_LEAF_QUERY_MAKER_TEMPLATE
void PUD_LEAF_QUERY_MAKER::admit_leaf(
    pud_goal_queries& made,
    pud_node_id leaf,
    const pud_ubs_state& bindings,
    const immer::map<pud_body_goal_id, pud_mhq_head_id>& inherited) {
    const uint32_t lvc = get_node_lvc_.get(leaf);
    for (const pud_body_goal& goal : get_node_goals_.get(leaf).goals) {
        if (inherited.find(goal.id) != nullptr)
            continue;
        const std::optional<pud_mhq_head_id> opened = open_query_.try_open(bindings, goal.goal, lvc);
        made.refuted = made.refuted || !opened.has_value();
        if (opened.has_value())
            made.heads = made.heads.set(goal.id, opened.value());
    }
    for (const auto& [goal, head_id] : made.heads)
        hold_query_.hold(head_id, leaf, goal);
}

#undef PUD_LEAF_QUERY_MAKER
#undef PUD_LEAF_QUERY_MAKER_TEMPLATE

#endif

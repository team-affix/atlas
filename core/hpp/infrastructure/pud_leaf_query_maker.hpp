#ifndef PUD_LEAF_QUERY_MAKER_HPP
#define PUD_LEAF_QUERY_MAKER_HPP

#include <cstdint>
#include <optional>
#include <immer/map.hpp>
#include "value_objects/pud_axiom_group_id.hpp"
#include "value_objects/pud_body_goal.hpp"
#include "value_objects/pud_body_goal_id.hpp"
#include "value_objects/pud_goal_queries.hpp"
#include "value_objects/pud_mhq_head_id.hpp"
#include "value_objects/pud_node_id.hpp"
#include "value_objects/pud_ubs_state.hpp"

// Gives a new leaf a query for each open goal, over the axioms its group sees, and records the leaf as
// holding it. A goal the leaf inherits keeps its parent's query unless its structure changed, in which
// case the query is forked into the leaf's environment; a goal the leaf adds gets a query of its own.
// A leaf left holding a query with no candidate, whether it failed to open or fork or was inherited
// already exhausted, is refuted.
template<
    typename IOpenQuery,
    typename IForkQuery,
    typename IHoldQuery,
    typename ICheckQueryExhausted,
    typename IGetNodeGoals,
    typename IGetNodeLvc,
    typename IGetNodeAxiomGroup>
struct pud_leaf_query_maker {
    pud_leaf_query_maker(
        IOpenQuery& open_query,
        IForkQuery& fork_query,
        IHoldQuery& hold_query,
        ICheckQueryExhausted& check_query_exhausted,
        IGetNodeGoals& get_node_goals,
        IGetNodeLvc& get_node_lvc,
        IGetNodeAxiomGroup& get_node_axiom_group);
    void fork_query(pud_goal_queries& made, pud_node_id leaf, const pud_ubs_state& bindings, pud_body_goal_id goal);
    void open_queries(
        pud_goal_queries& made,
        pud_node_id leaf,
        const pud_ubs_state& bindings,
        const immer::map<pud_body_goal_id, pud_mhq_head_id>& inherited);
private:
    IOpenQuery& open_query_;
    IForkQuery& fork_query_;
    IHoldQuery& hold_query_;
    ICheckQueryExhausted& check_query_exhausted_;
    IGetNodeGoals& get_node_goals_;
    IGetNodeLvc& get_node_lvc_;
    IGetNodeAxiomGroup& get_node_axiom_group_;
};

#define PUD_LEAF_QUERY_MAKER_TEMPLATE                                                      \
    template<typename IOpenQuery, typename IForkQuery, typename IHoldQuery,                \
             typename ICheckQueryExhausted, typename IGetNodeGoals, typename IGetNodeLvc,  \
             typename IGetNodeAxiomGroup>
#define PUD_LEAF_QUERY_MAKER                                                               \
    pud_leaf_query_maker<IOpenQuery, IForkQuery, IHoldQuery, ICheckQueryExhausted, IGetNodeGoals, IGetNodeLvc, \
        IGetNodeAxiomGroup>

PUD_LEAF_QUERY_MAKER_TEMPLATE
PUD_LEAF_QUERY_MAKER::pud_leaf_query_maker(
    IOpenQuery& open_query,
    IForkQuery& fork_query,
    IHoldQuery& hold_query,
    ICheckQueryExhausted& check_query_exhausted,
    IGetNodeGoals& get_node_goals,
    IGetNodeLvc& get_node_lvc,
    IGetNodeAxiomGroup& get_node_axiom_group)
    : open_query_(open_query)
    , fork_query_(fork_query)
    , hold_query_(hold_query)
    , check_query_exhausted_(check_query_exhausted)
    , get_node_goals_(get_node_goals)
    , get_node_lvc_(get_node_lvc)
    , get_node_axiom_group_(get_node_axiom_group) {}

// Replaces the leaf's query for a goal whose structure changed with a fork in the leaf's environment.
PUD_LEAF_QUERY_MAKER_TEMPLATE
void PUD_LEAF_QUERY_MAKER::fork_query(
    pud_goal_queries& made,
    pud_node_id leaf,
    const pud_ubs_state& bindings,
    pud_body_goal_id goal) {
    const uint32_t lvc = get_node_lvc_.get(leaf);
    const std::optional<pud_mhq_head_id> fork =
        fork_query_.try_fork(made.heads.at(goal), bindings, get_node_goals_.find(leaf, goal), lvc);
    made.refuted = made.refuted || !fork.has_value();
    made.heads = fork.has_value() ? made.heads.set(goal, fork.value()) : made.heads.erase(goal);
}

// Opens a query for every open goal the leaf did not inherit, and makes the leaf hold every query.
PUD_LEAF_QUERY_MAKER_TEMPLATE
void PUD_LEAF_QUERY_MAKER::open_queries(
    pud_goal_queries& made,
    pud_node_id leaf,
    const pud_ubs_state& bindings,
    const immer::map<pud_body_goal_id, pud_mhq_head_id>& inherited) {
    const uint32_t lvc = get_node_lvc_.get(leaf);
    const pud_axiom_group_id group = get_node_axiom_group_.get(leaf);
    for (const pud_body_goal& goal : get_node_goals_.get(leaf).goals) {
        if (inherited.find(goal.id) != nullptr)
            continue;
        const std::optional<pud_mhq_head_id> opened = open_query_.try_open(group, bindings, goal.goal, lvc);
        made.refuted = made.refuted || !opened.has_value();
        if (opened.has_value())
            made.heads = made.heads.set(goal.id, opened.value());
    }
    for (const auto& [goal, head_id] : made.heads) {
        hold_query_.hold(head_id, leaf, goal);
        made.refuted = made.refuted || check_query_exhausted_.check_exhausted(head_id);
    }
}

#undef PUD_LEAF_QUERY_MAKER
#undef PUD_LEAF_QUERY_MAKER_TEMPLATE

#endif

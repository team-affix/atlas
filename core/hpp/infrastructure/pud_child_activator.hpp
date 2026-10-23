#ifndef PUD_CHILD_ACTIVATOR_HPP
#define PUD_CHILD_ACTIVATOR_HPP

#include <optional>
#include <utility>
#include <immer/map.hpp>
#include "value_objects/pud_active_state.hpp"
#include "value_objects/pud_body_goal.hpp"
#include "value_objects/pud_body_goal_id.hpp"
#include "value_objects/pud_mhq_head_id.hpp"
#include "value_objects/pud_node_id.hpp"
#include "value_objects/pud_structure_watch_change.hpp"
#include "value_objects/pud_ubs_state.hpp"

// Gives a child its active state from its parent's: bindings, watch, and queries, each held by the
// child. The child inherits every query of its parent but the one for the goal it was resolved on,
// forks the ones whose goal changed structure, and opens one for each goal of its own. Returns whether
// some open goal has no candidate, an inherited query already exhausted included, for the caller to
// refute the child.
template<
    typename IGetActiveState,
    typename IStoreActiveState,
    typename IBind,
    typename IWatchDescend,
    typename IGetNodeGoals,
    typename IGetInference,
    typename IForkQuery,
    typename IOpenQuery,
    typename IHoldQuery,
    typename ICheckQueryExhausted>
struct pud_child_activator {
    pud_child_activator(
        IGetActiveState& get_active_state,
        IStoreActiveState& store_active_state,
        IBind& bind,
        IWatchDescend& watch_descend,
        IGetNodeGoals& get_node_goals,
        IGetInference& get_inference,
        IForkQuery& fork_query,
        IOpenQuery& open_query,
        IHoldQuery& hold_query,
        ICheckQueryExhausted& check_query_exhausted);
    bool activate_child(pud_node_id parent, pud_node_id child);
private:
    IGetActiveState& get_active_state_;
    IStoreActiveState& store_active_state_;
    IBind& bind_;
    IWatchDescend& watch_descend_;
    IGetNodeGoals& get_node_goals_;
    IGetInference& get_inference_;
    IForkQuery& fork_query_;
    IOpenQuery& open_query_;
    IHoldQuery& hold_query_;
    ICheckQueryExhausted& check_query_exhausted_;
};

#define PUD_CHILD_ACTIVATOR_TEMPLATE                                                           \
    template<typename IGetActiveState, typename IStoreActiveState, typename IBind,             \
             typename IWatchDescend, typename IGetNodeGoals, typename IGetInference,             \
             typename IForkQuery, typename IOpenQuery, typename IHoldQuery,                     \
             typename ICheckQueryExhausted>
#define PUD_CHILD_ACTIVATOR                                                                    \
    pud_child_activator<IGetActiveState, IStoreActiveState, IBind, IWatchDescend,              \
        IGetNodeGoals, IGetInference, IForkQuery, IOpenQuery, IHoldQuery, ICheckQueryExhausted>

PUD_CHILD_ACTIVATOR_TEMPLATE
PUD_CHILD_ACTIVATOR::pud_child_activator(
    IGetActiveState& get_active_state,
    IStoreActiveState& store_active_state,
    IBind& bind,
    IWatchDescend& watch_descend,
    IGetNodeGoals& get_node_goals,
    IGetInference& get_inference,
    IForkQuery& fork_query,
    IOpenQuery& open_query,
    IHoldQuery& hold_query,
    ICheckQueryExhausted& check_query_exhausted)
    : get_active_state_(get_active_state)
    , store_active_state_(store_active_state)
    , bind_(bind)
    , watch_descend_(watch_descend)
    , get_node_goals_(get_node_goals)
    , get_inference_(get_inference)
    , fork_query_(fork_query)
    , open_query_(open_query)
    , hold_query_(hold_query)
    , check_query_exhausted_(check_query_exhausted) {}

PUD_CHILD_ACTIVATOR_TEMPLATE
bool PUD_CHILD_ACTIVATOR::activate_child(pud_node_id parent, pud_node_id child) {
    const pud_active_state& parent_state = get_active_state_.get(parent);
    const pud_body_goal_id call_site = get_inference_.get_inference(child).call_site;
    pud_ubs_state bindings = bind_.bind(parent_state.bindings, child);
    pud_structure_watch_change watched = watch_descend_.descend(
        parent_state.watch, bindings, child, call_site, get_node_goals_.get(child));
    immer::map<pud_body_goal_id, pud_mhq_head_id> queries = parent_state.queries.erase(call_site);
    bool refuted = false;
    for (pud_body_goal_id goal : watched.changed_goals) {
        const std::optional<pud_mhq_head_id> forked = fork_query_.fork(queries.at(goal), child, bindings, goal);
        refuted = refuted || !forked.has_value();
        queries = forked.has_value() ? queries.set(goal, forked.value()) : queries.erase(goal);
    }
    for (const pud_body_goal& goal : get_node_goals_.get(child).goals) {
        if (parent_state.queries.find(goal.id) != nullptr)
            continue;
        const std::optional<pud_mhq_head_id> opened = open_query_.open(child, bindings, goal.id);
        refuted = refuted || !opened.has_value();
        if (!opened.has_value())
            continue;
        queries = queries.set(goal.id, opened.value());
    }
    for (const auto& [goal, head_id] : queries) {
        hold_query_.hold(head_id, child, goal);
        refuted = refuted || check_query_exhausted_.check_exhausted(head_id);
    }
    store_active_state_.store(child, pud_active_state{
        .bindings = std::move(bindings),
        .watch = std::move(watched.watch),
        .queries = std::move(queries),
    });
    return refuted;
}

#undef PUD_CHILD_ACTIVATOR
#undef PUD_CHILD_ACTIVATOR_TEMPLATE

#endif

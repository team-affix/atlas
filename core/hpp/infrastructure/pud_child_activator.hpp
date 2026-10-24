#ifndef PUD_CHILD_ACTIVATOR_HPP
#define PUD_CHILD_ACTIVATOR_HPP

#include <cstdint>
#include <optional>
#include <utility>
#include <immer/map.hpp>
#include "value_objects/expr.hpp"
#include "value_objects/pud_active_state.hpp"
#include "value_objects/pud_body_goal_id.hpp"
#include "value_objects/pud_global_node_id.hpp"
#include "value_objects/pud_goal_list.hpp"
#include "value_objects/pud_inference.hpp"
#include "value_objects/pud_mhq_head_id.hpp"
#include "value_objects/pud_structure_watch_change.hpp"
#include "value_objects/pud_ubs_state.hpp"

// Sets up everything the runtime keeps for a child, from its parent's while the parent is still
// active. Its goals, kept for good, are its parent's but the call site it was resolved on, under the
// same ids, then its own. It counts as one more live child of its parent. Its active state is its
// lvc, its bindings, its watch, and its queries, each held by the child: it inherits every query of
// its parent but the one for the call site, forks the ones whose goal changed structure, and opens one
// for each goal its parent holds no query for, its own and any the parent found no candidate for.
// Returns whether some goal has no candidate, an inherited query already exhausted included, for the
// caller to refute the child once every sibling is activated.
template<
    typename IGetActiveState,
    typename IStoreActiveState,
    typename IGetInference,
    typename IGetNodeGoals,
    typename IFindNodeGoal,
    typename IStoreNodeGoals,
    typename IIncrementLiveChildCount,
    typename IBind,
    typename IWatchDescend,
    typename IForkQuery,
    typename IOpenQuery,
    typename IHoldQuery,
    typename ICheckQueryExhausted>
struct pud_child_activator {
    pud_child_activator(
        IGetActiveState& get_active_state,
        IStoreActiveState& store_active_state,
        IGetInference& get_inference,
        IGetNodeGoals& get_node_goals,
        IFindNodeGoal& find_node_goal,
        IStoreNodeGoals& store_node_goals,
        IIncrementLiveChildCount& increment_live_child_count,
        IBind& bind,
        IWatchDescend& watch_descend,
        IForkQuery& fork_query,
        IOpenQuery& open_query,
        IHoldQuery& hold_query,
        ICheckQueryExhausted& check_query_exhausted);
    bool activate_child(pud_global_node_id parent, pud_global_node_id child);
private:
    IGetActiveState& get_active_state_;
    IStoreActiveState& store_active_state_;
    IGetInference& get_inference_;
    IGetNodeGoals& get_node_goals_;
    IFindNodeGoal& find_node_goal_;
    IStoreNodeGoals& store_node_goals_;
    IIncrementLiveChildCount& increment_live_child_count_;
    IBind& bind_;
    IWatchDescend& watch_descend_;
    IForkQuery& fork_query_;
    IOpenQuery& open_query_;
    IHoldQuery& hold_query_;
    ICheckQueryExhausted& check_query_exhausted_;
};

#define PUD_CHILD_ACTIVATOR_TEMPLATE                                                           \
    template<typename IGetActiveState, typename IStoreActiveState, typename IGetInference,     \
             typename IGetNodeGoals, typename IFindNodeGoal, typename IStoreNodeGoals,         \
             typename IIncrementLiveChildCount, typename IBind, typename IWatchDescend,        \
             typename IForkQuery, typename IOpenQuery, typename IHoldQuery,                    \
             typename ICheckQueryExhausted>
#define PUD_CHILD_ACTIVATOR                                                                    \
    pud_child_activator<IGetActiveState, IStoreActiveState, IGetInference, IGetNodeGoals,      \
        IFindNodeGoal, IStoreNodeGoals, IIncrementLiveChildCount, IBind, IWatchDescend,        \
        IForkQuery, IOpenQuery, IHoldQuery, ICheckQueryExhausted>

PUD_CHILD_ACTIVATOR_TEMPLATE
PUD_CHILD_ACTIVATOR::pud_child_activator(
    IGetActiveState& get_active_state,
    IStoreActiveState& store_active_state,
    IGetInference& get_inference,
    IGetNodeGoals& get_node_goals,
    IFindNodeGoal& find_node_goal,
    IStoreNodeGoals& store_node_goals,
    IIncrementLiveChildCount& increment_live_child_count,
    IBind& bind,
    IWatchDescend& watch_descend,
    IForkQuery& fork_query,
    IOpenQuery& open_query,
    IHoldQuery& hold_query,
    ICheckQueryExhausted& check_query_exhausted)
    : get_active_state_(get_active_state)
    , store_active_state_(store_active_state)
    , get_inference_(get_inference)
    , get_node_goals_(get_node_goals)
    , find_node_goal_(find_node_goal)
    , store_node_goals_(store_node_goals)
    , increment_live_child_count_(increment_live_child_count)
    , bind_(bind)
    , watch_descend_(watch_descend)
    , fork_query_(fork_query)
    , open_query_(open_query)
    , hold_query_(hold_query)
    , check_query_exhausted_(check_query_exhausted) {}

PUD_CHILD_ACTIVATOR_TEMPLATE
bool PUD_CHILD_ACTIVATOR::activate_child(pud_global_node_id parent, pud_global_node_id child) {
    const pud_active_state& parent_state = get_active_state_.get(parent);
    const pud_inference& inference = get_inference_.get_inference(child);
    pud_goal_list goals = get_node_goals_.get(parent).closing(inference.call_site);
    for (const expr* goal : inference.body_goals)
        goals = goals.adding(goal);
    store_node_goals_.store(child, goals);
    increment_live_child_count_.increment(parent);
    const uint32_t lvc = parent_state.lvc + inference.var_count;
    pud_ubs_state bindings = bind_.bind(parent_state.bindings, child);
    pud_structure_watch_change watched = watch_descend_.descend(parent_state.watch, bindings, child, inference.call_site, goals);
    immer::map<pud_body_goal_id, pud_mhq_head_id> queries = parent_state.queries.erase(inference.call_site);
    bool refuted = false;
    for (pud_body_goal_id goal : watched.changed_goals) {
        if (queries.find(goal) == nullptr)
            continue;
        const std::optional<pud_mhq_head_id> forked =
            fork_query_.try_fork(queries.at(goal), bindings, find_node_goal_.find(child, goal), lvc);
        refuted = refuted || !forked.has_value();
        queries = forked.has_value() ? queries.set(goal, forked.value()) : queries.erase(goal);
    }
    for (const auto& [id, goal] : goals.goals) {
        if (parent_state.queries.find(id) != nullptr)
            continue;
        const std::optional<pud_mhq_head_id> opened = open_query_.try_open(child.module, bindings, goal, lvc);
        refuted = refuted || !opened.has_value();
        if (!opened.has_value())
            continue;
        queries = queries.set(id, opened.value());
    }
    for (const auto& [goal, head_id] : queries) {
        hold_query_.hold(head_id, child, goal);
        refuted = refuted || check_query_exhausted_.check_exhausted(head_id);
    }
    store_active_state_.store(child, pud_active_state{
        .lvc = lvc,
        .bindings = std::move(bindings),
        .watch = std::move(watched.watch),
        .queries = std::move(queries),
    });
    return refuted;
}

#undef PUD_CHILD_ACTIVATOR
#undef PUD_CHILD_ACTIVATOR_TEMPLATE

#endif

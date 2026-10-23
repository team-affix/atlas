#ifndef PUD_CHILD_ACTIVATOR_HPP
#define PUD_CHILD_ACTIVATOR_HPP

#include <utility>
#include "value_objects/pud_active_state.hpp"
#include "value_objects/pud_body_goal_id.hpp"
#include "value_objects/pud_goal_queries.hpp"
#include "value_objects/pud_node_id.hpp"
#include "value_objects/pud_structure_watch_change.hpp"
#include "value_objects/pud_ubs_state.hpp"

// Gives a child its active state from its parent's: bindings, watch, and queries. The child inherits
// every query of its parent but the one for the goal it was resolved on. Returns whether some open goal
// has no candidate, for the caller to refute the child.
template<
    typename IGetActiveState,
    typename IStoreActiveState,
    typename IBind,
    typename IWatchDescend,
    typename IGetNodeGoals,
    typename IGetCallSite,
    typename IForkQuery,
    typename IOpenQueries>
struct pud_child_activator {
    pud_child_activator(
        IGetActiveState& get_active_state,
        IStoreActiveState& store_active_state,
        IBind& bind,
        IWatchDescend& watch_descend,
        IGetNodeGoals& get_node_goals,
        IGetCallSite& get_call_site,
        IForkQuery& fork_query,
        IOpenQueries& open_queries);
    bool activate_child(pud_node_id parent, pud_node_id child);
private:
    IGetActiveState& get_active_state_;
    IStoreActiveState& store_active_state_;
    IBind& bind_;
    IWatchDescend& watch_descend_;
    IGetNodeGoals& get_node_goals_;
    IGetCallSite& get_call_site_;
    IForkQuery& fork_query_;
    IOpenQueries& open_queries_;
};

#define PUD_CHILD_ACTIVATOR_TEMPLATE                                                           \
    template<typename IGetActiveState, typename IStoreActiveState, typename IBind,             \
             typename IWatchDescend, typename IGetNodeGoals, typename IGetCallSite,             \
             typename IForkQuery, typename IOpenQueries>
#define PUD_CHILD_ACTIVATOR                                                                    \
    pud_child_activator<IGetActiveState, IStoreActiveState, IBind, IWatchDescend,              \
        IGetNodeGoals, IGetCallSite, IForkQuery, IOpenQueries>

PUD_CHILD_ACTIVATOR_TEMPLATE
PUD_CHILD_ACTIVATOR::pud_child_activator(
    IGetActiveState& get_active_state,
    IStoreActiveState& store_active_state,
    IBind& bind,
    IWatchDescend& watch_descend,
    IGetNodeGoals& get_node_goals,
    IGetCallSite& get_call_site,
    IForkQuery& fork_query,
    IOpenQueries& open_queries)
    : get_active_state_(get_active_state)
    , store_active_state_(store_active_state)
    , bind_(bind)
    , watch_descend_(watch_descend)
    , get_node_goals_(get_node_goals)
    , get_call_site_(get_call_site)
    , fork_query_(fork_query)
    , open_queries_(open_queries) {}

PUD_CHILD_ACTIVATOR_TEMPLATE
bool PUD_CHILD_ACTIVATOR::activate_child(pud_node_id parent, pud_node_id child) {
    const pud_active_state& parent_state = get_active_state_.get(parent);
    const pud_body_goal_id call_site = get_call_site_.get(parent);
    pud_ubs_state bindings = bind_.bind(parent_state.bindings, child);
    pud_structure_watch_change watched = watch_descend_.descend(
        parent_state.watch, bindings, child, call_site, get_node_goals_.get(child));
    pud_goal_queries queries{.heads = parent_state.queries.erase(call_site), .refuted = false};
    for (pud_body_goal_id goal : watched.changed_goals)
        fork_query_.fork_query(queries, child, bindings, goal);
    open_queries_.open_queries(queries, child, bindings, parent_state.queries);
    store_active_state_.store(child, pud_active_state{
        .bindings = std::move(bindings),
        .watch = std::move(watched.watch),
        .queries = std::move(queries.heads),
    });
    return queries.refuted;
}

#undef PUD_CHILD_ACTIVATOR
#undef PUD_CHILD_ACTIVATOR_TEMPLATE

#endif

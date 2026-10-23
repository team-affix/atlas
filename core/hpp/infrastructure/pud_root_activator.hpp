#ifndef PUD_ROOT_ACTIVATOR_HPP
#define PUD_ROOT_ACTIVATOR_HPP

#include <utility>
#include "value_objects/pud_active_state.hpp"
#include "value_objects/pud_goal_queries.hpp"
#include "value_objects/pud_node_id.hpp"
#include "value_objects/pud_ubs_state.hpp"

// Gives a root its active state: empty bindings, a structure watch, and a query per open goal.
// Returns whether some open goal has no candidate, for the caller to refute the root.
template<typename IStoreActiveState, typename IWatchRoot, typename IGetNodeGoals, typename IOpenQueries>
struct pud_root_activator {
    pud_root_activator(
        IStoreActiveState& store_active_state,
        IWatchRoot& watch_root,
        IGetNodeGoals& get_node_goals,
        IOpenQueries& open_queries);
    bool activate_root(pud_node_id root);
private:
    IStoreActiveState& store_active_state_;
    IWatchRoot& watch_root_;
    IGetNodeGoals& get_node_goals_;
    IOpenQueries& open_queries_;
};

#define PUD_ROOT_ACTIVATOR_TEMPLATE                                                            \
    template<typename IStoreActiveState, typename IWatchRoot, typename IGetNodeGoals, typename IOpenQueries>
#define PUD_ROOT_ACTIVATOR pud_root_activator<IStoreActiveState, IWatchRoot, IGetNodeGoals, IOpenQueries>

PUD_ROOT_ACTIVATOR_TEMPLATE
PUD_ROOT_ACTIVATOR::pud_root_activator(
    IStoreActiveState& store_active_state,
    IWatchRoot& watch_root,
    IGetNodeGoals& get_node_goals,
    IOpenQueries& open_queries)
    : store_active_state_(store_active_state)
    , watch_root_(watch_root)
    , get_node_goals_(get_node_goals)
    , open_queries_(open_queries) {}

PUD_ROOT_ACTIVATOR_TEMPLATE
bool PUD_ROOT_ACTIVATOR::activate_root(pud_node_id root) {
    const pud_ubs_state bindings{};
    pud_goal_queries queries{.heads = {}, .refuted = false};
    open_queries_.open_queries(queries, root, bindings, {});
    store_active_state_.store(root, pud_active_state{
        .bindings = bindings,
        .watch = watch_root_.watch_root(bindings, get_node_goals_.get(root)),
        .queries = std::move(queries.heads),
    });
    return queries.refuted;
}

#undef PUD_ROOT_ACTIVATOR
#undef PUD_ROOT_ACTIVATOR_TEMPLATE

#endif

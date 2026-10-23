#ifndef PUD_ROOT_ACTIVATOR_HPP
#define PUD_ROOT_ACTIVATOR_HPP

#include <utility>
#include "value_objects/pud_active_state.hpp"
#include "value_objects/pud_goal_queries.hpp"
#include "value_objects/pud_node_id.hpp"
#include "value_objects/pud_ubs_state.hpp"

// Gives a new root its active state: empty bindings, a structure watch, and a query per open goal.
// Returns whether some open goal has no candidate (the caller refutes once that is safe).
template<typename IStoreActiveState, typename IWatchRoot, typename IGetNodeGoals, typename IMakeRootQueries>
struct pud_root_activator {
    pud_root_activator(
        IStoreActiveState& store_active_state,
        IWatchRoot& watch_root,
        IGetNodeGoals& get_node_goals,
        IMakeRootQueries& make_root_queries);
    bool activate_root(pud_node_id root);
private:
    IStoreActiveState& store_active_state_;
    IWatchRoot& watch_root_;
    IGetNodeGoals& get_node_goals_;
    IMakeRootQueries& make_root_queries_;
};

#define PUD_ROOT_ACTIVATOR_TEMPLATE                                                            \
    template<typename IStoreActiveState, typename IWatchRoot, typename IGetNodeGoals,          \
             typename IMakeRootQueries>
#define PUD_ROOT_ACTIVATOR                                                                       \
    pud_root_activator<IStoreActiveState, IWatchRoot, IGetNodeGoals, IMakeRootQueries>

PUD_ROOT_ACTIVATOR_TEMPLATE
PUD_ROOT_ACTIVATOR::pud_root_activator(
    IStoreActiveState& store_active_state,
    IWatchRoot& watch_root,
    IGetNodeGoals& get_node_goals,
    IMakeRootQueries& make_root_queries)
    : store_active_state_(store_active_state)
    , watch_root_(watch_root)
    , get_node_goals_(get_node_goals)
    , make_root_queries_(make_root_queries) {}

PUD_ROOT_ACTIVATOR_TEMPLATE
bool PUD_ROOT_ACTIVATOR::activate_root(pud_node_id root) {
    const pud_ubs_state bindings{};
    pud_goal_queries queries = make_root_queries_.make_root_queries(root, bindings);
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

#ifndef PUD_LEAF_ACTIVATOR_HPP
#define PUD_LEAF_ACTIVATOR_HPP

#include <utility>
#include "value_objects/pud_body_goal_id.hpp"
#include "value_objects/pud_goal_queries.hpp"
#include "value_objects/pud_active_state.hpp"
#include "value_objects/pud_node_id.hpp"
#include "value_objects/pud_structure_watch_change.hpp"
#include "value_objects/pud_ubs_state.hpp"

// Gives a new leaf its state: a root starts from empty bindings; a child continues its parent's. Each
// activation reports whether some open goal of the leaf has no candidate, which refutes the leaf; the
// caller refutes it once that is safe, since the leaf already holds its other queries.
template<
    typename IGetActiveState,
    typename IStoreActiveState,
    typename IBind,
    typename IWatch,
    typename IGetNodeGoals,
    typename IGetCallSite,
    typename IMakeQueries>
struct pud_leaf_activator {
    pud_leaf_activator(
        IGetActiveState& get_active_state,
        IStoreActiveState& store_active_state,
        IBind& bind,
        IWatch& watch,
        IGetNodeGoals& get_node_goals,
        IGetCallSite& get_call_site,
        IMakeQueries& make_queries);
    bool activate_root(pud_node_id root);
    bool activate_child(pud_node_id parent, pud_node_id child);
private:
    IGetActiveState& get_active_state_;
    IStoreActiveState& store_active_state_;
    IBind& bind_;
    IWatch& watch_;
    IGetNodeGoals& get_node_goals_;
    IGetCallSite& get_call_site_;
    IMakeQueries& make_queries_;
};

#define PUD_LEAF_ACTIVATOR_TEMPLATE                                                        \
    template<typename IGetActiveState, typename IStoreActiveState, typename IBind, typename IWatch, \
             typename IGetNodeGoals, typename IGetCallSite, typename IMakeQueries>
#define PUD_LEAF_ACTIVATOR                                                                 \
    pud_leaf_activator<IGetActiveState, IStoreActiveState, IBind, IWatch, IGetNodeGoals,   \
        IGetCallSite, IMakeQueries>

PUD_LEAF_ACTIVATOR_TEMPLATE
PUD_LEAF_ACTIVATOR::pud_leaf_activator(
    IGetActiveState& get_active_state,
    IStoreActiveState& store_active_state,
    IBind& bind,
    IWatch& watch,
    IGetNodeGoals& get_node_goals,
    IGetCallSite& get_call_site,
    IMakeQueries& make_queries)
    : get_active_state_(get_active_state)
    , store_active_state_(store_active_state)
    , bind_(bind)
    , watch_(watch)
    , get_node_goals_(get_node_goals)
    , get_call_site_(get_call_site)
    , make_queries_(make_queries) {}

PUD_LEAF_ACTIVATOR_TEMPLATE
bool PUD_LEAF_ACTIVATOR::activate_root(pud_node_id root) {
    const pud_ubs_state bindings{};
    pud_goal_queries queries = make_queries_.make_root_queries(root, bindings);
    store_active_state_.store(root, pud_active_state{
        .bindings = bindings,
        .watch = watch_.watch_root(bindings, get_node_goals_.get(root)),
        .queries = std::move(queries.heads),
    });
    return queries.refuted;
}

PUD_LEAF_ACTIVATOR_TEMPLATE
bool PUD_LEAF_ACTIVATOR::activate_child(pud_node_id parent, pud_node_id child) {
    const pud_active_state& parent_state = get_active_state_.get(parent);
    const pud_body_goal_id call_site = get_call_site_.get(parent);
    pud_ubs_state bindings = bind_.bind(parent_state.bindings, child);
    pud_structure_watch_change watched =
        watch_.descend(parent_state.watch, bindings, child, call_site, get_node_goals_.get(child));
    pud_goal_queries queries =
        make_queries_.make_child_queries(parent_state.queries, child, bindings, call_site, watched.changed_goals);
    store_active_state_.store(child, pud_active_state{
        .bindings = std::move(bindings),
        .watch = std::move(watched.watch),
        .queries = std::move(queries.heads),
    });
    return queries.refuted;
}

#undef PUD_LEAF_ACTIVATOR
#undef PUD_LEAF_ACTIVATOR_TEMPLATE

#endif

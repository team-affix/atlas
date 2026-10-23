#ifndef PUD_ROOT_ACTIVATOR_HPP
#define PUD_ROOT_ACTIVATOR_HPP

#include <optional>
#include <utility>
#include <immer/map.hpp>
#include "value_objects/pud_active_state.hpp"
#include "value_objects/pud_body_goal.hpp"
#include "value_objects/pud_body_goal_id.hpp"
#include "value_objects/pud_mhq_head_id.hpp"
#include "value_objects/pud_global_node_id.hpp"
#include "value_objects/pud_ubs_state.hpp"

// Gives a root its active state: empty bindings, a structure watch, and a query per open goal, each
// held by the root. Returns whether some open goal has no candidate, for the caller to refute the root.
template<typename IStoreActiveState, typename IWatchRoot, typename IGetNodeGoals, typename IOpenQuery, typename IHoldQuery>
struct pud_root_activator {
    pud_root_activator(
        IStoreActiveState& store_active_state,
        IWatchRoot& watch_root,
        IGetNodeGoals& get_node_goals,
        IOpenQuery& open_query,
        IHoldQuery& hold_query);
    bool activate_root(pud_global_node_id root);
private:
    IStoreActiveState& store_active_state_;
    IWatchRoot& watch_root_;
    IGetNodeGoals& get_node_goals_;
    IOpenQuery& open_query_;
    IHoldQuery& hold_query_;
};

#define PUD_ROOT_ACTIVATOR_TEMPLATE                                                            \
    template<typename IStoreActiveState, typename IWatchRoot, typename IGetNodeGoals,          \
             typename IOpenQuery, typename IHoldQuery>
#define PUD_ROOT_ACTIVATOR pud_root_activator<IStoreActiveState, IWatchRoot, IGetNodeGoals, IOpenQuery, IHoldQuery>

PUD_ROOT_ACTIVATOR_TEMPLATE
PUD_ROOT_ACTIVATOR::pud_root_activator(
    IStoreActiveState& store_active_state,
    IWatchRoot& watch_root,
    IGetNodeGoals& get_node_goals,
    IOpenQuery& open_query,
    IHoldQuery& hold_query)
    : store_active_state_(store_active_state)
    , watch_root_(watch_root)
    , get_node_goals_(get_node_goals)
    , open_query_(open_query)
    , hold_query_(hold_query) {}

PUD_ROOT_ACTIVATOR_TEMPLATE
bool PUD_ROOT_ACTIVATOR::activate_root(pud_global_node_id root) {
    const pud_ubs_state bindings{};
    immer::map<pud_body_goal_id, pud_mhq_head_id> queries;
    bool refuted = false;
    for (const pud_body_goal& goal : get_node_goals_.get(root).goals) {
        const std::optional<pud_mhq_head_id> opened = open_query_.open(root, bindings, goal.id);
        refuted = refuted || !opened.has_value();
        if (!opened.has_value())
            continue;
        queries = queries.set(goal.id, opened.value());
    }
    for (const auto& [goal, head_id] : queries)
        hold_query_.hold(head_id, root, goal);
    store_active_state_.store(root, pud_active_state{
        .bindings = bindings,
        .watch = watch_root_.watch_root(bindings, get_node_goals_.get(root)),
        .queries = std::move(queries),
    });
    return refuted;
}

#undef PUD_ROOT_ACTIVATOR
#undef PUD_ROOT_ACTIVATOR_TEMPLATE

#endif

#ifndef PUD_ROOT_ACTIVATOR_HPP
#define PUD_ROOT_ACTIVATOR_HPP

#include <optional>
#include <utility>
#include <immer/map.hpp>
#include "value_objects/expr.hpp"
#include "value_objects/pud_active_state.hpp"
#include "value_objects/pud_body_goal_id.hpp"
#include "value_objects/pud_global_node_id.hpp"
#include "value_objects/pud_goal_list.hpp"
#include "value_objects/pud_mhq_head_id.hpp"
#include "value_objects/pud_ubs_state.hpp"
#include "value_objects/rule.hpp"

// Sets up everything the controller keeps for an axiom. Its goals are its body, and are kept for good.
// Its active state is its lvc, empty bindings, a structure watch, and a query per goal over the axioms
// its module sees, each held by the axiom. Returns whether some goal has no candidate, for the caller
// to refute the axiom.
template<
    typename IGetAxiom,
    typename IStoreNodeGoals,
    typename IStoreActiveState,
    typename IWatchRoot,
    typename IOpenQuery,
    typename IHoldQuery>
struct pud_root_activator {
    pud_root_activator(
        IGetAxiom& get_axiom,
        IStoreNodeGoals& store_node_goals,
        IStoreActiveState& store_active_state,
        IWatchRoot& watch_root,
        IOpenQuery& open_query,
        IHoldQuery& hold_query);
    bool activate_root(pud_global_node_id root);
private:
    IGetAxiom& get_axiom_;
    IStoreNodeGoals& store_node_goals_;
    IStoreActiveState& store_active_state_;
    IWatchRoot& watch_root_;
    IOpenQuery& open_query_;
    IHoldQuery& hold_query_;
};

#define PUD_ROOT_ACTIVATOR_TEMPLATE                                                            \
    template<typename IGetAxiom, typename IStoreNodeGoals, typename IStoreActiveState,         \
             typename IWatchRoot, typename IOpenQuery, typename IHoldQuery>
#define PUD_ROOT_ACTIVATOR \
    pud_root_activator<IGetAxiom, IStoreNodeGoals, IStoreActiveState, IWatchRoot, IOpenQuery, IHoldQuery>

PUD_ROOT_ACTIVATOR_TEMPLATE
PUD_ROOT_ACTIVATOR::pud_root_activator(
    IGetAxiom& get_axiom,
    IStoreNodeGoals& store_node_goals,
    IStoreActiveState& store_active_state,
    IWatchRoot& watch_root,
    IOpenQuery& open_query,
    IHoldQuery& hold_query)
    : get_axiom_(get_axiom)
    , store_node_goals_(store_node_goals)
    , store_active_state_(store_active_state)
    , watch_root_(watch_root)
    , open_query_(open_query)
    , hold_query_(hold_query) {}

PUD_ROOT_ACTIVATOR_TEMPLATE
bool PUD_ROOT_ACTIVATOR::activate_root(pud_global_node_id root) {
    const rule& axiom = get_axiom_.get_axiom(root);
    pud_goal_list goals{.goals = {}, .next_id = 0};
    for (const expr* goal : axiom.body)
        goals = goals.adding(goal);
    store_node_goals_.store(root, goals);
    const pud_ubs_state bindings{};
    immer::map<pud_body_goal_id, pud_mhq_head_id> queries;
    bool refuted = false;
    for (const auto& [id, goal] : goals.goals) {
        const std::optional<pud_mhq_head_id> opened = open_query_.try_open(root.module, bindings, goal, axiom.var_count);
        refuted = refuted || !opened.has_value();
        if (!opened.has_value())
            continue;
        queries = queries.set(id, opened.value());
    }
    for (const auto& [goal, head_id] : queries)
        hold_query_.hold(head_id, root, goal);
    store_active_state_.store(root, pud_active_state{
        .lvc = axiom.var_count,
        .bindings = bindings,
        .watch = watch_root_.watch_root(bindings, goals),
        .queries = std::move(queries),
    });
    return refuted;
}

#undef PUD_ROOT_ACTIVATOR
#undef PUD_ROOT_ACTIVATOR_TEMPLATE

#endif

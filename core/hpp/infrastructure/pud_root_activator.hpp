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

template<
    typename IGetAxiom,
    typename IOpenGoals,
    typename IStoreNodeGoals,
    typename IStoreNodeAxiom,
    typename IStoreActiveState,
    typename IWatchRoot,
    typename IOpenQuery,
    typename IHoldQuery>
struct pud_root_activator {
    pud_root_activator(
        IGetAxiom& get_axiom,
        IOpenGoals& open_goals,
        IStoreNodeGoals& store_node_goals,
        IStoreNodeAxiom& store_node_axiom,
        IStoreActiveState& store_active_state,
        IWatchRoot& watch_root,
        IOpenQuery& open_query,
        IHoldQuery& hold_query);
    bool activate_root(pud_global_node_id root);
private:
    IGetAxiom& get_axiom_;
    IOpenGoals& open_goals_;
    IStoreNodeGoals& store_node_goals_;
    IStoreNodeAxiom& store_node_axiom_;
    IStoreActiveState& store_active_state_;
    IWatchRoot& watch_root_;
    IOpenQuery& open_query_;
    IHoldQuery& hold_query_;
};

#define PUD_ROOT_ACTIVATOR_TEMPLATE                                                            \
    template<typename IGetAxiom, typename IOpenGoals, typename IStoreNodeGoals,                \
             typename IStoreNodeAxiom, typename IStoreActiveState, typename IWatchRoot,         \
             typename IOpenQuery, typename IHoldQuery>
#define PUD_ROOT_ACTIVATOR                                                                       \
    pud_root_activator<IGetAxiom, IOpenGoals, IStoreNodeGoals, IStoreNodeAxiom,                \
        IStoreActiveState, IWatchRoot, IOpenQuery, IHoldQuery>

PUD_ROOT_ACTIVATOR_TEMPLATE
PUD_ROOT_ACTIVATOR::pud_root_activator(
    IGetAxiom& get_axiom,
    IOpenGoals& open_goals,
    IStoreNodeGoals& store_node_goals,
    IStoreNodeAxiom& store_node_axiom,
    IStoreActiveState& store_active_state,
    IWatchRoot& watch_root,
    IOpenQuery& open_query,
    IHoldQuery& hold_query)
    : get_axiom_(get_axiom)
    , open_goals_(open_goals)
    , store_node_goals_(store_node_goals)
    , store_node_axiom_(store_node_axiom)
    , store_active_state_(store_active_state)
    , watch_root_(watch_root)
    , open_query_(open_query)
    , hold_query_(hold_query) {}

PUD_ROOT_ACTIVATOR_TEMPLATE
bool PUD_ROOT_ACTIVATOR::activate_root(pud_global_node_id root) {
    const rule& axiom = get_axiom_.get_axiom(root);
    const pud_goal_list goals = open_goals_.open_goals(axiom.body);
    store_node_goals_.store(root, goals);
    store_node_axiom_.store(root, root);
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
        .watch = watch_root_.watch_root(bindings, goals, root.module),
        .queries = std::move(queries),
    });
    return refuted;
}

#undef PUD_ROOT_ACTIVATOR
#undef PUD_ROOT_ACTIVATOR_TEMPLATE

#endif

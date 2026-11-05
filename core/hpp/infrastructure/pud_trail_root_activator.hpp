#ifndef PUD_TRAIL_ROOT_ACTIVATOR_HPP
#define PUD_TRAIL_ROOT_ACTIVATOR_HPP

#include "value_objects/pud_global_node_id.hpp"
#include "value_objects/pud_goal_list.hpp"
#include "value_objects/pud_node_trails.hpp"
#include "value_objects/pud_ubs_state.hpp"
#include "value_objects/rule.hpp"

// Activates an axiom root with what every live leaf has, plus its structure watch and no trails yet: its goals are
// queried fresh when it is visited. A root is never refuted on arrival.
template<
    typename IGetAxiom,
    typename IBuildRootGoalList,
    typename IStoreNodeGoals,
    typename IStoreNodeAxiom,
    typename IStoreNodeLvc,
    typename IStoreNodeBindings,
    typename IStoreNodeWatch,
    typename IStoreNodeTrails,
    typename IWatchRoot>
struct pud_trail_root_activator {
    pud_trail_root_activator(
        IGetAxiom& get_axiom,
        IBuildRootGoalList& build_root_goal_list,
        IStoreNodeGoals& store_node_goals,
        IStoreNodeAxiom& store_node_axiom,
        IStoreNodeLvc& store_node_lvc,
        IStoreNodeBindings& store_node_bindings,
        IStoreNodeWatch& store_node_watch,
        IStoreNodeTrails& store_node_trails,
        IWatchRoot& watch_root);
    bool activate_root(pud_global_node_id root);
private:
    IGetAxiom& get_axiom_;
    IBuildRootGoalList& build_root_goal_list_;
    IStoreNodeGoals& store_node_goals_;
    IStoreNodeAxiom& store_node_axiom_;
    IStoreNodeLvc& store_node_lvc_;
    IStoreNodeBindings& store_node_bindings_;
    IStoreNodeWatch& store_node_watch_;
    IStoreNodeTrails& store_node_trails_;
    IWatchRoot& watch_root_;
};

#define PUD_TRAIL_ROOT_ACTIVATOR_TEMPLATE                                                                \
    template<typename IGetAxiom, typename IBuildRootGoalList, typename IStoreNodeGoals,                  \
             typename IStoreNodeAxiom, typename IStoreNodeLvc, typename IStoreNodeBindings,              \
             typename IStoreNodeWatch, typename IStoreNodeTrails, typename IWatchRoot>
#define PUD_TRAIL_ROOT_ACTIVATOR                                                                         \
    pud_trail_root_activator<IGetAxiom, IBuildRootGoalList, IStoreNodeGoals, IStoreNodeAxiom,            \
        IStoreNodeLvc, IStoreNodeBindings, IStoreNodeWatch, IStoreNodeTrails, IWatchRoot>

PUD_TRAIL_ROOT_ACTIVATOR_TEMPLATE
PUD_TRAIL_ROOT_ACTIVATOR::pud_trail_root_activator(
    IGetAxiom& get_axiom,
    IBuildRootGoalList& build_root_goal_list,
    IStoreNodeGoals& store_node_goals,
    IStoreNodeAxiom& store_node_axiom,
    IStoreNodeLvc& store_node_lvc,
    IStoreNodeBindings& store_node_bindings,
    IStoreNodeWatch& store_node_watch,
    IStoreNodeTrails& store_node_trails,
    IWatchRoot& watch_root)
    : get_axiom_(get_axiom)
    , build_root_goal_list_(build_root_goal_list)
    , store_node_goals_(store_node_goals)
    , store_node_axiom_(store_node_axiom)
    , store_node_lvc_(store_node_lvc)
    , store_node_bindings_(store_node_bindings)
    , store_node_watch_(store_node_watch)
    , store_node_trails_(store_node_trails)
    , watch_root_(watch_root) {}

PUD_TRAIL_ROOT_ACTIVATOR_TEMPLATE
bool PUD_TRAIL_ROOT_ACTIVATOR::activate_root(pud_global_node_id root) {
    const rule& axiom = get_axiom_.get_axiom(root);
    const pud_goal_list goals = build_root_goal_list_.build(axiom.body);
    const pud_ubs_state bindings{};
    store_node_goals_.store(root, goals);
    store_node_axiom_.store(root, root);
    store_node_lvc_.store(root, axiom.var_count);
    store_node_bindings_.store(root, bindings);
    store_node_watch_.store(root, watch_root_.watch_root(bindings, goals, root.module));
    store_node_trails_.store(root, pud_node_trails{});
    return false;
}

#undef PUD_TRAIL_ROOT_ACTIVATOR
#undef PUD_TRAIL_ROOT_ACTIVATOR_TEMPLATE

#endif

#ifndef PUD_PLAIN_ROOT_ACTIVATOR_HPP
#define PUD_PLAIN_ROOT_ACTIVATOR_HPP

#include "value_objects/pud_global_node_id.hpp"
#include "value_objects/pud_goal_list.hpp"
#include "value_objects/pud_ubs_state.hpp"
#include "value_objects/rule.hpp"

// Activates an axiom root with only what every live leaf has: its goals, its axiom, its lvc and empty bindings.
// It opens no queries, so a root is never refuted on arrival: its goals are queried when it is visited.
template<
    typename IGetAxiom,
    typename IBuildRootGoalList,
    typename IStoreNodeGoals,
    typename IStoreNodeAxiom,
    typename IStoreNodeLvc,
    typename IStoreNodeBindings>
struct pud_plain_root_activator {
    pud_plain_root_activator(
        IGetAxiom& get_axiom,
        IBuildRootGoalList& build_root_goal_list,
        IStoreNodeGoals& store_node_goals,
        IStoreNodeAxiom& store_node_axiom,
        IStoreNodeLvc& store_node_lvc,
        IStoreNodeBindings& store_node_bindings);
    bool activate_root(pud_global_node_id root);
private:
    IGetAxiom& get_axiom_;
    IBuildRootGoalList& build_root_goal_list_;
    IStoreNodeGoals& store_node_goals_;
    IStoreNodeAxiom& store_node_axiom_;
    IStoreNodeLvc& store_node_lvc_;
    IStoreNodeBindings& store_node_bindings_;
};

#define PUD_PLAIN_ROOT_ACTIVATOR_TEMPLATE                                                                \
    template<typename IGetAxiom, typename IBuildRootGoalList, typename IStoreNodeGoals,                  \
             typename IStoreNodeAxiom, typename IStoreNodeLvc, typename IStoreNodeBindings>
#define PUD_PLAIN_ROOT_ACTIVATOR                                                                         \
    pud_plain_root_activator<IGetAxiom, IBuildRootGoalList, IStoreNodeGoals, IStoreNodeAxiom,            \
        IStoreNodeLvc, IStoreNodeBindings>

PUD_PLAIN_ROOT_ACTIVATOR_TEMPLATE
PUD_PLAIN_ROOT_ACTIVATOR::pud_plain_root_activator(
    IGetAxiom& get_axiom,
    IBuildRootGoalList& build_root_goal_list,
    IStoreNodeGoals& store_node_goals,
    IStoreNodeAxiom& store_node_axiom,
    IStoreNodeLvc& store_node_lvc,
    IStoreNodeBindings& store_node_bindings)
    : get_axiom_(get_axiom)
    , build_root_goal_list_(build_root_goal_list)
    , store_node_goals_(store_node_goals)
    , store_node_axiom_(store_node_axiom)
    , store_node_lvc_(store_node_lvc)
    , store_node_bindings_(store_node_bindings) {}

PUD_PLAIN_ROOT_ACTIVATOR_TEMPLATE
bool PUD_PLAIN_ROOT_ACTIVATOR::activate_root(pud_global_node_id root) {
    const rule& axiom = get_axiom_.get_axiom(root);
    store_node_goals_.store(root, build_root_goal_list_.build(axiom.body));
    store_node_axiom_.store(root, root);
    store_node_lvc_.store(root, axiom.var_count);
    store_node_bindings_.store(root, pud_ubs_state{});
    return false;
}

#undef PUD_PLAIN_ROOT_ACTIVATOR
#undef PUD_PLAIN_ROOT_ACTIVATOR_TEMPLATE

#endif

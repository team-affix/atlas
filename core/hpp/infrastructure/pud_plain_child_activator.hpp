#ifndef PUD_PLAIN_CHILD_ACTIVATOR_HPP
#define PUD_PLAIN_CHILD_ACTIVATOR_HPP

#include <cstdint>
#include "value_objects/pud_global_node_id.hpp"
#include "value_objects/pud_goal_list.hpp"
#include "value_objects/pud_inference.hpp"
#include "value_objects/pud_ubs_state.hpp"

// Activates a child with only what every live leaf has: its goals, its axiom, its lvc and its bindings, bound from
// its parent's. It opens no queries, so a child is never refuted on arrival: its goals are queried when it is
// visited.
template<
    typename IGetNodeLvc,
    typename IGetNodeBindings,
    typename IStoreNodeLvc,
    typename IStoreNodeBindings,
    typename IGetInference,
    typename IGetNodeGoals,
    typename IGetNodeAxiom,
    typename IBuildChildGoalList,
    typename IStoreNodeGoals,
    typename IStoreNodeAxiom,
    typename IIncrementLiveChildCount,
    typename IBind>
struct pud_plain_child_activator {
    pud_plain_child_activator(
        IGetNodeLvc& get_node_lvc,
        IGetNodeBindings& get_node_bindings,
        IStoreNodeLvc& store_node_lvc,
        IStoreNodeBindings& store_node_bindings,
        IGetInference& get_inference,
        IGetNodeGoals& get_node_goals,
        IGetNodeAxiom& get_node_axiom,
        IBuildChildGoalList& build_child_goal_list,
        IStoreNodeGoals& store_node_goals,
        IStoreNodeAxiom& store_node_axiom,
        IIncrementLiveChildCount& increment_live_child_count,
        IBind& bind);
    bool activate_child(pud_global_node_id parent, pud_global_node_id child);
private:
    IGetNodeLvc& get_node_lvc_;
    IGetNodeBindings& get_node_bindings_;
    IStoreNodeLvc& store_node_lvc_;
    IStoreNodeBindings& store_node_bindings_;
    IGetInference& get_inference_;
    IGetNodeGoals& get_node_goals_;
    IGetNodeAxiom& get_node_axiom_;
    IBuildChildGoalList& build_child_goal_list_;
    IStoreNodeGoals& store_node_goals_;
    IStoreNodeAxiom& store_node_axiom_;
    IIncrementLiveChildCount& increment_live_child_count_;
    IBind& bind_;
};

#define PUD_PLAIN_CHILD_ACTIVATOR_TEMPLATE                                                               \
    template<typename IGetNodeLvc, typename IGetNodeBindings, typename IStoreNodeLvc,                    \
             typename IStoreNodeBindings, typename IGetInference, typename IGetNodeGoals,                \
             typename IGetNodeAxiom, typename IBuildChildGoalList, typename IStoreNodeGoals,             \
             typename IStoreNodeAxiom, typename IIncrementLiveChildCount, typename IBind>
#define PUD_PLAIN_CHILD_ACTIVATOR                                                                        \
    pud_plain_child_activator<IGetNodeLvc, IGetNodeBindings, IStoreNodeLvc, IStoreNodeBindings,          \
        IGetInference, IGetNodeGoals, IGetNodeAxiom, IBuildChildGoalList, IStoreNodeGoals,               \
        IStoreNodeAxiom, IIncrementLiveChildCount, IBind>

PUD_PLAIN_CHILD_ACTIVATOR_TEMPLATE
PUD_PLAIN_CHILD_ACTIVATOR::pud_plain_child_activator(
    IGetNodeLvc& get_node_lvc,
    IGetNodeBindings& get_node_bindings,
    IStoreNodeLvc& store_node_lvc,
    IStoreNodeBindings& store_node_bindings,
    IGetInference& get_inference,
    IGetNodeGoals& get_node_goals,
    IGetNodeAxiom& get_node_axiom,
    IBuildChildGoalList& build_child_goal_list,
    IStoreNodeGoals& store_node_goals,
    IStoreNodeAxiom& store_node_axiom,
    IIncrementLiveChildCount& increment_live_child_count,
    IBind& bind)
    : get_node_lvc_(get_node_lvc)
    , get_node_bindings_(get_node_bindings)
    , store_node_lvc_(store_node_lvc)
    , store_node_bindings_(store_node_bindings)
    , get_inference_(get_inference)
    , get_node_goals_(get_node_goals)
    , get_node_axiom_(get_node_axiom)
    , build_child_goal_list_(build_child_goal_list)
    , store_node_goals_(store_node_goals)
    , store_node_axiom_(store_node_axiom)
    , increment_live_child_count_(increment_live_child_count)
    , bind_(bind) {}

PUD_PLAIN_CHILD_ACTIVATOR_TEMPLATE
bool PUD_PLAIN_CHILD_ACTIVATOR::activate_child(pud_global_node_id parent, pud_global_node_id child) {
    const pud_inference& inference = get_inference_.get_inference(child);
    const pud_goal_list& parent_goals = get_node_goals_.get(parent);
    store_node_goals_.store(child, build_child_goal_list_.build(parent_goals, inference.call_site, inference.body_goals));
    store_node_axiom_.store(child, get_node_axiom_.get(parent));
    increment_live_child_count_.increment(parent);
    store_node_lvc_.store(child, get_node_lvc_.get(parent) + inference.var_count);
    store_node_bindings_.store(child, bind_.bind(get_node_bindings_.get(parent), child));
    return false;
}

#undef PUD_PLAIN_CHILD_ACTIVATOR
#undef PUD_PLAIN_CHILD_ACTIVATOR_TEMPLATE

#endif

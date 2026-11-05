#ifndef PUD_TRAIL_CHILD_ACTIVATOR_HPP
#define PUD_TRAIL_CHILD_ACTIVATOR_HPP

#include <cstdint>
#include <utility>
#include <immer/box.hpp>
#include "value_objects/pud_body_goal_id.hpp"
#include "value_objects/pud_global_node_id.hpp"
#include "value_objects/pud_goal_list.hpp"
#include "value_objects/pud_goal_trails.hpp"
#include "value_objects/pud_inference.hpp"
#include "value_objects/pud_node_trails.hpp"
#include "value_objects/pud_structure_watch.hpp"
#include "value_objects/pud_structure_watch_change.hpp"
#include "value_objects/pud_ubs_state.hpp"

// Activates a child with what every live leaf has, plus its structure watch and its parent's trails. The closed goal's
// trails are dropped, a goal whose structure the child's bindings changed has its trails marked forked, and the
// inference's new goals have none. A child is never refuted on arrival: its goals are queried when it is visited.
template<
    typename IGetNodeLvc,
    typename IGetNodeBindings,
    typename IGetNodeWatch,
    typename IGetNodeTrails,
    typename IStoreNodeLvc,
    typename IStoreNodeBindings,
    typename IStoreNodeWatch,
    typename IStoreNodeTrails,
    typename IGetInference,
    typename IGetNodeGoals,
    typename IGetNodeAxiom,
    typename IBuildChildGoalList,
    typename IStoreNodeGoals,
    typename IStoreNodeAxiom,
    typename IIncrementLiveChildCount,
    typename IBind,
    typename IWatchDescend>
struct pud_trail_child_activator {
    pud_trail_child_activator(
        IGetNodeLvc& get_node_lvc,
        IGetNodeBindings& get_node_bindings,
        IGetNodeWatch& get_node_watch,
        IGetNodeTrails& get_node_trails,
        IStoreNodeLvc& store_node_lvc,
        IStoreNodeBindings& store_node_bindings,
        IStoreNodeWatch& store_node_watch,
        IStoreNodeTrails& store_node_trails,
        IGetInference& get_inference,
        IGetNodeGoals& get_node_goals,
        IGetNodeAxiom& get_node_axiom,
        IBuildChildGoalList& build_child_goal_list,
        IStoreNodeGoals& store_node_goals,
        IStoreNodeAxiom& store_node_axiom,
        IIncrementLiveChildCount& increment_live_child_count,
        IBind& bind,
        IWatchDescend& watch_descend);
    bool activate_child(pud_global_node_id parent, pud_global_node_id child);
private:
    IGetNodeLvc& get_node_lvc_;
    IGetNodeBindings& get_node_bindings_;
    IGetNodeWatch& get_node_watch_;
    IGetNodeTrails& get_node_trails_;
    IStoreNodeLvc& store_node_lvc_;
    IStoreNodeBindings& store_node_bindings_;
    IStoreNodeWatch& store_node_watch_;
    IStoreNodeTrails& store_node_trails_;
    IGetInference& get_inference_;
    IGetNodeGoals& get_node_goals_;
    IGetNodeAxiom& get_node_axiom_;
    IBuildChildGoalList& build_child_goal_list_;
    IStoreNodeGoals& store_node_goals_;
    IStoreNodeAxiom& store_node_axiom_;
    IIncrementLiveChildCount& increment_live_child_count_;
    IBind& bind_;
    IWatchDescend& watch_descend_;
};

#define PUD_TRAIL_CHILD_ACTIVATOR_TEMPLATE                                                               \
    template<typename IGetNodeLvc, typename IGetNodeBindings, typename IGetNodeWatch,                    \
             typename IGetNodeTrails, typename IStoreNodeLvc, typename IStoreNodeBindings,               \
             typename IStoreNodeWatch, typename IStoreNodeTrails, typename IGetInference,                \
             typename IGetNodeGoals, typename IGetNodeAxiom, typename IBuildChildGoalList,               \
             typename IStoreNodeGoals, typename IStoreNodeAxiom, typename IIncrementLiveChildCount,      \
             typename IBind, typename IWatchDescend>
#define PUD_TRAIL_CHILD_ACTIVATOR                                                                        \
    pud_trail_child_activator<IGetNodeLvc, IGetNodeBindings, IGetNodeWatch, IGetNodeTrails,              \
        IStoreNodeLvc, IStoreNodeBindings, IStoreNodeWatch, IStoreNodeTrails, IGetInference,             \
        IGetNodeGoals, IGetNodeAxiom, IBuildChildGoalList, IStoreNodeGoals, IStoreNodeAxiom,             \
        IIncrementLiveChildCount, IBind, IWatchDescend>

PUD_TRAIL_CHILD_ACTIVATOR_TEMPLATE
PUD_TRAIL_CHILD_ACTIVATOR::pud_trail_child_activator(
    IGetNodeLvc& get_node_lvc,
    IGetNodeBindings& get_node_bindings,
    IGetNodeWatch& get_node_watch,
    IGetNodeTrails& get_node_trails,
    IStoreNodeLvc& store_node_lvc,
    IStoreNodeBindings& store_node_bindings,
    IStoreNodeWatch& store_node_watch,
    IStoreNodeTrails& store_node_trails,
    IGetInference& get_inference,
    IGetNodeGoals& get_node_goals,
    IGetNodeAxiom& get_node_axiom,
    IBuildChildGoalList& build_child_goal_list,
    IStoreNodeGoals& store_node_goals,
    IStoreNodeAxiom& store_node_axiom,
    IIncrementLiveChildCount& increment_live_child_count,
    IBind& bind,
    IWatchDescend& watch_descend)
    : get_node_lvc_(get_node_lvc)
    , get_node_bindings_(get_node_bindings)
    , get_node_watch_(get_node_watch)
    , get_node_trails_(get_node_trails)
    , store_node_lvc_(store_node_lvc)
    , store_node_bindings_(store_node_bindings)
    , store_node_watch_(store_node_watch)
    , store_node_trails_(store_node_trails)
    , get_inference_(get_inference)
    , get_node_goals_(get_node_goals)
    , get_node_axiom_(get_node_axiom)
    , build_child_goal_list_(build_child_goal_list)
    , store_node_goals_(store_node_goals)
    , store_node_axiom_(store_node_axiom)
    , increment_live_child_count_(increment_live_child_count)
    , bind_(bind)
    , watch_descend_(watch_descend) {}

PUD_TRAIL_CHILD_ACTIVATOR_TEMPLATE
bool PUD_TRAIL_CHILD_ACTIVATOR::activate_child(pud_global_node_id parent, pud_global_node_id child) {
    const pud_inference& inference = get_inference_.get_inference(child);
    const pud_goal_list goals =
        build_child_goal_list_.build(get_node_goals_.get(parent), inference.call_site, inference.body_goals);
    store_node_goals_.store(child, goals);
    store_node_axiom_.store(child, get_node_axiom_.get(parent));
    increment_live_child_count_.increment(parent);
    pud_ubs_state bindings = bind_.bind(get_node_bindings_.get(parent), child);
    pud_structure_watch_change watched = watch_descend_.descend(get_node_watch_.get(parent), bindings, child, goals);
    pud_node_trails trails = get_node_trails_.get(parent).erase(inference.call_site);
    for (const pud_body_goal_id goal : watched.changed_goals) {
        const immer::box<pud_goal_trails>* inherited = trails.find(goal);
        if (inherited == nullptr)
            continue;
        trails = trails.set(goal, immer::box<pud_goal_trails>{
            pud_goal_trails{.candidates = inherited->get().candidates, .forked = true}});
    }
    store_node_lvc_.store(child, get_node_lvc_.get(parent) + inference.var_count);
    store_node_bindings_.store(child, std::move(bindings));
    store_node_watch_.store(child, std::move(watched.watch));
    store_node_trails_.store(child, std::move(trails));
    return false;
}

#undef PUD_TRAIL_CHILD_ACTIVATOR
#undef PUD_TRAIL_CHILD_ACTIVATOR_TEMPLATE

#endif

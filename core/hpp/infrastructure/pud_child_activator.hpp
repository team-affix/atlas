#ifndef PUD_CHILD_ACTIVATOR_HPP
#define PUD_CHILD_ACTIVATOR_HPP

#include <cstdint>
#include <optional>
#include <utility>
#include <immer/box.hpp>
#include <immer/map.hpp>
#include "value_objects/expr.hpp"
#include "value_objects/pud_body_goal_id.hpp"
#include "value_objects/pud_global_node_id.hpp"
#include "value_objects/pud_goal_list.hpp"
#include "value_objects/pud_inference.hpp"
#include "value_objects/pud_mhq_head_id.hpp"
#include "value_objects/pud_node_queries.hpp"
#include "value_objects/pud_structure_watch_change.hpp"
#include "value_objects/pud_ubs_state.hpp"
#include "infrastructure/pud_mhq_head_owner.hpp"
#include "debug_assert.hpp"

template<
    typename IGetNodeLvc,
    typename IGetNodeBindings,
    typename IGetNodeWatch,
    typename IGetNodeQueries,
    typename IStoreNodeLvc,
    typename IStoreNodeBindings,
    typename IStoreNodeWatch,
    typename IStoreNodeQueries,
    typename IGetInference,
    typename IGetNodeGoals,
    typename IGetNodeAxiom,
    typename IBuildChildGoalList,
    typename IStoreNodeGoals,
    typename IStoreNodeAxiom,
    typename IIncrementLiveChildCount,
    typename IBind,
    typename IWatchDescend,
    typename IForkQuery,
    typename IOpenQuery,
    typename IRemoveQuery,
    typename IIntroduceQuery,
    typename IGetExhaustedQueries>
struct pud_child_activator {
    pud_child_activator(
        IGetNodeLvc& get_node_lvc,
        IGetNodeBindings& get_node_bindings,
        IGetNodeWatch& get_node_watch,
        IGetNodeQueries& get_node_queries,
        IStoreNodeLvc& store_node_lvc,
        IStoreNodeBindings& store_node_bindings,
        IStoreNodeWatch& store_node_watch,
        IStoreNodeQueries& store_node_queries,
        IGetInference& get_inference,
        IGetNodeGoals& get_node_goals,
        IGetNodeAxiom& get_node_axiom,
        IBuildChildGoalList& build_child_goal_list,
        IStoreNodeGoals& store_node_goals,
        IStoreNodeAxiom& store_node_axiom,
        IIncrementLiveChildCount& increment_live_child_count,
        IBind& bind,
        IWatchDescend& watch_descend,
        IForkQuery& fork_query,
        IOpenQuery& open_query,
        IRemoveQuery& remove_query,
        IIntroduceQuery& introduce_query,
        IGetExhaustedQueries& get_exhausted_queries);
    bool activate_child(pud_global_node_id parent, pud_global_node_id child);
private:
    IGetNodeLvc& get_node_lvc_;
    IGetNodeBindings& get_node_bindings_;
    IGetNodeWatch& get_node_watch_;
    IGetNodeQueries& get_node_queries_;
    IStoreNodeLvc& store_node_lvc_;
    IStoreNodeBindings& store_node_bindings_;
    IStoreNodeWatch& store_node_watch_;
    IStoreNodeQueries& store_node_queries_;
    IGetInference& get_inference_;
    IGetNodeGoals& get_node_goals_;
    IGetNodeAxiom& get_node_axiom_;
    IBuildChildGoalList& build_child_goal_list_;
    IStoreNodeGoals& store_node_goals_;
    IStoreNodeAxiom& store_node_axiom_;
    IIncrementLiveChildCount& increment_live_child_count_;
    IBind& bind_;
    IWatchDescend& watch_descend_;
    IForkQuery& fork_query_;
    IOpenQuery& open_query_;
    IRemoveQuery& remove_query_;
    IIntroduceQuery& introduce_query_;
    IGetExhaustedQueries& get_exhausted_queries_;
};

#define PUD_CHILD_ACTIVATOR_TEMPLATE                                                           \
    template<typename IGetNodeLvc, typename IGetNodeBindings, typename IGetNodeWatch, typename IGetNodeQueries, \
             typename IStoreNodeLvc, typename IStoreNodeBindings, typename IStoreNodeWatch,             \
             typename IStoreNodeQueries, typename IGetInference,                                     \
             typename IGetNodeGoals, typename IGetNodeAxiom, typename IBuildChildGoalList,           \
             typename IStoreNodeGoals, typename IStoreNodeAxiom,                               \
             typename IIncrementLiveChildCount, typename IBind, typename IWatchDescend,        \
             typename IForkQuery, typename IOpenQuery, typename IRemoveQuery, typename IIntroduceQuery, \
             typename IGetExhaustedQueries>
#define PUD_CHILD_ACTIVATOR                                                                    \
    pud_child_activator<IGetNodeLvc, IGetNodeBindings, IGetNodeWatch, IGetNodeQueries, IStoreNodeLvc,  \
        IStoreNodeBindings, IStoreNodeWatch, IStoreNodeQueries, IGetInference, IGetNodeGoals,          \
        IGetNodeAxiom, IBuildChildGoalList, IStoreNodeGoals, IStoreNodeAxiom,        \
        IIncrementLiveChildCount, IBind, IWatchDescend, IForkQuery, IOpenQuery, IRemoveQuery, IIntroduceQuery, \
        IGetExhaustedQueries>

PUD_CHILD_ACTIVATOR_TEMPLATE
PUD_CHILD_ACTIVATOR::pud_child_activator(
    IGetNodeLvc& get_node_lvc,
    IGetNodeBindings& get_node_bindings,
    IGetNodeWatch& get_node_watch,
    IGetNodeQueries& get_node_queries,
    IStoreNodeLvc& store_node_lvc,
    IStoreNodeBindings& store_node_bindings,
    IStoreNodeWatch& store_node_watch,
    IStoreNodeQueries& store_node_queries,
    IGetInference& get_inference,
    IGetNodeGoals& get_node_goals,
    IGetNodeAxiom& get_node_axiom,
    IBuildChildGoalList& build_child_goal_list,
    IStoreNodeGoals& store_node_goals,
    IStoreNodeAxiom& store_node_axiom,
    IIncrementLiveChildCount& increment_live_child_count,
    IBind& bind,
    IWatchDescend& watch_descend,
    IForkQuery& fork_query,
    IOpenQuery& open_query,
    IRemoveQuery& remove_query,
    IIntroduceQuery& introduce_query,
    IGetExhaustedQueries& get_exhausted_queries)
    : get_node_lvc_(get_node_lvc)
    , get_node_bindings_(get_node_bindings)
    , get_node_watch_(get_node_watch)
    , get_node_queries_(get_node_queries)
    , store_node_lvc_(store_node_lvc)
    , store_node_bindings_(store_node_bindings)
    , store_node_watch_(store_node_watch)
    , store_node_queries_(store_node_queries)
    , get_inference_(get_inference)
    , get_node_goals_(get_node_goals)
    , get_node_axiom_(get_node_axiom)
    , build_child_goal_list_(build_child_goal_list)
    , store_node_goals_(store_node_goals)
    , store_node_axiom_(store_node_axiom)
    , increment_live_child_count_(increment_live_child_count)
    , bind_(bind)
    , watch_descend_(watch_descend)
    , fork_query_(fork_query)
    , open_query_(open_query)
    , remove_query_(remove_query)
    , introduce_query_(introduce_query)
    , get_exhausted_queries_(get_exhausted_queries) {}

PUD_CHILD_ACTIVATOR_TEMPLATE
bool PUD_CHILD_ACTIVATOR::activate_child(pud_global_node_id parent, pud_global_node_id child) {
    using owner_box_t = immer::box<pud_mhq_head_owner<IRemoveQuery>>;
    const pud_node_queries<IRemoveQuery>& parent_queries = get_node_queries_.get(parent);
    const pud_inference& inference = get_inference_.get_inference(child);
    const pud_goal_list& parent_goals = get_node_goals_.get(parent);
    const pud_goal_list goals = build_child_goal_list_.build(parent_goals, inference.call_site, inference.body_goals);
    store_node_goals_.store(child, goals);
    store_node_axiom_.store(child, get_node_axiom_.get(parent));
    increment_live_child_count_.increment(parent);
    const uint32_t lvc = get_node_lvc_.get(parent) + inference.var_count;
    pud_ubs_state bindings = bind_.bind(get_node_bindings_.get(parent), child);
    pud_structure_watch_change watched = watch_descend_.descend(get_node_watch_.get(parent), bindings, child, goals);
    pud_node_queries<IRemoveQuery> queries = parent_queries.erase(inference.call_site);
    bool refuted = false;
    for (pud_body_goal_id goal : watched.changed_goals) {
        const owner_box_t* inherited = queries.find(goal);
        if (inherited == nullptr)
            continue;
        const std::optional<pud_mhq_head_id> forked =
            fork_query_.try_fork((*inherited)->head_id(), bindings, goals.goals.at(goal), lvc);
        if (!forked.has_value()) {
            refuted = true;
            queries = queries.erase(goal);
            continue;
        }
        queries = queries.set(goal, owner_box_t{remove_query_, forked.value()});
        introduce_query_.introduce(forked.value(), child, goal);
    }
    for (pud_body_goal_id id = parent_goals.next_id; id < goals.next_id; ++id) {
        const std::optional<pud_mhq_head_id> opened =
            open_query_.try_open(child.module, bindings, goals.goals.at(id), lvc);
        if (!opened.has_value()) {
            refuted = true;
            continue;
        }
        queries = queries.set(id, owner_box_t{remove_query_, opened.value()});
        introduce_query_.introduce(opened.value(), child, id);
    }
    // Every inherited goal kept its parent's query or forked it, so an unrefuted child holds one per goal.
    DEBUG_ASSERT(refuted || queries.size() == goals.goals.size());
    // A parent activated under a query that has since lost every candidate passes it on unless this child closed
    // or forked its goal. An exhausted query goes once the leaves holding it are refuted, so few are outstanding.
    for (const auto& [head_id, introduction] : get_exhausted_queries_.exhausted()) {
        const owner_box_t* held = queries.find(introduction.goal);
        const bool inherited = held != nullptr && (*held)->head_id() == head_id;
        refuted = refuted || inherited;
    }
    store_node_lvc_.store(child, lvc);
    store_node_bindings_.store(child, std::move(bindings));
    store_node_watch_.store(child, std::move(watched.watch));
    store_node_queries_.store(child, std::move(queries));
    return refuted;
}

#undef PUD_CHILD_ACTIVATOR
#undef PUD_CHILD_ACTIVATOR_TEMPLATE

#endif

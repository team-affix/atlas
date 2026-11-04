#ifndef PUD_ROOT_ACTIVATOR_HPP
#define PUD_ROOT_ACTIVATOR_HPP

#include <optional>
#include <utility>
#include <immer/box.hpp>
#include <immer/map.hpp>
#include "value_objects/expr.hpp"
#include "value_objects/pud_body_goal_id.hpp"
#include "value_objects/pud_global_node_id.hpp"
#include "value_objects/pud_goal_list.hpp"
#include "value_objects/pud_mhq_head_id.hpp"
#include "value_objects/pud_node_queries.hpp"
#include "value_objects/pud_ubs_state.hpp"
#include "value_objects/rule.hpp"
#include "infrastructure/pud_mhq_head_owner.hpp"

template<
    typename IGetAxiom,
    typename IBuildRootGoalList,
    typename IStoreNodeGoals,
    typename IStoreNodeAxiom,
    typename IStoreNodeLvc,
    typename IStoreNodeBindings,
    typename IStoreNodeWatch,
    typename IStoreNodeQueries,
    typename IWatchRoot,
    typename IOpenQuery,
    typename IRemoveQuery,
    typename IIntroduceQuery>
struct pud_root_activator {
    pud_root_activator(
        IGetAxiom& get_axiom,
        IBuildRootGoalList& build_root_goal_list,
        IStoreNodeGoals& store_node_goals,
        IStoreNodeAxiom& store_node_axiom,
        IStoreNodeLvc& store_node_lvc,
        IStoreNodeBindings& store_node_bindings,
        IStoreNodeWatch& store_node_watch,
        IStoreNodeQueries& store_node_queries,
        IWatchRoot& watch_root,
        IOpenQuery& open_query,
        IRemoveQuery& remove_query,
        IIntroduceQuery& introduce_query);
    bool activate_root(pud_global_node_id root);
private:
    IGetAxiom& get_axiom_;
    IBuildRootGoalList& build_root_goal_list_;
    IStoreNodeGoals& store_node_goals_;
    IStoreNodeAxiom& store_node_axiom_;
    IStoreNodeLvc& store_node_lvc_;
    IStoreNodeBindings& store_node_bindings_;
    IStoreNodeWatch& store_node_watch_;
    IStoreNodeQueries& store_node_queries_;
    IWatchRoot& watch_root_;
    IOpenQuery& open_query_;
    IRemoveQuery& remove_query_;
    IIntroduceQuery& introduce_query_;
};

#define PUD_ROOT_ACTIVATOR_TEMPLATE                                                            \
    template<typename IGetAxiom, typename IBuildRootGoalList, typename IStoreNodeGoals,                \
             typename IStoreNodeAxiom, typename IStoreNodeLvc, typename IStoreNodeBindings,             \
             typename IStoreNodeWatch, typename IStoreNodeQueries, typename IWatchRoot,                 \
             typename IOpenQuery, typename IRemoveQuery, typename IIntroduceQuery>
#define PUD_ROOT_ACTIVATOR                                                                       \
    pud_root_activator<IGetAxiom, IBuildRootGoalList, IStoreNodeGoals, IStoreNodeAxiom,                \
        IStoreNodeLvc, IStoreNodeBindings, IStoreNodeWatch, IStoreNodeQueries, IWatchRoot, IOpenQuery,  \
        IRemoveQuery, IIntroduceQuery>

PUD_ROOT_ACTIVATOR_TEMPLATE
PUD_ROOT_ACTIVATOR::pud_root_activator(
    IGetAxiom& get_axiom,
    IBuildRootGoalList& build_root_goal_list,
    IStoreNodeGoals& store_node_goals,
    IStoreNodeAxiom& store_node_axiom,
    IStoreNodeLvc& store_node_lvc,
    IStoreNodeBindings& store_node_bindings,
    IStoreNodeWatch& store_node_watch,
    IStoreNodeQueries& store_node_queries,
    IWatchRoot& watch_root,
    IOpenQuery& open_query,
    IRemoveQuery& remove_query,
    IIntroduceQuery& introduce_query)
    : get_axiom_(get_axiom)
    , build_root_goal_list_(build_root_goal_list)
    , store_node_goals_(store_node_goals)
    , store_node_axiom_(store_node_axiom)
    , store_node_lvc_(store_node_lvc)
    , store_node_bindings_(store_node_bindings)
    , store_node_watch_(store_node_watch)
    , store_node_queries_(store_node_queries)
    , watch_root_(watch_root)
    , open_query_(open_query)
    , remove_query_(remove_query)
    , introduce_query_(introduce_query) {}

PUD_ROOT_ACTIVATOR_TEMPLATE
bool PUD_ROOT_ACTIVATOR::activate_root(pud_global_node_id root) {
    const rule& axiom = get_axiom_.get_axiom(root);
    const pud_goal_list goals = build_root_goal_list_.build(axiom.body);
    store_node_goals_.store(root, goals);
    store_node_axiom_.store(root, root);
    const pud_ubs_state bindings{};
    pud_node_queries<IRemoveQuery> queries;
    bool refuted = false;
    for (const auto& [id, goal] : goals.goals) {
        const std::optional<pud_mhq_head_id> opened = open_query_.try_open(root.module, bindings, goal, axiom.var_count);
        if (!opened.has_value()) {
            refuted = true;
            continue;
        }
        queries = queries.set(id, immer::box<pud_mhq_head_owner<IRemoveQuery>>{remove_query_, opened.value()});
        introduce_query_.introduce(opened.value(), root, id);
    }
    store_node_lvc_.store(root, axiom.var_count);
    store_node_bindings_.store(root, bindings);
    store_node_watch_.store(root, watch_root_.watch_root(bindings, goals, root.module));
    store_node_queries_.store(root, std::move(queries));
    return refuted;
}

#undef PUD_ROOT_ACTIVATOR
#undef PUD_ROOT_ACTIVATOR_TEMPLATE

#endif

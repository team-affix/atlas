#ifndef PUD_MANIFEST_HPP
#define PUD_MANIFEST_HPP

#include <deque>
#include "infrastructure/globalizer.hpp"
#include "infrastructure/pud_inference_builder.hpp"
#include "infrastructure/pud_activity_mover.hpp"
#include "infrastructure/pud_binder.hpp"
#include "infrastructure/pud_bind_map_factory.hpp"
#include "infrastructure/pud_child_activator.hpp"
#include "infrastructure/pud_cycle_detector_factory.hpp"
#include "infrastructure/pud_demoduler.hpp"
#include "infrastructure/pud_term_exporter_factory.hpp"
#include "infrastructure/pud_functor_linker.hpp"
#include "infrastructure/pud_functor_relinker.hpp"
#include "infrastructure/pud_link_table.hpp"
#include "infrastructure/pud_symbol_table.hpp"
#include "infrastructure/pud_child_goal_list_builder.hpp"
#include "infrastructure/pud_root_goal_list_builder.hpp"
#include "infrastructure/pud_leaf_refuter.hpp"
#include "infrastructure/pud_mhcs.hpp"
#include "infrastructure/pud_mhq.hpp"
#include "infrastructure/pud_mhws.hpp"
#include "infrastructure/pud_module_activator.hpp"
#include "infrastructure/pud_module_registry.hpp"
#include "infrastructure/pud_node_deactivator.hpp"
#include "infrastructure/pud_node_table.hpp"
#include "value_objects/pud_goal_list.hpp"
#include "infrastructure/pud_node_live_child_counts.hpp"
#include "infrastructure/pud_query_introducers.hpp"
#include "infrastructure/pud_query_leaf_finder.hpp"
#include "infrastructure/pud_query_remover.hpp"
#include "infrastructure/pud_refutation_propagator.hpp"
#include "infrastructure/pud_replayer.hpp"
#include "infrastructure/pud_resolver.hpp"
#include "infrastructure/pud_root_activator.hpp"
#include "infrastructure/pud_structure_watcher.hpp"
#include "infrastructure/pud_unifier_factory.hpp"
#include "infrastructure/pud_unfolder.hpp"
#include "value_objects/pud_mhq_head_id.hpp"
#include "value_objects/pud_node_queries.hpp"
#include "value_objects/pud_structure_watch.hpp"
#include "value_objects/pud_ubs_state.hpp"

template<typename IModule, typename IExprPool>
struct pud_manifest {
    struct node_goals_t : pud_node_table<pud_goal_list> {
        using pud_node_table<pud_goal_list>::pud_node_table;
    };
    struct node_axioms_t : pud_node_table<pud_global_node_id> {
        using pud_node_table<pud_global_node_id>::pud_node_table;
    };
    struct node_lvcs_t : pud_node_table<uint32_t> {
        using pud_node_table<uint32_t>::pud_node_table;
    };
    struct node_bindings_t : pud_node_table<pud_ubs_state> {
        using pud_node_table<pud_ubs_state>::pud_node_table;
    };
    struct node_watches_t : pud_node_table<pud_structure_watch> {
        using pud_node_table<pud_structure_watch>::pud_node_table;
    };
    struct bind_map_factory_t : pud_bind_map_factory {
        using pud_bind_map_factory::pud_bind_map_factory;
    };
    struct module_registry_t : pud_module_registry<IModule> {
        using pud_module_registry<IModule>::pud_module_registry;
    };
    struct demoduler_t : pud_demoduler<pud_link_table, IExprPool, IExprPool> {
        using pud_demoduler<pud_link_table, IExprPool, IExprPool>::pud_demoduler;
    };
    struct cycle_detector_factory_t : pud_cycle_detector_factory<globalizer, demoduler_t> {
        using pud_cycle_detector_factory<globalizer, demoduler_t>::pud_cycle_detector_factory;
    };
    struct functor_linker_t : pud_functor_linker<pud_symbol_table, pud_link_table> {
        using pud_functor_linker<pud_symbol_table, pud_link_table>::pud_functor_linker;
    };
    struct unifier_factory_t : pud_unifier_factory<
        globalizer, IExprPool, pud_link_table, typename cycle_detector_factory_t::cycle_detector_t> {
        using pud_unifier_factory<
            globalizer, IExprPool, pud_link_table,
            typename cycle_detector_factory_t::cycle_detector_t>::pud_unifier_factory;
    };
    struct functor_relinker_t : pud_functor_relinker<
        pud_link_table, pud_link_table, pud_symbol_table, module_registry_t, pud_link_table> {
        using pud_functor_relinker<
            pud_link_table, pud_link_table, pud_symbol_table, module_registry_t, pud_link_table>::pud_functor_relinker;
    };
    struct term_exporter_factory_t : pud_term_exporter_factory<
        globalizer, IExprPool, IExprPool, functor_relinker_t> {
        using pud_term_exporter_factory<
            globalizer, IExprPool, IExprPool, functor_relinker_t>::pud_term_exporter_factory;
    };
    struct unit_queries_t : std::deque<pud_mhq_head_id> {
        using std::deque<pud_mhq_head_id>::deque;
    };
    struct refutation_propagator_t : pud_refutation_propagator<
        module_registry_t, pud_node_live_child_counts, module_registry_t> {
        using pud_refutation_propagator<
            module_registry_t, pud_node_live_child_counts, module_registry_t>::pud_refutation_propagator;
    };
    struct replayer_t : pud_replayer<
        module_registry_t, module_registry_t, IExprPool, bind_map_factory_t, cycle_detector_factory_t,
        unifier_factory_t> {
        using pud_replayer<
            module_registry_t, module_registry_t, IExprPool, bind_map_factory_t, cycle_detector_factory_t,
            unifier_factory_t>::pud_replayer;
    };
    struct mhws_t : pud_mhws<replayer_t, module_registry_t, module_registry_t, module_registry_t> {
        using pud_mhws<replayer_t, module_registry_t, module_registry_t, module_registry_t>::pud_mhws;
    };
    struct mhcs_t : pud_mhcs<
        replayer_t, mhws_t, mhws_t, mhws_t, mhws_t, mhws_t, module_registry_t, module_registry_t, module_registry_t> {
        using pud_mhcs<
            replayer_t, mhws_t, mhws_t, mhws_t, mhws_t, mhws_t, module_registry_t, module_registry_t,
            module_registry_t>::pud_mhcs;
    };
    struct mhq_t : pud_mhq<replayer_t, module_registry_t, mhcs_t, mhcs_t, mhcs_t, mhcs_t, unit_queries_t> {
        using pud_mhq<replayer_t, module_registry_t, mhcs_t, mhcs_t, mhcs_t, mhcs_t, unit_queries_t>::pud_mhq;
    };
    struct query_remover_t : pud_query_remover<pud_query_introducers, mhq_t> {
        using pud_query_remover<pud_query_introducers, mhq_t>::pud_query_remover;
    };
    struct node_queries_t : pud_node_table<pud_node_queries<query_remover_t>> {
        using pud_node_table<pud_node_queries<query_remover_t>>::pud_node_table;
    };
    struct query_leaf_finder_t : pud_query_leaf_finder<
        pud_query_introducers, module_registry_t, module_registry_t, module_registry_t, node_queries_t> {
        using pud_query_leaf_finder<
            pud_query_introducers, module_registry_t, module_registry_t, module_registry_t,
            node_queries_t>::pud_query_leaf_finder;
    };
    struct binder_t : pud_binder<bind_map_factory_t, module_registry_t> {
        using pud_binder<bind_map_factory_t, module_registry_t>::pud_binder;
    };
    struct structure_watcher_t : pud_structure_watcher<bind_map_factory_t, module_registry_t, IExprPool> {
        using pud_structure_watcher<bind_map_factory_t, module_registry_t, IExprPool>::pud_structure_watcher;
    };
    struct root_activator_t : pud_root_activator<
        module_registry_t, pud_root_goal_list_builder, node_goals_t, node_axioms_t, node_lvcs_t, node_bindings_t, node_watches_t, node_queries_t,
        structure_watcher_t, mhq_t, query_remover_t, pud_query_introducers> {
        using pud_root_activator<
            module_registry_t, pud_root_goal_list_builder, node_goals_t, node_axioms_t, node_lvcs_t, node_bindings_t, node_watches_t, node_queries_t,
            structure_watcher_t, mhq_t, query_remover_t, pud_query_introducers>::pud_root_activator;
    };
    struct child_activator_t : pud_child_activator<
        node_lvcs_t, node_bindings_t, node_watches_t, node_queries_t, node_lvcs_t, node_bindings_t, node_watches_t, node_queries_t, module_registry_t, node_goals_t, node_axioms_t,
        pud_child_goal_list_builder, node_goals_t, node_axioms_t, pud_node_live_child_counts, binder_t,
        structure_watcher_t, mhq_t, mhq_t, query_remover_t, pud_query_introducers, pud_query_introducers> {
        using pud_child_activator<
            node_lvcs_t, node_bindings_t, node_watches_t, node_queries_t, node_lvcs_t, node_bindings_t, node_watches_t, node_queries_t, module_registry_t, node_goals_t, node_axioms_t,
            pud_child_goal_list_builder, node_goals_t, node_axioms_t, pud_node_live_child_counts, binder_t,
            structure_watcher_t, mhq_t, mhq_t, query_remover_t, pud_query_introducers, pud_query_introducers>::pud_child_activator;
    };
    struct node_deactivator_t : pud_node_deactivator<
        node_queries_t, node_watches_t, node_bindings_t, node_lvcs_t, mhq_t, pud_query_introducers, query_leaf_finder_t> {
        using pud_node_deactivator<
        node_queries_t, node_watches_t, node_bindings_t, node_lvcs_t, mhq_t, pud_query_introducers, query_leaf_finder_t>::pud_node_deactivator;
    };
    struct leaf_refuter_t : pud_leaf_refuter<
        module_registry_t, refutation_propagator_t, node_deactivator_t, module_registry_t, module_registry_t> {
        using pud_leaf_refuter<
            module_registry_t, refutation_propagator_t, node_deactivator_t, module_registry_t,
            module_registry_t>::pud_leaf_refuter;
    };
    struct inference_builder_t : pud_inference_builder<
        node_goals_t, module_registry_t, module_registry_t, IExprPool, IExprPool, bind_map_factory_t,
        cycle_detector_factory_t, unifier_factory_t, term_exporter_factory_t> {
        using pud_inference_builder<
            node_goals_t, module_registry_t, module_registry_t, IExprPool, IExprPool, bind_map_factory_t,
            cycle_detector_factory_t, unifier_factory_t, term_exporter_factory_t>::pud_inference_builder;
    };
    struct resolver_t : pud_resolver<node_queries_t, node_bindings_t, node_lvcs_t, node_goals_t, mhq_t, mhcs_t, inference_builder_t> {
        using pud_resolver<node_queries_t, node_bindings_t, node_lvcs_t, node_goals_t, mhq_t, mhcs_t, inference_builder_t>::pud_resolver;
    };
    struct activity_mover_t : pud_activity_mover<child_activator_t, leaf_refuter_t, node_deactivator_t> {
        using pud_activity_mover<child_activator_t, leaf_refuter_t, node_deactivator_t>::pud_activity_mover;
    };
    struct unfolder_t : pud_unfolder<resolver_t, module_registry_t, activity_mover_t, module_registry_t> {
        using pud_unfolder<resolver_t, module_registry_t, activity_mover_t, module_registry_t>::pud_unfolder;
    };
    struct module_activator_t : pud_module_activator<
        module_registry_t, module_registry_t, module_registry_t, root_activator_t, leaf_refuter_t, activity_mover_t> {
        using pud_module_activator<
            module_registry_t, module_registry_t, module_registry_t, root_activator_t, leaf_refuter_t,
            activity_mover_t>::pud_module_activator;
    };

    pud_manifest(IExprPool& expr_pool);

    pud_symbol_table           symbol_table_;
    pud_link_table             link_table_;
    demoduler_t                demoduler_;
    functor_linker_t           functor_linker_;
    globalizer                 globalizer_;
    cycle_detector_factory_t   cycle_detector_factory_;
    unifier_factory_t          unifier_factory_;
    bind_map_factory_t         bind_map_factory_;
    module_registry_t                  modules_;
    functor_relinker_t         functor_relinker_;
    term_exporter_factory_t         term_exporter_factory_;
    node_goals_t               node_goals_;
    node_axioms_t              node_axioms_;
    pud_child_goal_list_builder          child_goal_list_builder_;
    pud_root_goal_list_builder    root_goal_list_builder_;
    pud_node_live_child_counts live_child_counts_;
    refutation_propagator_t    refutation_propagator_;

    unit_queries_t             unit_queries_;
    replayer_t                 replayer_;
    mhws_t                     mhws_;
    mhcs_t                     mhcs_;
    mhq_t                      mhq_;

    node_lvcs_t                node_lvcs_;
    node_bindings_t            node_bindings_;
    node_watches_t             node_watches_;
    pud_query_introducers      query_introducers_;
    query_remover_t            query_remover_;
    node_queries_t             node_queries_;
    query_leaf_finder_t        query_leaf_finder_;
    binder_t                   binder_;
    structure_watcher_t        structure_watcher_;
    root_activator_t           root_activator_;
    child_activator_t          child_activator_;
    node_deactivator_t         node_deactivator_;
    leaf_refuter_t             leaf_refuter_;

    inference_builder_t                 inference_builder_;
    resolver_t                 resolver_;
    activity_mover_t           activity_mover_;
    unfolder_t                 unfolder_;
    module_activator_t         module_activator_;
};

template<typename IModule, typename IExprPool>
pud_manifest<IModule, IExprPool>::pud_manifest(IExprPool& expr_pool)
    : symbol_table_()
    , link_table_()
    , demoduler_(link_table_, expr_pool, expr_pool)
    , functor_linker_(symbol_table_, link_table_)
    , globalizer_()
    , cycle_detector_factory_(globalizer_, demoduler_)
    , unifier_factory_(globalizer_, expr_pool, link_table_)
    , bind_map_factory_()
    , modules_()
    , functor_relinker_(link_table_, link_table_, symbol_table_, modules_, link_table_)
    , term_exporter_factory_(globalizer_, expr_pool, expr_pool, functor_relinker_)
    , node_goals_()
    , node_axioms_()
    , child_goal_list_builder_()
    , root_goal_list_builder_()
    , live_child_counts_()
    , refutation_propagator_(modules_, live_child_counts_, modules_)
    , unit_queries_()
    , replayer_(modules_, modules_, expr_pool, bind_map_factory_, cycle_detector_factory_, unifier_factory_)
    , mhws_(replayer_, modules_, modules_, modules_)
    , mhcs_(replayer_, mhws_, mhws_, mhws_, mhws_, mhws_, modules_, modules_, modules_)
    , mhq_(replayer_, modules_, mhcs_, mhcs_, mhcs_, mhcs_, unit_queries_)
    , node_lvcs_()
    , node_bindings_()
    , node_watches_()
    , query_introducers_()
    , query_remover_(query_introducers_, mhq_)
    , node_queries_()
    , query_leaf_finder_(query_introducers_, modules_, modules_, modules_, node_queries_)
    , binder_(bind_map_factory_, modules_)
    , structure_watcher_(bind_map_factory_, modules_, expr_pool)
    , root_activator_(
          modules_, root_goal_list_builder_, node_goals_, node_axioms_, node_lvcs_, node_bindings_, node_watches_, node_queries_, structure_watcher_, mhq_,
          query_remover_, query_introducers_)
    , child_activator_(
          node_lvcs_, node_bindings_, node_watches_, node_queries_, node_lvcs_, node_bindings_, node_watches_, node_queries_, modules_, node_goals_, node_axioms_, child_goal_list_builder_, node_goals_,
          node_axioms_, live_child_counts_, binder_, structure_watcher_, mhq_, mhq_, query_remover_, query_introducers_, query_introducers_)
    , node_deactivator_(node_queries_, node_watches_, node_bindings_, node_lvcs_, mhq_, query_introducers_, query_leaf_finder_)
    , leaf_refuter_(modules_, refutation_propagator_, node_deactivator_, modules_, modules_)
    , inference_builder_(
          node_goals_, modules_, modules_, expr_pool, expr_pool, bind_map_factory_, cycle_detector_factory_,
          unifier_factory_, term_exporter_factory_)
    , resolver_(node_queries_, node_bindings_, node_lvcs_, node_goals_, mhq_, mhcs_, inference_builder_)
    , activity_mover_(child_activator_, leaf_refuter_, node_deactivator_)
    , unfolder_(resolver_, modules_, activity_mover_, modules_)
    , module_activator_(modules_, modules_, modules_, root_activator_, leaf_refuter_, activity_mover_) {}

#endif

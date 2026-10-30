#ifndef PUD_LODESTAR_MANIFEST_HPP
#define PUD_LODESTAR_MANIFEST_HPP

#include <cstddef>
#include <deque>
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/globalizer.hpp"
#include "infrastructure/pud_module.hpp"
#include "infrastructure/pud_acceptor.hpp"
#include "infrastructure/pud_active_states.hpp"
#include "infrastructure/pud_binder.hpp"
#include "infrastructure/pud_bind_map_factory.hpp"
#include "infrastructure/pud_child_activator.hpp"
#include "infrastructure/pud_cycle_detector_factory.hpp"
#include "infrastructure/pud_demoduler.hpp"
#include "infrastructure/pud_exporter_factory.hpp"
#include "infrastructure/pud_functor_demoduler.hpp"
#include "infrastructure/pud_functor_linker.hpp"
#include "infrastructure/pud_link_table.hpp"
#include "infrastructure/pud_symbol_table.hpp"
#include "infrastructure/pud_goal_resolver.hpp"
#include "infrastructure/pud_initial_goal_opener.hpp"
#include "infrastructure/pud_leaf_refuter.hpp"
#include "infrastructure/pud_lodestar_candidate_watch.hpp"
#include "infrastructure/pud_lodestar_driver.hpp"
#include "infrastructure/pud_lodestar_focus_extender.hpp"
#include "infrastructure/pud_lodestar_focus_seeder.hpp"
#include "infrastructure/pud_lodestar_focus_set.hpp"
#include "infrastructure/pud_lodestar_frontier.hpp"
#include "infrastructure/pud_lodestar_module_registrar.hpp"
#include "infrastructure/pud_lodestar_search_stepper.hpp"
#include "infrastructure/pud_lodestar_solution_taker.hpp"
#include "infrastructure/pud_lodestar_solved_leaf_finder.hpp"
#include "infrastructure/pud_lodestar_unfold_budget_store.hpp"
#include "infrastructure/pud_lodestar_witness_log.hpp"
#include "infrastructure/pud_mhcs.hpp"
#include "infrastructure/pud_mhq.hpp"
#include "infrastructure/pud_mhws.hpp"
#include "infrastructure/pud_module_activator.hpp"
#include "infrastructure/pud_modules.hpp"
#include "infrastructure/pud_node_axioms.hpp"
#include "infrastructure/pud_node_deactivator.hpp"
#include "infrastructure/pud_node_goals.hpp"
#include "infrastructure/pud_node_depth_counter.hpp"
#include "infrastructure/pud_node_live_child_counts.hpp"
#include "infrastructure/pud_pending_conflict_store.hpp"
#include "infrastructure/pud_query_holders.hpp"
#include "infrastructure/pud_refutation_propagator.hpp"
#include "infrastructure/pud_replayer.hpp"
#include "infrastructure/pud_resolver.hpp"
#include "infrastructure/pud_root_activator.hpp"
#include "infrastructure/pud_solution_normalizer.hpp"
#include "infrastructure/pud_solution_store.hpp"
#include "infrastructure/pud_solution_term_reader.hpp"
#include "infrastructure/pud_structure_watcher.hpp"
#include "infrastructure/pud_unfold_allower.hpp"
#include "infrastructure/pud_unifier_factory.hpp"
#include "infrastructure/pud_unfolder.hpp"
#include "value_objects/pud_global_node_id.hpp"
#include "value_objects/pud_mhq_head_id.hpp"

struct pud_lodestar_manifest {
    struct bind_map_factory_t : pud_bind_map_factory {
        using pud_bind_map_factory::pud_bind_map_factory;
    };
    struct modules_t : pud_modules<pud_module> {
        using pud_modules<pud_module>::pud_modules;
    };
    struct demoduler_t;
    struct functor_demoduler_t : pud_functor_demoduler<demoduler_t, pud_link_table, expr_pool> {
        using pud_functor_demoduler<demoduler_t, pud_link_table, expr_pool>::pud_functor_demoduler;
    };
    struct demoduler_t : pud_demoduler<functor_demoduler_t, expr_pool> {
        using pud_demoduler<functor_demoduler_t, expr_pool>::pud_demoduler;
    };
    struct cycle_detector_factory_t : pud_cycle_detector_factory<globalizer, functor_demoduler_t> {
        using pud_cycle_detector_factory<globalizer, functor_demoduler_t>::pud_cycle_detector_factory;
    };
    using functor_linker_t = pud_functor_linker<pud_symbol_table, pud_link_table>;
    struct unifier_factory_t : pud_unifier_factory<globalizer, expr_pool, demoduler_t, functor_demoduler_t> {
        using pud_unifier_factory<globalizer, expr_pool, demoduler_t, functor_demoduler_t>::pud_unifier_factory;
    };
    struct exporter_factory_t : pud_exporter_factory<
        globalizer, expr_pool, expr_pool, pud_link_table, pud_link_table, pud_link_table, pud_symbol_table, modules_t,
        pud_link_table> {
        using pud_exporter_factory<
            globalizer, expr_pool, expr_pool, pud_link_table, pud_link_table, pud_link_table, pud_symbol_table,
            modules_t, pud_link_table>::pud_exporter_factory;
    };
    struct unit_queries_t : std::deque<pud_mhq_head_id> {
        using std::deque<pud_mhq_head_id>::deque;
    };
    struct refutation_propagator_t : pud_refutation_propagator<modules_t, pud_node_live_child_counts, modules_t> {
        using pud_refutation_propagator<modules_t, pud_node_live_child_counts, modules_t>::pud_refutation_propagator;
    };
    struct replayer_t : pud_replayer<
        modules_t, modules_t, expr_pool, bind_map_factory_t, cycle_detector_factory_t, unifier_factory_t> {
        using pud_replayer<
            modules_t, modules_t, expr_pool, bind_map_factory_t, cycle_detector_factory_t, unifier_factory_t>::pud_replayer;
    };
    struct mhws_t : pud_mhws<replayer_t, modules_t, modules_t, modules_t> {
        using pud_mhws<replayer_t, modules_t, modules_t, modules_t>::pud_mhws;
    };
    struct witness_log_t : pud_lodestar_witness_log<mhws_t, modules_t> {
        using pud_lodestar_witness_log<mhws_t, modules_t>::pud_lodestar_witness_log;
    };
    struct mhcs_t : pud_mhcs<
        replayer_t, witness_log_t, witness_log_t, witness_log_t, witness_log_t, witness_log_t, modules_t, modules_t,
        modules_t> {
        using pud_mhcs<
            replayer_t, witness_log_t, witness_log_t, witness_log_t, witness_log_t, witness_log_t, modules_t,
            modules_t, modules_t>::pud_mhcs;
    };
    struct candidate_watch_t : pud_lodestar_candidate_watch<mhcs_t, witness_log_t> {
        using pud_lodestar_candidate_watch<mhcs_t, witness_log_t>::pud_lodestar_candidate_watch;
    };
    struct mhq_t : pud_mhq<replayer_t, modules_t, candidate_watch_t, candidate_watch_t, candidate_watch_t, candidate_watch_t, unit_queries_t> {
        using pud_mhq<
            replayer_t, modules_t, candidate_watch_t, candidate_watch_t, candidate_watch_t, candidate_watch_t,
            unit_queries_t>::pud_mhq;
    };
    struct binder_t : pud_binder<bind_map_factory_t, modules_t> {
        using pud_binder<bind_map_factory_t, modules_t>::pud_binder;
    };
    struct structure_watcher_t : pud_structure_watcher<bind_map_factory_t, modules_t, expr_pool> {
        using pud_structure_watcher<bind_map_factory_t, modules_t, expr_pool>::pud_structure_watcher;
    };
    struct root_activator_t : pud_root_activator<
        modules_t, pud_initial_goal_opener, pud_node_goals, pud_node_axioms, pud_active_states, structure_watcher_t,
        mhq_t, pud_query_holders> {
        using pud_root_activator<
            modules_t, pud_initial_goal_opener, pud_node_goals, pud_node_axioms, pud_active_states, structure_watcher_t,
            mhq_t, pud_query_holders>::pud_root_activator;
    };
    struct child_activator_t : pud_child_activator<
        pud_active_states, pud_active_states, modules_t, pud_node_goals, pud_node_axioms, pud_goal_resolver,
        pud_node_goals, pud_node_goals, pud_node_axioms, pud_node_live_child_counts, binder_t, structure_watcher_t,
        mhq_t, mhq_t, pud_query_holders, mhq_t> {
        using pud_child_activator<
            pud_active_states, pud_active_states, modules_t, pud_node_goals, pud_node_axioms, pud_goal_resolver,
            pud_node_goals, pud_node_goals, pud_node_axioms, pud_node_live_child_counts, binder_t, structure_watcher_t,
            mhq_t, mhq_t, pud_query_holders, mhq_t>::pud_child_activator;
    };
    struct node_deactivator_t : pud_node_deactivator<
        pud_active_states, pud_active_states, pud_query_holders, mhq_t, mhq_t, pud_query_holders> {
        using pud_node_deactivator<
            pud_active_states, pud_active_states, pud_query_holders, mhq_t, mhq_t, pud_query_holders>::pud_node_deactivator;
    };
    struct leaf_refuter_t : pud_leaf_refuter<modules_t, refutation_propagator_t, node_deactivator_t, modules_t, modules_t> {
        using pud_leaf_refuter<modules_t, refutation_propagator_t, node_deactivator_t, modules_t, modules_t>::pud_leaf_refuter;
    };
    struct acceptor_t : pud_acceptor<
        pud_node_goals, modules_t, modules_t, expr_pool, expr_pool, bind_map_factory_t, cycle_detector_factory_t,
        unifier_factory_t, exporter_factory_t> {
        using pud_acceptor<
            pud_node_goals, modules_t, modules_t, expr_pool, expr_pool, bind_map_factory_t, cycle_detector_factory_t,
            unifier_factory_t, exporter_factory_t>::pud_acceptor;
    };
    struct resolver_t : pud_resolver<pud_active_states, pud_node_goals, mhq_t, mhcs_t, acceptor_t> {
        using pud_resolver<pud_active_states, pud_node_goals, mhq_t, mhcs_t, acceptor_t>::pud_resolver;
    };
    struct unfolder_t : pud_unfolder<resolver_t, modules_t, child_activator_t, leaf_refuter_t, node_deactivator_t, modules_t> {
        using pud_unfolder<resolver_t, modules_t, child_activator_t, leaf_refuter_t, node_deactivator_t, modules_t>::pud_unfolder;
    };
    struct module_activator_t : pud_module_activator<
        modules_t, modules_t, modules_t, root_activator_t, child_activator_t, leaf_refuter_t, node_deactivator_t> {
        using pud_module_activator<
            modules_t, modules_t, modules_t, root_activator_t, child_activator_t, leaf_refuter_t,
            node_deactivator_t>::pud_module_activator;
    };
    using driver_t = pud_lodestar_driver<pud_lodestar_manifest>;
    struct focus_seeder_t : pud_lodestar_focus_seeder<modules_t, pud_lodestar_focus_set, pud_lodestar_focus_set> {
        using pud_lodestar_focus_seeder<modules_t, pud_lodestar_focus_set, pud_lodestar_focus_set>::pud_lodestar_focus_seeder;
    };
    struct focus_extender_t : pud_lodestar_focus_extender<pud_lodestar_focus_set, pud_lodestar_focus_set> {
        using pud_lodestar_focus_extender<pud_lodestar_focus_set, pud_lodestar_focus_set>::pud_lodestar_focus_extender;
    };
    struct node_depth_counter_t : pud_node_depth_counter<modules_t> {
        using pud_node_depth_counter<modules_t>::pud_node_depth_counter;
    };
    struct unfold_allower_t : pud_unfold_allower<node_depth_counter_t, pud_node_goals> {
        using pud_unfold_allower<node_depth_counter_t, pud_node_goals>::pud_unfold_allower;
    };
    struct solved_leaf_finder_t : pud_lodestar_solved_leaf_finder<
        pud_lodestar_frontier, pud_lodestar_focus_set, modules_t, pud_node_goals> {
        using pud_lodestar_solved_leaf_finder<
            pud_lodestar_frontier, pud_lodestar_focus_set, modules_t, pud_node_goals>::pud_lodestar_solved_leaf_finder;
    };
    struct solution_term_reader_t : pud_solution_term_reader<pud_node_axioms, modules_t, pud_active_states> {
        using pud_solution_term_reader<pud_node_axioms, modules_t, pud_active_states>::pud_solution_term_reader;
    };
    struct solution_normalizer_t : pud_solution_normalizer<pud_link_table, expr_pool, expr_pool> {
        using pud_solution_normalizer<pud_link_table, expr_pool, expr_pool>::pud_solution_normalizer;
    };
    struct solution_taker_t : pud_lodestar_solution_taker<
        solved_leaf_finder_t, solution_term_reader_t, solution_normalizer_t, node_depth_counter_t, pud_solution_store,
        pud_lodestar_frontier> {
        using pud_lodestar_solution_taker<
            solved_leaf_finder_t, solution_term_reader_t, solution_normalizer_t, node_depth_counter_t,
            pud_solution_store, pud_lodestar_frontier>::pud_lodestar_solution_taker;
    };
    struct search_stepper_t : pud_lodestar_search_stepper<
        pud_solution_store, pud_solution_store, pud_pending_conflict_store, pud_pending_conflict_store,
        pud_lodestar_unfold_budget_store, pud_lodestar_unfold_budget_store, pud_lodestar_unfold_budget_store,
        pud_lodestar_unfold_budget_store, pud_lodestar_unfold_budget_store, pud_lodestar_unfold_budget_store,
        pud_lodestar_unfold_budget_store, solution_taker_t, driver_t, driver_t, driver_t> {
        using pud_lodestar_search_stepper<
            pud_solution_store, pud_solution_store, pud_pending_conflict_store, pud_pending_conflict_store,
            pud_lodestar_unfold_budget_store, pud_lodestar_unfold_budget_store, pud_lodestar_unfold_budget_store,
            pud_lodestar_unfold_budget_store, pud_lodestar_unfold_budget_store, pud_lodestar_unfold_budget_store,
            pud_lodestar_unfold_budget_store, solution_taker_t, driver_t, driver_t,
            driver_t>::pud_lodestar_search_stepper;
    };
    struct module_registrar_t : pud_lodestar_module_registrar<
        pud_module, functor_linker_t, modules_t, focus_seeder_t, module_activator_t, pud_lodestar_frontier,
        pud_pending_conflict_store, pud_pending_conflict_store> {
        using pud_lodestar_module_registrar<
            pud_module, functor_linker_t, modules_t, focus_seeder_t, module_activator_t, pud_lodestar_frontier,
            pud_pending_conflict_store, pud_pending_conflict_store>::pud_lodestar_module_registrar;
    };

    pud_lodestar_manifest(
        expr_pool& expr_pool,
        pud_global_node_id focus_axiom,
        size_t max_unfolds,
        size_t non_conjecture_unfolds_per_conjecture_unfold,
        size_t max_node_depth);

    pud_symbol_table           symbol_table_;
    pud_link_table             link_table_;
    functor_demoduler_t        functor_demoduler_;
    demoduler_t                demoduler_;
    functor_linker_t           functor_linker_;
    globalizer                 globalizer_;
    cycle_detector_factory_t   cycle_detector_factory_;
    unifier_factory_t          unifier_factory_;
    bind_map_factory_t         bind_map_factory_;
    modules_t                  modules_;
    exporter_factory_t         exporter_factory_;
    pud_node_goals             node_goals_;
    pud_node_axioms            node_axioms_;
    pud_goal_resolver          goal_resolver_;
    pud_initial_goal_opener    initial_goal_opener_;
    pud_node_live_child_counts live_child_counts_;
    refutation_propagator_t    refutation_propagator_;

    unit_queries_t             unit_queries_;
    replayer_t                 replayer_;
    mhws_t                     mhws_;
    witness_log_t              witness_log_;
    mhcs_t                     mhcs_;
    candidate_watch_t          candidate_watch_;
    mhq_t                      mhq_;

    pud_active_states          active_states_;
    pud_query_holders          query_holders_;
    binder_t                   binder_;
    structure_watcher_t        structure_watcher_;
    root_activator_t           root_activator_;
    child_activator_t          child_activator_;
    node_deactivator_t         node_deactivator_;
    leaf_refuter_t             leaf_refuter_;

    acceptor_t                 acceptor_;
    resolver_t                 resolver_;
    unfolder_t                 unfolder_;
    module_activator_t         module_activator_;

    pud_lodestar_focus_set           focus_set_;
    pud_lodestar_unfold_budget_store unfold_budget_store_;
    pud_solution_store               solution_store_;
    pud_pending_conflict_store       pending_conflict_store_;
    pud_lodestar_frontier            frontier_;

    focus_seeder_t                   focus_seeder_;
    focus_extender_t                 focus_extender_;
    node_depth_counter_t             node_depth_counter_;
    unfold_allower_t                 unfold_allower_;
    driver_t                         driver_;
    solved_leaf_finder_t             solved_leaf_finder_;
    solution_term_reader_t           solution_term_reader_;
    solution_normalizer_t            solution_normalizer_;
    solution_taker_t                 solution_taker_;
    search_stepper_t                 search_stepper_;
    module_registrar_t               module_registrar_;
};

#endif

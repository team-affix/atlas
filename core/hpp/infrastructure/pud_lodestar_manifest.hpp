#ifndef PUD_LODESTAR_MANIFEST_HPP
#define PUD_LODESTAR_MANIFEST_HPP

#include <cstddef>
#include <deque>
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/globalizer.hpp"
#include "infrastructure/pud_module.hpp"
#include "infrastructure/pud_inference_builder.hpp"
#include "infrastructure/pud_activity_mover.hpp"
#include "infrastructure/pud_active_states.hpp"
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
#include "infrastructure/pud_blocked_leaf_refuter.hpp"
#include "infrastructure/pud_candidate_witness_index.hpp"
#include "infrastructure/pud_lodestar_driver.hpp"
#include "infrastructure/pud_focus_extender.hpp"
#include "infrastructure/pud_focus_seeder.hpp"
#include "infrastructure/pud_focus_set.hpp"
#include "infrastructure/pud_frontier.hpp"
#include "infrastructure/pud_module_loader.hpp"
#include "infrastructure/pud_focus_query_lister.hpp"
#include "infrastructure/pud_lodestar_ring_builder.hpp"
#include "infrastructure/pud_lodestar_round_robin.hpp"
#include "infrastructure/pud_search_stepper.hpp"
#include "infrastructure/pud_solution_taker.hpp"
#include "infrastructure/pud_solved_leaf_finder.hpp"
#include "infrastructure/pud_unfold_applier.hpp"
#include "infrastructure/pud_unit_propagator.hpp"
#include "infrastructure/pud_witness_search_recorder.hpp"
#include "infrastructure/pud_mhcs.hpp"
#include "infrastructure/pud_mhq.hpp"
#include "infrastructure/pud_mhws.hpp"
#include "infrastructure/pud_module_activator.hpp"
#include "infrastructure/pud_module_registry.hpp"
#include "infrastructure/pud_node_deactivator.hpp"
#include "infrastructure/pud_node_table.hpp"
#include "value_objects/pud_goal_list.hpp"
#include "infrastructure/pud_node_depth_counter.hpp"
#include "infrastructure/pud_node_live_child_counts.hpp"
#include "infrastructure/pud_pending_conflict_store.hpp"
#include "infrastructure/pud_query_holders.hpp"
#include "infrastructure/pud_refutation_propagator.hpp"
#include "infrastructure/pud_replayer.hpp"
#include "infrastructure/pud_resolver.hpp"
#include "infrastructure/pud_root_activator.hpp"
#include "infrastructure/pud_normalizer_factory.hpp"
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
    struct node_goals_t : pud_node_table<pud_goal_list> {
        using pud_node_table<pud_goal_list>::pud_node_table;
    };
    struct node_axioms_t : pud_node_table<pud_global_node_id> {
        using pud_node_table<pud_global_node_id>::pud_node_table;
    };
    struct bind_map_factory_t : pud_bind_map_factory {
        using pud_bind_map_factory::pud_bind_map_factory;
    };
    struct module_registry_t : pud_module_registry<pud_module> {
        using pud_module_registry<pud_module>::pud_module_registry;
    };
    struct demoduler_t : pud_demoduler<pud_link_table, expr_pool, expr_pool> {
        using pud_demoduler<pud_link_table, expr_pool, expr_pool>::pud_demoduler;
    };
    struct cycle_detector_factory_t : pud_cycle_detector_factory<globalizer, demoduler_t> {
        using pud_cycle_detector_factory<globalizer, demoduler_t>::pud_cycle_detector_factory;
    };
    struct functor_linker_t : pud_functor_linker<pud_symbol_table, pud_link_table> {
        using pud_functor_linker<pud_symbol_table, pud_link_table>::pud_functor_linker;
    };
    struct unifier_factory_t : pud_unifier_factory<
        globalizer, expr_pool, pud_link_table, cycle_detector_factory_t::cycle_detector_t> {
        using pud_unifier_factory<
            globalizer, expr_pool, pud_link_table, cycle_detector_factory_t::cycle_detector_t>::pud_unifier_factory;
    };
    struct functor_relinker_t : pud_functor_relinker<
        pud_link_table, pud_link_table, pud_symbol_table, module_registry_t, pud_link_table> {
        using pud_functor_relinker<
            pud_link_table, pud_link_table, pud_symbol_table, module_registry_t, pud_link_table>::pud_functor_relinker;
    };
    struct term_exporter_factory_t : pud_term_exporter_factory<globalizer, expr_pool, expr_pool, functor_relinker_t> {
        using pud_term_exporter_factory<globalizer, expr_pool, expr_pool, functor_relinker_t>::pud_term_exporter_factory;
    };
    struct unit_queries_t : std::deque<pud_mhq_head_id> {
        using std::deque<pud_mhq_head_id>::deque;
    };
    struct refutation_propagator_t : pud_refutation_propagator<module_registry_t, pud_node_live_child_counts, module_registry_t> {
        using pud_refutation_propagator<module_registry_t, pud_node_live_child_counts, module_registry_t>::pud_refutation_propagator;
    };
    struct replayer_t : pud_replayer<
        module_registry_t, module_registry_t, expr_pool, bind_map_factory_t, cycle_detector_factory_t, unifier_factory_t> {
        using pud_replayer<
            module_registry_t, module_registry_t, expr_pool, bind_map_factory_t, cycle_detector_factory_t, unifier_factory_t>::pud_replayer;
    };
    struct mhws_t : pud_mhws<replayer_t, module_registry_t, module_registry_t, module_registry_t> {
        using pud_mhws<replayer_t, module_registry_t, module_registry_t, module_registry_t>::pud_mhws;
    };
    struct witness_search_recorder_t : pud_witness_search_recorder<mhws_t> {
        using pud_witness_search_recorder<mhws_t>::pud_witness_search_recorder;
    };
    struct mhcs_t : pud_mhcs<
        replayer_t, witness_search_recorder_t, witness_search_recorder_t, witness_search_recorder_t, witness_search_recorder_t, witness_search_recorder_t, module_registry_t, module_registry_t,
        module_registry_t> {
        using pud_mhcs<
            replayer_t, witness_search_recorder_t, witness_search_recorder_t, witness_search_recorder_t, witness_search_recorder_t, witness_search_recorder_t, module_registry_t,
            module_registry_t, module_registry_t>::pud_mhcs;
    };
    struct candidate_witness_index_t : pud_candidate_witness_index<mhcs_t, witness_search_recorder_t> {
        using pud_candidate_witness_index<mhcs_t, witness_search_recorder_t>::pud_candidate_witness_index;
    };
    struct mhq_t : pud_mhq<replayer_t, module_registry_t, candidate_witness_index_t, candidate_witness_index_t, candidate_witness_index_t, candidate_witness_index_t, unit_queries_t> {
        using pud_mhq<
            replayer_t, module_registry_t, candidate_witness_index_t, candidate_witness_index_t, candidate_witness_index_t, candidate_witness_index_t,
            unit_queries_t>::pud_mhq;
    };
    struct binder_t : pud_binder<bind_map_factory_t, module_registry_t> {
        using pud_binder<bind_map_factory_t, module_registry_t>::pud_binder;
    };
    struct structure_watcher_t : pud_structure_watcher<bind_map_factory_t, module_registry_t, expr_pool> {
        using pud_structure_watcher<bind_map_factory_t, module_registry_t, expr_pool>::pud_structure_watcher;
    };
    struct root_activator_t : pud_root_activator<
        module_registry_t, pud_root_goal_list_builder, node_goals_t, node_axioms_t, pud_active_states, structure_watcher_t,
        mhq_t, pud_query_holders> {
        using pud_root_activator<
            module_registry_t, pud_root_goal_list_builder, node_goals_t, node_axioms_t, pud_active_states, structure_watcher_t,
            mhq_t, pud_query_holders>::pud_root_activator;
    };
    struct child_activator_t : pud_child_activator<
        pud_active_states, pud_active_states, module_registry_t, node_goals_t, node_axioms_t, pud_child_goal_list_builder,
        node_goals_t, node_axioms_t, pud_node_live_child_counts, binder_t, structure_watcher_t,
        mhq_t, mhq_t, pud_query_holders, mhq_t> {
        using pud_child_activator<
            pud_active_states, pud_active_states, module_registry_t, node_goals_t, node_axioms_t, pud_child_goal_list_builder,
            node_goals_t, node_axioms_t, pud_node_live_child_counts, binder_t, structure_watcher_t,
            mhq_t, mhq_t, pud_query_holders, mhq_t>::pud_child_activator;
    };
    struct node_deactivator_t : pud_node_deactivator<
        pud_active_states, pud_active_states, pud_query_holders, mhq_t, mhq_t, pud_query_holders> {
        using pud_node_deactivator<
            pud_active_states, pud_active_states, pud_query_holders, mhq_t, mhq_t, pud_query_holders>::pud_node_deactivator;
    };
    struct leaf_refuter_t : pud_leaf_refuter<module_registry_t, refutation_propagator_t, node_deactivator_t, module_registry_t, module_registry_t> {
        using pud_leaf_refuter<module_registry_t, refutation_propagator_t, node_deactivator_t, module_registry_t, module_registry_t>::pud_leaf_refuter;
    };
    struct inference_builder_t : pud_inference_builder<
        node_goals_t, module_registry_t, module_registry_t, expr_pool, expr_pool, bind_map_factory_t, cycle_detector_factory_t,
        unifier_factory_t, term_exporter_factory_t> {
        using pud_inference_builder<
            node_goals_t, module_registry_t, module_registry_t, expr_pool, expr_pool, bind_map_factory_t, cycle_detector_factory_t,
            unifier_factory_t, term_exporter_factory_t>::pud_inference_builder;
    };
    struct resolver_t : pud_resolver<pud_active_states, node_goals_t, mhq_t, mhcs_t, inference_builder_t> {
        using pud_resolver<pud_active_states, node_goals_t, mhq_t, mhcs_t, inference_builder_t>::pud_resolver;
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
            module_registry_t, module_registry_t, module_registry_t, root_activator_t, leaf_refuter_t, activity_mover_t>::pud_module_activator;
    };
    struct focus_seeder_t : pud_focus_seeder<module_registry_t, pud_focus_set, pud_focus_set> {
        using pud_focus_seeder<module_registry_t, pud_focus_set, pud_focus_set>::pud_focus_seeder;
    };
    struct focus_extender_t : pud_focus_extender<pud_focus_set, pud_focus_set> {
        using pud_focus_extender<pud_focus_set, pud_focus_set>::pud_focus_extender;
    };
    struct node_depth_counter_t : pud_node_depth_counter<module_registry_t> {
        using pud_node_depth_counter<module_registry_t>::pud_node_depth_counter;
    };
    struct unfold_allower_t : pud_unfold_allower<node_depth_counter_t, node_goals_t> {
        using pud_unfold_allower<node_depth_counter_t, node_goals_t>::pud_unfold_allower;
    };
    struct query_lister_t : pud_focus_query_lister<
        pud_frontier, pud_focus_set, module_registry_t, pud_active_states, node_goals_t, mhq_t> {
        using pud_focus_query_lister<
            pud_frontier, pud_focus_set, module_registry_t, pud_active_states, node_goals_t,
            mhq_t>::pud_focus_query_lister;
    };
    struct ring_builder_t : pud_lodestar_ring_builder<
        mhq_t, candidate_witness_index_t, witness_search_recorder_t, pud_focus_set, pud_frontier, module_registry_t,
        pud_active_states, mhq_t, unfold_allower_t> {
        using pud_lodestar_ring_builder<
            mhq_t, candidate_witness_index_t, witness_search_recorder_t, pud_focus_set, pud_frontier, module_registry_t,
            pud_active_states, mhq_t, unfold_allower_t>::pud_lodestar_ring_builder;
    };
    struct unfold_applier_t : pud_unfold_applier<
        unfolder_t, pud_frontier, focus_extender_t, pud_focus_set> {
        using pud_unfold_applier<
            unfolder_t, pud_frontier, focus_extender_t, pud_focus_set>::pud_unfold_applier;
    };
    struct unit_propagator_t : pud_unit_propagator<
        unit_queries_t, pud_query_holders, pud_query_holders, mhq_t, pud_frontier, module_registry_t,
        unfold_allower_t, unfold_applier_t> {
        using pud_unit_propagator<
            unit_queries_t, pud_query_holders, pud_query_holders, mhq_t, pud_frontier, module_registry_t,
            unfold_allower_t, unfold_applier_t>::pud_unit_propagator;
    };
    struct blocked_refuter_t : pud_blocked_leaf_refuter<query_lister_t, leaf_refuter_t, pud_frontier> {
        using pud_blocked_leaf_refuter<
            query_lister_t, leaf_refuter_t, pud_frontier>::pud_blocked_leaf_refuter;
    };
    struct driver_t : pud_lodestar_driver<
        unit_propagator_t, query_lister_t, ring_builder_t, pud_lodestar_round_robin, pud_lodestar_round_robin,
        unfold_applier_t> {
        using pud_lodestar_driver<
            unit_propagator_t, query_lister_t, ring_builder_t, pud_lodestar_round_robin, pud_lodestar_round_robin,
            unfold_applier_t>::pud_lodestar_driver;
    };
    struct solved_leaf_finder_t : pud_solved_leaf_finder<
        pud_frontier, pud_focus_set, module_registry_t, node_goals_t> {
        using pud_solved_leaf_finder<
            pud_frontier, pud_focus_set, module_registry_t, node_goals_t>::pud_solved_leaf_finder;
    };
    struct solution_term_reader_t : pud_solution_term_reader<node_axioms_t, module_registry_t, pud_active_states> {
        using pud_solution_term_reader<node_axioms_t, module_registry_t, pud_active_states>::pud_solution_term_reader;
    };
    struct normalizer_factory_t : pud_normalizer_factory<globalizer, expr_pool, expr_pool, demoduler_t> {
        using pud_normalizer_factory<globalizer, expr_pool, expr_pool, demoduler_t>::pud_normalizer_factory;
    };
    struct solution_normalizer_t : pud_solution_normalizer<demoduler_t, normalizer_factory_t> {
        using pud_solution_normalizer<demoduler_t, normalizer_factory_t>::pud_solution_normalizer;
    };
    struct solution_taker_t : pud_solution_taker<
        solved_leaf_finder_t, solution_term_reader_t, solution_normalizer_t, node_depth_counter_t, pud_solution_store,
        pud_frontier> {
        using pud_solution_taker<
            solved_leaf_finder_t, solution_term_reader_t, solution_normalizer_t, node_depth_counter_t,
            pud_solution_store, pud_frontier>::pud_solution_taker;
    };
    struct search_stepper_t : pud_search_stepper<
        pud_solution_store, pud_pending_conflict_store, solution_taker_t, blocked_refuter_t, driver_t,
        query_lister_t> {
        using pud_search_stepper<
            pud_solution_store, pud_pending_conflict_store, solution_taker_t, blocked_refuter_t,
            driver_t, query_lister_t>::pud_search_stepper;
    };
    struct module_loader_t : pud_module_loader<
        pud_module, functor_linker_t, module_registry_t, focus_seeder_t, module_activator_t, pud_frontier,
        pud_pending_conflict_store> {
        using pud_module_loader<
            pud_module, functor_linker_t, module_registry_t, focus_seeder_t, module_activator_t, pud_frontier,
            pud_pending_conflict_store>::pud_module_loader;
    };

    pud_lodestar_manifest(
        expr_pool& expr_pool,
        pud_global_node_id focus_axiom,
        size_t max_unfolds,
        size_t non_conjecture_unfolds_per_conjecture_unfold,
        size_t max_node_depth,
        size_t max_open_goals);

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
    witness_search_recorder_t              witness_search_recorder_;
    mhcs_t                     mhcs_;
    candidate_witness_index_t          candidate_witness_index_;
    mhq_t                      mhq_;

    pud_active_states          active_states_;
    pud_query_holders          query_holders_;
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

    pud_focus_set              focus_set_;
    pud_solution_store         solution_store_;
    pud_pending_conflict_store pending_conflict_store_;
    pud_frontier               frontier_;

    focus_seeder_t                   focus_seeder_;
    focus_extender_t                 focus_extender_;
    node_depth_counter_t             node_depth_counter_;
    unfold_allower_t                 unfold_allower_;
    query_lister_t                   query_lister_;
    ring_builder_t                   ring_builder_;
    pud_lodestar_round_robin         non_conjecture_round_robin_;
    pud_lodestar_round_robin         conjecture_round_robin_;
    unfold_applier_t                 unfold_applier_;
    unit_propagator_t                unit_propagator_;
    blocked_refuter_t                blocked_refuter_;
    driver_t                         driver_;
    solved_leaf_finder_t             solved_leaf_finder_;
    solution_term_reader_t           solution_term_reader_;
    normalizer_factory_t             normalizer_factory_;
    solution_normalizer_t            solution_normalizer_;
    solution_taker_t                 solution_taker_;
    search_stepper_t                 search_stepper_;
    module_loader_t               module_loader_;
};

#endif

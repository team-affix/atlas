#ifndef PUD_BURROW_MANIFEST_HPP
#define PUD_BURROW_MANIFEST_HPP

#include <cstddef>
#include <cstdint>
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/globalizer.hpp"
#include "infrastructure/pud_axiom_focus_test.hpp"
#include "infrastructure/pud_bind_map_factory.hpp"
#include "infrastructure/pud_binder.hpp"
#include "infrastructure/pud_cache_trimming_step.hpp"
#include "infrastructure/pud_child_goal_list_builder.hpp"
#include "infrastructure/pud_child_scan_refutation_propagator.hpp"
#include "infrastructure/pud_child_shuffler.hpp"
#include "infrastructure/pud_cycle_detector_factory.hpp"
#include "infrastructure/pud_demoduler.hpp"
#include "infrastructure/pud_eager_goal_querier.hpp"
#include "infrastructure/pud_eager_leaf_evaluator.hpp"
#include "infrastructure/pud_evaluation_resolver.hpp"
#include "infrastructure/pud_focus_leaf_scanner.hpp"
#include "infrastructure/pud_focus_leaf_tally.hpp"
#include "infrastructure/pud_functor_linker.hpp"
#include "infrastructure/pud_functor_relinker.hpp"
#include "infrastructure/pud_inference_builder.hpp"
#include "infrastructure/pud_lazy_activity_mover.hpp"
#include "infrastructure/pud_lazy_focus_frontier.hpp"
#include "infrastructure/pud_leaf_evaluation_store.hpp"
#include "infrastructure/pud_leaf_refuter.hpp"
#include "infrastructure/pud_leaf_round_robin.hpp"
#include "infrastructure/pud_link_table.hpp"
#include "infrastructure/pud_module_trim_list.hpp"
#include "infrastructure/pud_paged_module_assembly.hpp"
#include "infrastructure/pud_module_loader.hpp"
#include "infrastructure/pud_module_registry.hpp"
#include "infrastructure/pud_node_state_cache.hpp"
#include "infrastructure/pud_normalizer_factory.hpp"
#include "infrastructure/pud_pending_conflict_store.hpp"
#include "infrastructure/pud_replayer.hpp"
#include "infrastructure/pud_resumable_candidate_specializer.hpp"
#include "infrastructure/pud_resumable_witness_search.hpp"
#include "infrastructure/pud_root_goal_list_builder.hpp"
#include "infrastructure/pud_search_driver.hpp"
#include "infrastructure/pud_searcher.hpp"
#include "infrastructure/pud_smallest_goal_chooser.hpp"
#include "infrastructure/pud_solution_normalizer.hpp"
#include "infrastructure/pud_solution_store.hpp"
#include "infrastructure/pud_solution_taker.hpp"
#include "infrastructure/pud_solution_term_reader.hpp"
#include "infrastructure/pud_solved_leaf_finder.hpp"
#include "infrastructure/pud_state_axioms.hpp"
#include "infrastructure/pud_state_bindings.hpp"
#include "infrastructure/pud_state_depths.hpp"
#include "infrastructure/pud_state_evicting_deactivator.hpp"
#include "infrastructure/pud_state_goals.hpp"
#include "infrastructure/pud_state_lvcs.hpp"
#include "infrastructure/pud_stored_solution_dropper.hpp"
#include "infrastructure/pud_symbol_table.hpp"
#include "infrastructure/pud_term_exporter_factory.hpp"
#include "infrastructure/pud_unfold_allower.hpp"
#include "infrastructure/pud_unfold_applier.hpp"
#include "infrastructure/pud_unfold_depth_recorder.hpp"
#include "infrastructure/pud_unfoldable_leaf_filter.hpp"
#include "infrastructure/pud_unfolder.hpp"
#include "infrastructure/pud_unifier_factory.hpp"
#include "infrastructure/pud_unit_goal_first_chooser.hpp"
#include "infrastructure/pud_witness_cascade_driver.hpp"
#include "value_objects/pud_global_node_id.hpp"

// Burrow: Forager's search over lazily worked-out node state and modules paged from disk. No node keeps goals,
// bindings or any other field: a node's state is folded from its path when read and kept in a bounded cache, and each
// module's decoded nodes are kept in another; both are trimmed after every step. Loading replays nothing; it only
// walks the focus tree's ids for its live leaves.
struct pud_burrow_manifest {
    using module_t = pud_paged_module_assembly::module_t;
    struct bind_map_factory_t : pud_bind_map_factory {
        using pud_bind_map_factory::pud_bind_map_factory;
    };
    struct module_registry_t : pud_module_registry<module_t> {
        using pud_module_registry<module_t>::pud_module_registry;
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
        using pud_term_exporter_factory<
            globalizer, expr_pool, expr_pool, functor_relinker_t>::pud_term_exporter_factory;
    };
    struct binder_t : pud_binder<bind_map_factory_t, module_registry_t> {
        using pud_binder<bind_map_factory_t, module_registry_t>::pud_binder;
    };
    struct node_states_t : pud_node_state_cache<
        module_registry_t, module_registry_t, module_registry_t, pud_root_goal_list_builder,
        pud_child_goal_list_builder, binder_t> {
        using pud_node_state_cache<
            module_registry_t, module_registry_t, module_registry_t, pud_root_goal_list_builder,
            pud_child_goal_list_builder, binder_t>::pud_node_state_cache;
    };
    struct node_goals_t : pud_state_goals<node_states_t> {
        using pud_state_goals<node_states_t>::pud_state_goals;
    };
    struct node_axioms_t : pud_state_axioms<node_states_t> {
        using pud_state_axioms<node_states_t>::pud_state_axioms;
    };
    struct node_depths_t : pud_state_depths<node_states_t> {
        using pud_state_depths<node_states_t>::pud_state_depths;
    };
    struct node_lvcs_t : pud_state_lvcs<node_states_t> {
        using pud_state_lvcs<node_states_t>::pud_state_lvcs;
    };
    struct node_bindings_t : pud_state_bindings<node_states_t> {
        using pud_state_bindings<node_states_t>::pud_state_bindings;
    };
    struct focus_test_t : pud_axiom_focus_test<module_registry_t> {
        using pud_axiom_focus_test<module_registry_t>::pud_axiom_focus_test;
    };
    struct refutation_propagator_t : pud_child_scan_refutation_propagator<
        module_registry_t, module_registry_t, module_registry_t, module_registry_t> {
        using pud_child_scan_refutation_propagator<
            module_registry_t, module_registry_t, module_registry_t,
            module_registry_t>::pud_child_scan_refutation_propagator;
    };
    struct replayer_t : pud_replayer<
        module_registry_t, module_registry_t, expr_pool, bind_map_factory_t, cycle_detector_factory_t,
        unifier_factory_t> {
        using pud_replayer<
            module_registry_t, module_registry_t, expr_pool, bind_map_factory_t, cycle_detector_factory_t,
            unifier_factory_t>::pud_replayer;
    };
    struct witness_search_t : pud_resumable_witness_search<
        replayer_t, module_registry_t, module_registry_t, module_registry_t> {
        using pud_resumable_witness_search<
            replayer_t, module_registry_t, module_registry_t, module_registry_t>::pud_resumable_witness_search;
    };
    struct candidate_specializer_t : pud_resumable_candidate_specializer<
        replayer_t, witness_search_t, witness_search_t, module_registry_t, module_registry_t, module_registry_t> {
        using pud_resumable_candidate_specializer<
            replayer_t, witness_search_t, witness_search_t, module_registry_t, module_registry_t,
            module_registry_t>::pud_resumable_candidate_specializer;
    };
    struct goal_querier_t : pud_eager_goal_querier<replayer_t, module_registry_t, candidate_specializer_t> {
        using pud_eager_goal_querier<replayer_t, module_registry_t, candidate_specializer_t>::pud_eager_goal_querier;
    };
    struct node_deactivator_t : pud_state_evicting_deactivator<node_states_t> {
        using pud_state_evicting_deactivator<node_states_t>::pud_state_evicting_deactivator;
    };
    struct leaf_refuter_t : pud_leaf_refuter<
        module_registry_t, refutation_propagator_t, node_deactivator_t, module_registry_t, module_registry_t> {
        using pud_leaf_refuter<
            module_registry_t, refutation_propagator_t, node_deactivator_t, module_registry_t,
            module_registry_t>::pud_leaf_refuter;
    };
    struct leaf_evaluator_t : pud_eager_leaf_evaluator<node_goals_t, node_bindings_t, node_lvcs_t, goal_querier_t> {
        using pud_eager_leaf_evaluator<
            node_goals_t, node_bindings_t, node_lvcs_t, goal_querier_t>::pud_eager_leaf_evaluator;
    };
    struct inference_builder_t : pud_inference_builder<
        node_goals_t, module_registry_t, module_registry_t, expr_pool, expr_pool, bind_map_factory_t,
        cycle_detector_factory_t, unifier_factory_t, term_exporter_factory_t> {
        using pud_inference_builder<
            node_goals_t, module_registry_t, module_registry_t, expr_pool, expr_pool, bind_map_factory_t,
            cycle_detector_factory_t, unifier_factory_t, term_exporter_factory_t>::pud_inference_builder;
    };
    struct resolver_t : pud_evaluation_resolver<
        pud_leaf_evaluation_store, node_bindings_t, node_lvcs_t, node_goals_t, inference_builder_t> {
        using pud_evaluation_resolver<
            pud_leaf_evaluation_store, node_bindings_t, node_lvcs_t, node_goals_t,
            inference_builder_t>::pud_evaluation_resolver;
    };
    struct unfolder_t : pud_unfolder<
        resolver_t, pud_child_shuffler, module_registry_t, pud_lazy_activity_mover, module_registry_t> {
        using pud_unfolder<
            resolver_t, pud_child_shuffler, module_registry_t, pud_lazy_activity_mover,
            module_registry_t>::pud_unfolder;
    };
    struct module_activator_t : pud_focus_leaf_scanner<module_registry_t, module_registry_t, module_registry_t> {
        using pud_focus_leaf_scanner<module_registry_t, module_registry_t, module_registry_t>::pud_focus_leaf_scanner;
    };
    struct unfold_allower_t : pud_unfold_allower<node_depths_t, node_goals_t> {
        using pud_unfold_allower<node_depths_t, node_goals_t>::pud_unfold_allower;
    };
    struct focus_frontier_t : pud_lazy_focus_frontier<focus_test_t, node_goals_t> {
        using pud_lazy_focus_frontier<focus_test_t, node_goals_t>::pud_lazy_focus_frontier;
    };
    struct focus_leaf_tally_t : pud_focus_leaf_tally<
        focus_frontier_t, focus_frontier_t, focus_frontier_t, focus_frontier_t, focus_frontier_t, focus_test_t,
        unfold_allower_t> {
        using pud_focus_leaf_tally<
            focus_frontier_t, focus_frontier_t, focus_frontier_t, focus_frontier_t, focus_frontier_t, focus_test_t,
            unfold_allower_t>::pud_focus_leaf_tally;
    };
    struct unfoldable_leaf_filter_t : pud_unfoldable_leaf_filter<
        module_registry_t, module_registry_t, node_goals_t, unfold_allower_t> {
        using pud_unfoldable_leaf_filter<
            module_registry_t, module_registry_t, node_goals_t, unfold_allower_t>::pud_unfoldable_leaf_filter;
    };
    struct focus_round_robin_t : pud_leaf_round_robin<focus_frontier_t, unfoldable_leaf_filter_t> {
        using pud_leaf_round_robin<focus_frontier_t, unfoldable_leaf_filter_t>::pud_leaf_round_robin;
    };
    struct unfold_applier_t : pud_unfold_applier<unfolder_t, focus_leaf_tally_t, focus_test_t> {
        using pud_unfold_applier<unfolder_t, focus_leaf_tally_t, focus_test_t>::pud_unfold_applier;
    };
    struct unfold_depth_recorder_t : pud_unfold_depth_recorder<node_depths_t, unfold_applier_t> {
        using pud_unfold_depth_recorder<node_depths_t, unfold_applier_t>::pud_unfold_depth_recorder;
    };
    struct smallest_goal_chooser_t : pud_smallest_goal_chooser<node_goals_t> {
        using pud_smallest_goal_chooser<node_goals_t>::pud_smallest_goal_chooser;
    };
    struct goal_chooser_t : pud_unit_goal_first_chooser<pud_leaf_evaluation_store, smallest_goal_chooser_t> {
        using pud_unit_goal_first_chooser<
            pud_leaf_evaluation_store, smallest_goal_chooser_t>::pud_unit_goal_first_chooser;
    };
    struct driver_t : pud_witness_cascade_driver<
        focus_round_robin_t, unfoldable_leaf_filter_t, leaf_evaluator_t, pud_leaf_evaluation_store, leaf_refuter_t,
        focus_leaf_tally_t, goal_chooser_t, unfold_depth_recorder_t> {
        using pud_witness_cascade_driver<
            focus_round_robin_t, unfoldable_leaf_filter_t, leaf_evaluator_t, pud_leaf_evaluation_store,
            leaf_refuter_t, focus_leaf_tally_t, goal_chooser_t, unfold_depth_recorder_t>::pud_witness_cascade_driver;
    };
    struct state_trimming_step_t : pud_cache_trimming_step<driver_t, node_states_t> {
        using pud_cache_trimming_step<driver_t, node_states_t>::pud_cache_trimming_step;
    };
    struct module_trim_list_t : pud_module_trim_list<module_t> {
        using pud_module_trim_list<module_t>::pud_module_trim_list;
    };
    struct trimming_step_t : pud_cache_trimming_step<state_trimming_step_t, module_trim_list_t> {
        using pud_cache_trimming_step<state_trimming_step_t, module_trim_list_t>::pud_cache_trimming_step;
    };
    struct solved_leaf_finder_t : pud_solved_leaf_finder<focus_frontier_t, module_registry_t> {
        using pud_solved_leaf_finder<focus_frontier_t, module_registry_t>::pud_solved_leaf_finder;
    };
    struct solution_term_reader_t : pud_solution_term_reader<node_axioms_t, module_registry_t, node_bindings_t> {
        using pud_solution_term_reader<node_axioms_t, module_registry_t, node_bindings_t>::pud_solution_term_reader;
    };
    struct normalizer_factory_t : pud_normalizer_factory<globalizer, expr_pool, expr_pool, demoduler_t> {
        using pud_normalizer_factory<globalizer, expr_pool, expr_pool, demoduler_t>::pud_normalizer_factory;
    };
    struct solution_normalizer_t : pud_solution_normalizer<demoduler_t, normalizer_factory_t> {
        using pud_solution_normalizer<demoduler_t, normalizer_factory_t>::pud_solution_normalizer;
    };
    struct solution_taker_t : pud_solution_taker<
        solved_leaf_finder_t, solution_term_reader_t, solution_normalizer_t, node_depths_t, pud_solution_store,
        focus_leaf_tally_t> {
        using pud_solution_taker<
            solved_leaf_finder_t, solution_term_reader_t, solution_normalizer_t, node_depths_t, pud_solution_store,
            focus_leaf_tally_t>::pud_solution_taker;
    };
    struct searcher_t : pud_searcher<
        pud_solution_store, pud_pending_conflict_store, solution_taker_t, trimming_step_t, focus_frontier_t> {
        using pud_searcher<
            pud_solution_store, pud_pending_conflict_store, solution_taker_t, trimming_step_t,
            focus_frontier_t>::pud_searcher;
    };
    struct stored_solution_dropper_t : pud_stored_solution_dropper<
        solved_leaf_finder_t, module_registry_t, focus_leaf_tally_t> {
        using pud_stored_solution_dropper<
            solved_leaf_finder_t, module_registry_t, focus_leaf_tally_t>::pud_stored_solution_dropper;
    };
    struct module_loader_t : pud_module_loader<
        module_t, functor_linker_t, module_registry_t, module_activator_t, focus_leaf_tally_t,
        pud_pending_conflict_store, stored_solution_dropper_t> {
        using pud_module_loader<
            module_t, functor_linker_t, module_registry_t, module_activator_t, focus_leaf_tally_t,
            pud_pending_conflict_store, stored_solution_dropper_t>::pud_module_loader;
    };

    pud_burrow_manifest(
        expr_pool& expr_pool,
        pud_global_node_id focus_axiom,
        size_t max_unfolds,
        size_t cascade_depth,
        size_t max_node_depth,
        size_t max_open_goals,
        size_t state_cache_bytes,
        bool report_stored_solutions);

    pud_symbol_table              symbol_table_;
    pud_link_table                link_table_;
    demoduler_t                   demoduler_;
    functor_linker_t              functor_linker_;
    globalizer                    globalizer_;
    cycle_detector_factory_t      cycle_detector_factory_;
    unifier_factory_t             unifier_factory_;
    bind_map_factory_t            bind_map_factory_;
    module_registry_t             modules_;
    functor_relinker_t            functor_relinker_;
    term_exporter_factory_t       term_exporter_factory_;
    pud_child_goal_list_builder   child_goal_list_builder_;
    pud_root_goal_list_builder    root_goal_list_builder_;
    binder_t                      binder_;
    node_states_t                 node_states_;
    node_goals_t                  node_goals_;
    node_axioms_t                 node_axioms_;
    node_depths_t                 node_depths_;
    node_lvcs_t                   node_lvcs_;
    node_bindings_t               node_bindings_;
    focus_test_t                  focus_test_;
    refutation_propagator_t       refutation_propagator_;

    replayer_t                    replayer_;
    witness_search_t              witness_search_;
    candidate_specializer_t       candidate_specializer_;
    goal_querier_t                goal_querier_;

    node_deactivator_t            node_deactivator_;
    leaf_refuter_t                leaf_refuter_;
    leaf_evaluator_t              leaf_evaluator_;
    pud_leaf_evaluation_store     evaluation_store_;
    inference_builder_t           inference_builder_;
    resolver_t                    resolver_;
    pud_lazy_activity_mover       activity_mover_;
    pud_child_shuffler            child_shuffler_;
    unfolder_t                    unfolder_;
    module_activator_t            module_activator_;

    pud_solution_store            solution_store_;
    pud_pending_conflict_store    pending_conflict_store_;

    unfold_allower_t              unfold_allower_;
    focus_frontier_t              focus_frontier_;
    focus_leaf_tally_t            focus_leaf_tally_;
    unfoldable_leaf_filter_t      unfoldable_leaf_filter_;
    focus_round_robin_t           focus_round_robin_;
    unfold_applier_t              unfold_applier_;
    unfold_depth_recorder_t       unfold_depth_recorder_;
    smallest_goal_chooser_t       smallest_goal_chooser_;
    goal_chooser_t                goal_chooser_;
    driver_t                      driver_;
    state_trimming_step_t         state_trimming_step_;
    module_trim_list_t            module_trim_list_;
    trimming_step_t               trimming_step_;
    solved_leaf_finder_t          solved_leaf_finder_;
    stored_solution_dropper_t     stored_solution_dropper_;
    solution_term_reader_t        solution_term_reader_;
    normalizer_factory_t          normalizer_factory_;
    solution_normalizer_t         solution_normalizer_;
    solution_taker_t              solution_taker_;
    searcher_t                    searcher_;
    pud_search_driver             search_driver_;
    module_loader_t               module_loader_;
};

#endif

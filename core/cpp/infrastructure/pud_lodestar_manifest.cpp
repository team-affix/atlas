#include "infrastructure/pud_lodestar_manifest.hpp"

pud_lodestar_manifest::pud_lodestar_manifest(
    expr_pool& expr_pool,
    pud_global_node_id focus_axiom,
    size_t max_unfolds,
    size_t non_conjecture_unfolds_per_conjecture_unfold,
    size_t max_node_depth,
    size_t max_open_goals,
    bool report_stored_solutions)
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
    , witness_search_recorder_(mhws_)
    , mhcs_(replayer_, witness_search_recorder_, witness_search_recorder_, witness_search_recorder_, witness_search_recorder_, witness_search_recorder_, modules_, modules_, modules_)
    , candidate_witness_index_(mhcs_, witness_search_recorder_)
    , mhq_(replayer_, modules_, candidate_witness_index_, candidate_witness_index_, candidate_witness_index_, candidate_witness_index_, unit_queries_)
    , active_states_()
    , query_holders_()
    , binder_(bind_map_factory_, modules_)
    , structure_watcher_(bind_map_factory_, modules_, expr_pool)
    , root_activator_(
          modules_, root_goal_list_builder_, node_goals_, node_axioms_, active_states_, structure_watcher_, mhq_,
          query_holders_)
    , child_activator_(
          active_states_, active_states_, modules_, node_goals_, node_axioms_, child_goal_list_builder_, node_goals_,
          node_axioms_, live_child_counts_, binder_, structure_watcher_, mhq_, mhq_, query_holders_,
          mhq_)
    , node_deactivator_(active_states_, active_states_, query_holders_, mhq_, mhq_, query_holders_)
    , leaf_refuter_(modules_, refutation_propagator_, node_deactivator_, modules_, modules_)
    , inference_builder_(
          node_goals_, modules_, modules_, expr_pool, expr_pool, bind_map_factory_, cycle_detector_factory_,
          unifier_factory_, term_exporter_factory_)
    , resolver_(active_states_, node_goals_, mhq_, mhcs_, inference_builder_)
    , activity_mover_(child_activator_, leaf_refuter_, node_deactivator_)
    , unfolder_(resolver_, modules_, activity_mover_, modules_)
    , module_activator_(modules_, modules_, modules_, root_activator_, leaf_refuter_, activity_mover_)
    , focus_set_()
    , solution_store_()
    , pending_conflict_store_()
    , frontier_()
    , focus_seeder_(focus_axiom, modules_, focus_set_, focus_set_)
    , focus_extender_(focus_set_, focus_set_)
    , node_depth_counter_(modules_)
    , unfold_allower_(node_depth_counter_, node_goals_, max_node_depth, max_open_goals)
    , query_lister_(frontier_, focus_set_, modules_, active_states_, node_goals_, mhq_)
    , ring_builder_(
          mhq_, candidate_witness_index_, witness_search_recorder_, focus_set_, frontier_, modules_, active_states_, mhq_, unfold_allower_)
    , non_conjecture_round_robin_()
    , conjecture_round_robin_()
    , unfold_applier_(unfolder_, frontier_, focus_extender_, focus_set_)
    , unit_propagator_(
          unit_queries_, query_holders_, query_holders_, mhq_, frontier_, modules_, unfold_allower_, unfold_applier_)
    , blocked_refuter_(query_lister_, leaf_refuter_, frontier_)
    , driver_(
          unit_propagator_, query_lister_, ring_builder_, non_conjecture_round_robin_, conjecture_round_robin_,
          unfold_applier_, non_conjecture_unfolds_per_conjecture_unfold)
    , solved_leaf_finder_(frontier_, focus_set_, modules_, node_goals_)
    , stored_solution_dropper_(solved_leaf_finder_, modules_, frontier_, report_stored_solutions)
    , solution_term_reader_(node_axioms_, modules_, active_states_)
    , normalizer_factory_(globalizer_, expr_pool, expr_pool, demoduler_)
    , solution_normalizer_(demoduler_, normalizer_factory_)
    , solution_taker_(
          solved_leaf_finder_, solution_term_reader_, solution_normalizer_, node_depth_counter_, solution_store_,
          frontier_)
    , search_stepper_(
          solution_store_, pending_conflict_store_, solution_taker_, blocked_refuter_, driver_, query_lister_,
          max_unfolds)
    , module_loader_(
          functor_linker_, modules_, focus_seeder_, module_activator_, frontier_, pending_conflict_store_,
          stored_solution_dropper_) {
    focus_set_.insert(focus_axiom);
}

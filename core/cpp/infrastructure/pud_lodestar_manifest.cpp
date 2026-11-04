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
    , node_depths_()
    , child_goal_list_builder_()
    , root_goal_list_builder_()
    , live_child_counts_()
    , refutation_propagator_(modules_, live_child_counts_, modules_)
    , unit_goal_finder_(query_introducers_, mhq_, node_queries_)
    , replayer_(modules_, modules_, expr_pool, bind_map_factory_, cycle_detector_factory_, unifier_factory_)
    , mhws_(replayer_, modules_, modules_, modules_)
    , witness_search_recorder_(mhws_)
    , mhcs_(replayer_, witness_search_recorder_, witness_search_recorder_, witness_search_recorder_, witness_search_recorder_, witness_search_recorder_, modules_, modules_, modules_)
    , candidate_witness_index_(mhcs_, witness_search_recorder_)
    , mhq_(replayer_, modules_, candidate_witness_index_, candidate_witness_index_, candidate_witness_index_, candidate_witness_index_, unit_goal_finder_)
    , node_lvcs_()
    , node_bindings_()
    , node_watches_()
    , query_introducers_()
    , introduction_recorder_(query_introducers_, query_introducers_)
    , query_remover_(introduction_recorder_, mhq_)
    , node_queries_()
    , query_leaf_finder_(query_introducers_, modules_, modules_, modules_, node_queries_)
    , focus_set_()
    , binder_(bind_map_factory_, modules_)
    , structure_watcher_(bind_map_factory_, modules_, expr_pool)
    , root_activator_(
          modules_, root_goal_list_builder_, node_goals_, node_axioms_, node_lvcs_, node_bindings_, node_watches_, node_queries_, structure_watcher_,
          mhq_, query_remover_, introduction_recorder_)
    , child_activator_(
          node_lvcs_, node_bindings_, node_watches_, node_queries_, node_lvcs_, node_bindings_, node_watches_, node_queries_,
          modules_, node_goals_, node_axioms_, child_goal_list_builder_, node_goals_,
          node_axioms_, live_child_counts_, binder_, structure_watcher_, mhq_, mhq_, query_remover_,
          introduction_recorder_, query_introducers_)
    , root_depth_recorder_(node_depths_, root_activator_)
    , child_depth_recorder_(node_depths_, node_depths_, child_activator_)
    , child_focus_recorder_(focus_set_, focus_set_, child_depth_recorder_)
    , node_deactivator_(
          node_queries_, node_watches_, node_bindings_, node_lvcs_, mhq_, query_introducers_, query_leaf_finder_)
    , leaf_refuter_(modules_, refutation_propagator_, node_deactivator_, modules_, modules_)
    , inference_builder_(
          node_goals_, modules_, modules_, expr_pool, expr_pool, bind_map_factory_, cycle_detector_factory_,
          unifier_factory_, term_exporter_factory_)
    , resolver_(node_queries_, node_bindings_, node_lvcs_, node_goals_, mhq_, mhcs_, inference_builder_)
    , activity_mover_(child_focus_recorder_, leaf_refuter_, node_deactivator_)
    , child_shuffler_()
    , unfolder_(resolver_, child_shuffler_, modules_, activity_mover_, modules_)
    , module_activator_(modules_, modules_, modules_, root_depth_recorder_, leaf_refuter_, activity_mover_)
    , solution_store_()
    , pending_conflict_store_()
    , frontier_()
    , unfold_allower_(node_depths_, node_goals_, max_node_depth, max_open_goals)
    , focus_frontier_(frontier_, frontier_, frontier_, frontier_, focus_set_, node_goals_)
    , focus_leaf_tally_(
          focus_frontier_, focus_frontier_, focus_frontier_, focus_frontier_, frontier_, focus_set_, unfold_allower_)
    , slot_index_(
          focus_leaf_tally_, focus_leaf_tally_, focus_leaf_tally_, focus_leaf_tally_, witness_search_recorder_,
          witness_search_recorder_, focus_frontier_, frontier_, modules_, focus_set_, node_goals_, mhq_,
          candidate_witness_index_, witness_search_recorder_, unfold_allower_, introduction_recorder_,
          introduction_recorder_)
    , non_conjecture_round_robin_(slot_index_, false)
    , conjecture_round_robin_(slot_index_, true)
    , unfold_applier_(unfolder_, slot_index_, focus_set_)
    , unfold_depth_recorder_(node_depths_, unfold_applier_)
    , smallest_goal_chooser_(node_goals_)
    , goal_chooser_(unit_goal_finder_, smallest_goal_chooser_)
    , driver_(
          non_conjecture_round_robin_, conjecture_round_robin_, goal_chooser_, unfold_depth_recorder_,
          non_conjecture_unfolds_per_conjecture_unfold)
    , solved_leaf_finder_(focus_frontier_, modules_)
    , stored_solution_dropper_(solved_leaf_finder_, modules_, slot_index_, report_stored_solutions)
    , solution_term_reader_(node_axioms_, modules_, node_bindings_)
    , normalizer_factory_(globalizer_, expr_pool, expr_pool, demoduler_)
    , solution_normalizer_(demoduler_, normalizer_factory_)
    , solution_taker_(
          solved_leaf_finder_, solution_term_reader_, solution_normalizer_, node_depths_, solution_store_,
          slot_index_)
    , searcher_(
          solution_store_, pending_conflict_store_, solution_taker_, driver_, focus_frontier_, max_unfolds)
    , search_driver_(searcher_.search())
    , module_loader_(
          functor_linker_, modules_, module_activator_, slot_index_, pending_conflict_store_,
          stored_solution_dropper_) {
    focus_set_.insert(focus_axiom);
}

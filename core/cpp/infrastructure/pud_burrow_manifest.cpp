#include "infrastructure/pud_burrow_manifest.hpp"

pud_burrow_manifest::pud_burrow_manifest(
    expr_pool& expr_pool,
    pud_global_node_id focus_axiom,
    size_t max_unfolds,
    size_t cascade_depth,
    size_t max_node_depth,
    size_t max_open_goals,
    size_t state_cache_bytes,
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
    , child_goal_list_builder_()
    , root_goal_list_builder_()
    , binder_(bind_map_factory_, modules_)
    , node_states_(
          modules_, modules_, modules_, root_goal_list_builder_, child_goal_list_builder_, binder_, state_cache_bytes)
    , node_goals_(node_states_)
    , node_axioms_(node_states_)
    , node_depths_(node_states_)
    , node_lvcs_(node_states_)
    , node_bindings_(node_states_)
    , focus_test_(modules_, focus_axiom)
    , refutation_propagator_(modules_, modules_, modules_, modules_)
    , replayer_(modules_, modules_, expr_pool, bind_map_factory_, cycle_detector_factory_, unifier_factory_)
    , witness_search_(replayer_, modules_, modules_, modules_)
    , candidate_specializer_(replayer_, witness_search_, witness_search_, modules_, modules_, modules_)
    , goal_querier_(replayer_, modules_, candidate_specializer_)
    , node_deactivator_(node_states_)
    , leaf_refuter_(modules_, refutation_propagator_, node_deactivator_, modules_, modules_)
    , leaf_evaluator_(node_goals_, node_bindings_, node_lvcs_, goal_querier_)
    , evaluation_store_()
    , inference_builder_(
          node_goals_, modules_, modules_, expr_pool, expr_pool, bind_map_factory_, cycle_detector_factory_,
          unifier_factory_, term_exporter_factory_)
    , resolver_(evaluation_store_, node_bindings_, node_lvcs_, node_goals_, inference_builder_)
    , activity_mover_()
    , child_shuffler_()
    , unfolder_(resolver_, child_shuffler_, modules_, activity_mover_, modules_)
    , module_activator_(modules_, modules_, modules_, focus_axiom)
    , solution_store_()
    , pending_conflict_store_()
    , unfold_allower_(node_depths_, node_goals_, max_node_depth, max_open_goals)
    , focus_frontier_(focus_test_, node_goals_)
    , focus_leaf_tally_(
          focus_frontier_, focus_frontier_, focus_frontier_, focus_frontier_, focus_frontier_, focus_test_,
          unfold_allower_)
    , unfoldable_leaf_filter_(modules_, modules_, node_goals_, unfold_allower_)
    , focus_round_robin_(focus_frontier_, unfoldable_leaf_filter_)
    , unfold_applier_(unfolder_, focus_leaf_tally_, focus_test_)
    , unfold_depth_recorder_(node_depths_, unfold_applier_)
    , smallest_goal_chooser_(node_goals_)
    , goal_chooser_(evaluation_store_, smallest_goal_chooser_)
    , driver_(
          focus_round_robin_, unfoldable_leaf_filter_, leaf_evaluator_, evaluation_store_, leaf_refuter_,
          focus_leaf_tally_, goal_chooser_, unfold_depth_recorder_, cascade_depth)
    , trimming_step_(driver_, node_states_)
    , solved_leaf_finder_(focus_frontier_, modules_)
    , stored_solution_dropper_(solved_leaf_finder_, modules_, focus_leaf_tally_, report_stored_solutions)
    , solution_term_reader_(node_axioms_, modules_, node_bindings_)
    , normalizer_factory_(globalizer_, expr_pool, expr_pool, demoduler_)
    , solution_normalizer_(demoduler_, normalizer_factory_)
    , solution_taker_(
          solved_leaf_finder_, solution_term_reader_, solution_normalizer_, node_depths_, solution_store_,
          focus_leaf_tally_)
    , searcher_(
          solution_store_, pending_conflict_store_, solution_taker_, trimming_step_, focus_frontier_, max_unfolds)
    , search_driver_(searcher_.search())
    , module_loader_(
          functor_linker_, modules_, module_activator_, focus_leaf_tally_, pending_conflict_store_,
          stored_solution_dropper_) {}

#include "infrastructure/pud_lodestar_manifest.hpp"

pud_lodestar_manifest::pud_lodestar_manifest(
    expr_pool& expr_pool,
    pud_global_node_id focus_axiom,
    size_t max_unfolds,
    size_t non_conjecture_unfolds_per_conjecture_unfold,
    size_t max_node_depth)
    : symbol_table_()
    , link_table_()
    , functor_demoduler_(demoduler_, link_table_, expr_pool)
    , demoduler_(functor_demoduler_, expr_pool)
    , functor_linker_(symbol_table_, link_table_)
    , globalizer_()
    , cycle_detector_factory_(globalizer_, functor_demoduler_)
    , unifier_factory_(globalizer_, expr_pool, demoduler_)
    , bind_map_factory_()
    , modules_()
    , exporter_factory_(
          globalizer_, expr_pool, expr_pool, link_table_, link_table_, link_table_, symbol_table_, modules_,
          link_table_)
    , node_goals_()
    , node_axioms_()
    , live_child_counts_()
    , refutation_propagator_(modules_, live_child_counts_, modules_)
    , unit_queries_()
    , replayer_(modules_, modules_, expr_pool, bind_map_factory_, cycle_detector_factory_, unifier_factory_)
    , mhws_(replayer_, modules_, modules_, modules_)
    , witness_log_(mhws_, modules_)
    , mhcs_(replayer_, witness_log_, witness_log_, witness_log_, witness_log_, witness_log_, modules_, modules_, modules_)
    , candidate_watch_(mhcs_, witness_log_)
    , mhq_(replayer_, modules_, candidate_watch_, candidate_watch_, candidate_watch_, candidate_watch_, unit_queries_)
    , active_states_()
    , query_holders_()
    , binder_(bind_map_factory_, modules_)
    , structure_watcher_(bind_map_factory_, modules_, expr_pool)
    , root_activator_(
          modules_, initial_goal_opener_, node_goals_, node_axioms_, active_states_, structure_watcher_, mhq_,
          query_holders_)
    , child_activator_(
          active_states_, active_states_, modules_, node_goals_, node_axioms_, goal_resolver_, node_goals_,
          node_goals_, node_axioms_, live_child_counts_, binder_, structure_watcher_, mhq_, mhq_, query_holders_,
          mhq_)
    , node_deactivator_(active_states_, active_states_, query_holders_, mhq_, mhq_, query_holders_)
    , leaf_refuter_(modules_, refutation_propagator_, node_deactivator_, modules_, modules_)
    , acceptor_(
          node_goals_, modules_, modules_, expr_pool, expr_pool, bind_map_factory_, cycle_detector_factory_,
          unifier_factory_, exporter_factory_)
    , resolver_(active_states_, node_goals_, mhq_, mhcs_, acceptor_)
    , unfolder_(resolver_, modules_, child_activator_, leaf_refuter_, node_deactivator_, modules_)
    , module_activator_(
          modules_, modules_, modules_, root_activator_, child_activator_, leaf_refuter_, node_deactivator_)
    , focus_set_()
    , unfold_budget_store_()
    , solution_store_()
    , pending_conflict_store_()
    , frontier_()
    , focus_seeder_(focus_axiom, modules_, focus_set_, focus_set_)
    , focus_extender_(focus_set_, focus_set_)
    , node_depth_counter_(modules_)
    , unfold_allower_(node_depth_counter_, node_goals_, max_node_depth)
    , driver_(*this, frontier_, non_conjecture_unfolds_per_conjecture_unfold)
    , solved_leaf_finder_(frontier_, focus_set_, modules_, node_goals_)
    , solution_term_reader_(node_axioms_, modules_, active_states_)
    , solution_normalizer_(link_table_, expr_pool, expr_pool)
    , solution_taker_(
          solved_leaf_finder_, solution_term_reader_, solution_normalizer_, node_depth_counter_, solution_store_,
          frontier_)
    , search_stepper_(
          solution_store_, solution_store_, pending_conflict_store_, pending_conflict_store_, unfold_budget_store_,
          unfold_budget_store_, unfold_budget_store_, unfold_budget_store_, solution_taker_, driver_, driver_,
          driver_, max_unfolds)
    , module_registrar_(
          functor_linker_, modules_, focus_seeder_, module_activator_, frontier_, pending_conflict_store_,
          pending_conflict_store_) {
    focus_set_.insert(focus_axiom);
}

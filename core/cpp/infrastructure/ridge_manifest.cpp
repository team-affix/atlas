#include "infrastructure/ridge_manifest.hpp"

ridge_manifest::early_wiring::early_wiring(
    locator& loc, db& database, initial_goal_exprs& initial_goals)
    : trail_(),
      bind_map_(),
      bind_map_factory_(),
      unifier_factory_(),
      lineage_pool_(),
      rule_id_set_factory_(),
      ra_rule_id_set_factory_(),
      srt_active_goals_(),
      goal_exprs_(),
      goal_candidate_rules_(ra_rule_id_set_factory_),
      unit_goals_(),
      decision_memory_(),
      resolution_memory_(),
      candidate_translation_maps_() {
    loc.bind_as<i_push_trail_frame, i_pop_trail_frame, i_log_to_current_trail_frame>(trail_);
    loc.bind_as<i_bind_map, i_clear_bindings>(bind_map_);
    loc.bind_as<i_bind_map_factory>(bind_map_factory_);
    loc.bind_as<i_unifier_factory>(unifier_factory_);
    loc.bind_as<i_make_goal_lineage, i_make_resolution_lineage, i_pin_goal_lineage, i_pin_resolution_lineage, i_trim_unpinned_lineages, i_import_goal_lineage, i_import_resolution_lineage>(lineage_pool_);
    loc.bind_as<i_db_rule_id_set_factory>(rule_id_set_factory_);
    loc.bind_as<i_candidate_rule_id_set_factory>(ra_rule_id_set_factory_);
    loc.bind_as<i_insert_active_goal, i_erase_active_goal, i_is_active_goal,
        i_iterate_root_goals, i_iterate_child_goals,
        i_active_goals_size, i_check_active_goals_empty, i_clear_active_goals,
        i_srt_link_goal_batch_parent, i_srt_flush_goal_batch>(srt_active_goals_);
    loc.bind_as<i_get_goal_expr, i_set_goal_expr, i_unset_goal_expr, i_clear_goal_exprs>(goal_exprs_);
    loc.bind_as<i_get_goal_candidate_rule_ids, i_insert_goal_candidates, i_link_goal_candidate, i_unlink_goal_candidate, i_erase_goal_candidates, i_clear_goal_candidate_rule_ids>(goal_candidate_rules_);
    loc.bind_as<i_push_unit_goal, i_pop_unit_goal, i_clear_unit_goals>(unit_goals_);
    loc.bind_as<i_record_decision, i_clear_recorded_decisions, i_get_decision_count, i_derive_decision_lemma>(decision_memory_);
    loc.bind_as<i_record_resolution, i_clear_recorded_resolutions, i_get_resolution_count, i_derive_resolution_lemma>(resolution_memory_);
    loc.bind_as<i_get_candidate_translation_map, i_set_candidate_translation_map, i_unset_candidate_translation_map, i_clear_candidate_translation_maps>(candidate_translation_maps_);
    loc.bind_as<i_get_rule, i_get_goal_db_rule_ids>(database);
    loc.bind_as<i_get_initial_goal_count, i_get_initial_goal_expr>(initial_goals);
}

ridge_manifest::pool_wiring::pool_wiring(locator& loc, size_t initial_var_count)
    : expr_pool_(loc),
      var_sequencer_(loc, static_cast<uint32_t>(initial_var_count)),
      cdcl_sequencer_(loc),
      elimination_backlog_(loc) {
    loc.bind_as<i_make_functor, i_make_var, i_import_expr, i_get_expr_count>(expr_pool_);
    loc.bind_as<i_var_sequencer>(var_sequencer_);
    loc.bind_as<i_cdcl_sequencer>(cdcl_sequencer_);
    loc.bind_as<i_insert_backlogged_elimination, i_is_backlogged_elimination>(elimination_backlog_);
}

ridge_manifest::elim_wiring::elim_wiring(locator& loc)
    : cdcl_(loc), mhu_(loc) {
    loc.bind_as<i_learn_avoidance>(cdcl_);
    loc.bind_as<i_try_add_mhu_head, i_clear_mhu_heads>(mhu_);
    joint_.emplace(loc);
    loc.bind_as<i_elimination_generator>(*joint_);
}

ridge_manifest::core_wiring::core_wiring(locator& loc)
    : get_resolution_rule_(loc),
      copier_(loc),
      conflict_detector_(loc),
      unit_goal_detector_(loc),
      solution_detector_(loc) {
    loc.bind_as<i_get_resolution_rule>(get_resolution_rule_);
    loc.bind_as<i_copier>(copier_);
    loc.bind_as<i_conflict_detector>(conflict_detector_);
    loc.bind_as<i_detect_unit_goal>(unit_goal_detector_);
    loc.bind_as<i_solution_detector>(solution_detector_);
}

ridge_manifest::activator_wiring::activator_wiring(locator& loc)
    : goal_activator_(loc),
      goal_deactivator_(loc),
      candidate_activator_(loc),
      candidate_deactivator_(loc) {
    loc.bind_as<i_goal_activator>(goal_activator_);
    loc.bind_as<i_goal_deactivator>(goal_deactivator_);
    loc.bind_as<i_candidate_activator>(candidate_activator_);
    loc.bind_as<i_candidate_deactivator>(candidate_deactivator_);
}

ridge_manifest::router_wiring::router_wiring(locator& loc)
    : elimination_router_(loc),
      get_unit_resolution_(loc),
      make_initial_goal_lineage_(loc) {
    loc.bind_as<i_elimination_router>(elimination_router_);
    loc.bind_as<i_get_unit_resolution>(get_unit_resolution_);
    loc.bind_as<i_make_initial_goal_lineage>(make_initial_goal_lineage_);
    initial_goal_activator_.emplace(loc);
    loc.bind_as<i_activate_initial_goal>(*initial_goal_activator_);
}

ridge_manifest::orchestration_wiring::orchestration_wiring(
    locator& loc, size_t max_resolutions, uint32_t random_seed, double exploration_constant)
    : goal_candidates_activator_(loc),
      goal_candidates_deactivator_(loc),
      rng_(random_seed),
      ridge_reward_(loc) {
    loc.bind_as<i_activate_goal_candidates>(goal_candidates_activator_);
    loc.bind_as<i_deactivate_goal_candidates>(goal_candidates_deactivator_);
    subgoals_activator_.emplace(loc);
    loc.bind_as<>(*subgoals_activator_);
    srt_subgoals_activator_.emplace(loc);
    loc.bind_as<i_activate_subgoals_and_candidates>(*srt_subgoals_activator_);
    initial_goals_activator_.emplace(loc);
    loc.bind_as<>(*initial_goals_activator_);
    srt_initial_goals_activator_.emplace(loc);
    loc.bind_as<i_activate_initial_goals_and_candidates>(*srt_initial_goals_activator_);
    set_up_sim_.emplace(loc);
    tear_down_sim_.emplace(loc);
    resolver_.emplace(loc);
    loc.bind_as<i_resolver>(*resolver_);
    loc.bind_as<i_compute_mcts_reward>(ridge_reward_);
    mcts_sim_.emplace(loc, *set_up_sim_, *tear_down_sim_, rng_, exploration_constant);
    loc.bind_as<i_set_up_sim, i_tear_down_sim, i_mcts_choose>(*mcts_sim_);
    mcts_decision_generator_.emplace(loc);
    loc.bind_as<i_generate_decision>(*mcts_decision_generator_);
    run_sim_.emplace(loc, max_resolutions);
    loc.bind_as<i_run_sim>(*run_sim_);
    solver_.emplace(loc);
    loc.bind_as<i_solve>(*solver_);
}

ridge_manifest::ridge_manifest(
    db& database,
    initial_goal_exprs& initial_goals,
    size_t initial_var_count,
    size_t max_resolutions,
    uint32_t random_seed,
    double exploration_constant)
    : loc_(),
      database_(database),
      initial_goals_(initial_goals),
      max_resolutions_(max_resolutions),
      early_(loc_, database_, initial_goals_),
      pools_(loc_, initial_var_count),
      elims_(loc_),
      core_(loc_),
      activators_(loc_),
      routers_(loc_),
      orch_(loc_, max_resolutions, random_seed, exploration_constant),
      trail_(early_.trail_),
      bind_map_(early_.bind_map_),
      bind_map_factory_(early_.bind_map_factory_),
      unifier_factory_(early_.unifier_factory_),
      expr_pool_(pools_.expr_pool_),
      var_sequencer_(pools_.var_sequencer_),
      cdcl_sequencer_(pools_.cdcl_sequencer_),
      lineage_pool_(early_.lineage_pool_),
      srt_active_goals_(early_.srt_active_goals_),
      goal_exprs_(early_.goal_exprs_),
      goal_candidate_rules_(early_.goal_candidate_rules_),
      unit_goals_(early_.unit_goals_),
      decision_memory_(early_.decision_memory_),
      resolution_memory_(early_.resolution_memory_),
      elimination_backlog_(pools_.elimination_backlog_),
      cdcl_(elims_.cdcl_),
      mhu_(elims_.mhu_),
      joint_(*elims_.joint_),
      candidate_translation_maps_(early_.candidate_translation_maps_),
      get_resolution_rule_(core_.get_resolution_rule_),
      copier_(core_.copier_),
      conflict_detector_(core_.conflict_detector_),
      unit_goal_detector_(core_.unit_goal_detector_),
      solution_detector_(core_.solution_detector_),
      goal_activator_(activators_.goal_activator_),
      goal_deactivator_(activators_.goal_deactivator_),
      candidate_activator_(activators_.candidate_activator_),
      candidate_deactivator_(activators_.candidate_deactivator_),
      elimination_router_(routers_.elimination_router_),
      get_unit_resolution_(routers_.get_unit_resolution_),
      make_initial_goal_lineage_(routers_.make_initial_goal_lineage_),
      initial_goal_activator_(*routers_.initial_goal_activator_),
      rng_(orch_.rng_),
      mcts_decision_generator_(*orch_.mcts_decision_generator_),
      resolver_(*orch_.resolver_),
      set_up_sim_(*orch_.set_up_sim_),
      tear_down_sim_(*orch_.tear_down_sim_),
      run_sim_(*orch_.run_sim_),
      ridge_reward_(orch_.ridge_reward_),
      mcts_sim_(*orch_.mcts_sim_),
      solver_(*orch_.solver_) {}

#ifndef DBUCT_QUELL_MANIFEST_HPP
#define DBUCT_QUELL_MANIFEST_HPP

#include <cstddef>
#include <cstdint>
#include <random>
#include <unordered_map>
#include <vector>

#include "dbuct.hpp"
#include "dispatches_table.hpp"
#include "linear_batch_increment.hpp"
#include "random_rollout.hpp"
#include "value_table.hpp"
#include "visits_table.hpp"

#include "infrastructure/candidate_activator.hpp"
#include "infrastructure/candidate_deactivator.hpp"
#include "infrastructure/conflict_detector.hpp"
#include "infrastructure/db.hpp"
#include "infrastructure/elimination_router.hpp"
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/get_resolution_rule.hpp"
#include "infrastructure/get_unit_resolution.hpp"
#include "infrastructure/globalizer.hpp"
#include "infrastructure/goal_activator.hpp"
#include "infrastructure/goal_candidates_activator.hpp"
#include "infrastructure/goal_candidates_deactivator.hpp"
#include "infrastructure/goal_work_function.hpp"
#include "infrastructure/quell_goal_activator.hpp"
#include "infrastructure/quell_goal_deactivator.hpp"
#include "infrastructure/quell_initial_goal_activator.hpp"
#include "infrastructure/quell_resolver.hpp"
#include "infrastructure/quell_reward.hpp"
#include "infrastructure/initial_goal_activator.hpp"
#include "infrastructure/initial_goal_exprs.hpp"
#include "infrastructure/initial_goals_activator.hpp"
#include "infrastructure/lineage_pool.hpp"
#include "infrastructure/make_initial_goal_lineage.hpp"
#include "infrastructure/mcts_decision_generator.hpp"
#include "infrastructure/ra_rule_id_set_factory.hpp"
#include "infrastructure/resolver.hpp"
#include "infrastructure/run_sim.hpp"
#include "uniform_exploration_constant.hpp"
#include "uniform_value_delta.hpp"
#include "infrastructure/solution_detector.hpp"
#include "infrastructure/srt_goal_deactivator.hpp"
#include "infrastructure/srt_initial_goals_activator.hpp"
#include "infrastructure/srt_subgoals_activator.hpp"
#include "infrastructure/subgoals_activator.hpp"
#include "infrastructure/unit_goal_detector.hpp"
#include "infrastructure/unifier.hpp"
#include "infrastructure/unifier_factory.hpp"

#include "infrastructure/dbuct_avoidance_unit_boundary.hpp"
#include "infrastructure/dbuct_bind_map.hpp"
#include "infrastructure/dbuct_bind_map_factory.hpp"
#include "infrastructure/pool_allocator.hpp"
#include "infrastructure/dbuct_candidate_frame_offsets.hpp"
#include "infrastructure/dbuct_cdcl_elimination_generator.hpp"
#include "infrastructure/dbuct_chosen_goal_candidates.hpp"
#include "infrastructure/dbuct_decision_memory.hpp"
#include "infrastructure/dbuct_elimination_backlog.hpp"
#include "infrastructure/dbuct_frame_bump_allocator.hpp"
#include "infrastructure/dbuct_frame_hub.hpp"
#include "infrastructure/solver_frame_depth_tracker.hpp"
#include "infrastructure/dbuct_frontier_ready.hpp"
#include "infrastructure/dbuct_goal_candidate_rules.hpp"
#include "infrastructure/dbuct_goal_depths.hpp"
#include "infrastructure/dbuct_goal_exprs.hpp"
#include "infrastructure/dbuct_goal_work_values.hpp"
#include "infrastructure/dbuct_remaining_work.hpp"
#include "infrastructure/querier.hpp"
#include "infrastructure/dbuct_quell_frame_hub.hpp"
#include "infrastructure/dbuct_joint_elimination_generator.hpp"
#include "infrastructure/dbuct_mhu_elimination_generator.hpp"
#include "infrastructure/dbuct_nearest_decision.hpp"
#include "infrastructure/dbuct_resolution_memory.hpp"
#include "infrastructure/dbuct_resolution_recorder.hpp"
#include "infrastructure/tree_walker.hpp"
#include "infrastructure/mcts_root_tree_node.hpp"
#include "infrastructure/check_mcts_choice_is_rule_choice.hpp"
#include "infrastructure/dbuct_sim.hpp"
#include "infrastructure/dbuct_quell_terminate_sim.hpp"
#include "value_objects/mcts_choice.hpp"
#include "value_objects/mcts_tree_node_id.hpp"
#include "infrastructure/dbuct_solver.hpp"
#include "infrastructure/dbuct_srt_active_goals.hpp"
#include "infrastructure/dbuct_unit_goals.hpp"
#include "infrastructure/normalizer.hpp"
#include "infrastructure/solver_driver.hpp"

struct dbuct_quell_manifest {
    using bind_map_t                = dbuct_bind_map<globalizer>;
    using bind_map_factory_t        = dbuct_bind_map_factory<globalizer>;
    using local_bind_map_pool_t = pool_allocator<bind_map_t>;
    using goal_exprs_t              = dbuct_goal_exprs;
    using goal_candidate_rules_t    = dbuct_goal_candidate_rules;
    using srt_active_goals_t        = dbuct_srt_active_goals;
    using unit_goals_t              = dbuct_unit_goals;
    using decision_memory_t         = dbuct_decision_memory;
    using resolution_memory_t       = dbuct_resolution_memory;
    using candidate_frame_offsets_t = dbuct_candidate_frame_offsets;
    using chosen_goal_candidates_t  = dbuct_chosen_goal_candidates;
    using frame_bump_allocator_t    = dbuct_frame_bump_allocator;
    using elimination_backlog_t     = dbuct_elimination_backlog;
    using nearest_decision_t        = dbuct_nearest_decision;
    using goal_depths_t             = dbuct_goal_depths;
    using goal_work_values_t        = dbuct_goal_work_values;
    using remaining_work_t          = dbuct_remaining_work;
    using unifier_factory_t = unifier_factory<globalizer, bind_map_t>;

    using tree_walker_t            = tree_walker;
    using dbuct_choices_t          = std::vector<mcts_choice>;
    using dbuct_visits_table_t     = monte_carlo::visits_table<mcts_tree_node_id, std::unordered_map>;
    using dbuct_value_table_t      = monte_carlo::value_table<mcts_tree_node_id, double, std::unordered_map>;
    using dbuct_dispatches_table_t = monte_carlo::dispatches_table<mcts_tree_node_id, std::unordered_map>;
    using dbuct_rollout_t          = monte_carlo::random_rollout<mcts_choice, std::mt19937,
                                              dbuct_choices_t, dbuct_choices_t>;
    using dbuct_batch_t            = monte_carlo::linear_batch_increment;
    using value_delta_t            = monte_carlo::uniform_value_delta<double>;
    using exploration_constant_t   = monte_carlo::uniform_exploration_constant<double>;
    using dbuct_t                  = monte_carlo::dbuct<
                                         mcts_tree_node_id, mcts_choice, double,
                                         dbuct_visits_table_t, dbuct_value_table_t,
                                         dbuct_visits_table_t, dbuct_value_table_t,
                                         dbuct_dispatches_table_t, dbuct_dispatches_table_t,
                                         dbuct_batch_t, tree_walker_t,
                                         dbuct_choices_t, dbuct_choices_t, dbuct_rollout_t,
                                         value_delta_t, exploration_constant_t>;

    using avoidance_unit_boundary_t = dbuct_avoidance_unit_boundary<
        nearest_decision_t, dbuct_t>;
    using solver_frame_depth_tracker_t = solver_frame_depth_tracker;
    using cdcl_t  = dbuct_cdcl_elimination_generator<
                    chosen_goal_candidates_t, avoidance_unit_boundary_t, decision_memory_t,
                    avoidance_unit_boundary_t, avoidance_unit_boundary_t,
                    avoidance_unit_boundary_t>;
    using mhu_t   = dbuct_mhu_elimination_generator<
                    bind_map_t, bind_map_t, bind_map_t,
                    local_bind_map_pool_t, local_bind_map_pool_t, local_bind_map_pool_t,
                    bind_map_factory_t,
                    unifier<globalizer, bind_map_t>, unifier_factory_t, lineage_pool,
                    expr_pool, goal_candidate_rules_t>;
    using hub_t   = dbuct_frame_hub<
                    solver_frame_depth_tracker_t, solver_frame_depth_tracker_t,
                    goal_exprs_t, goal_exprs_t,
                    goal_candidate_rules_t, goal_candidate_rules_t,
                    chosen_goal_candidates_t, chosen_goal_candidates_t,
                    decision_memory_t, decision_memory_t,
                    resolution_memory_t, resolution_memory_t,
                    unit_goals_t, unit_goals_t,
                    candidate_frame_offsets_t, candidate_frame_offsets_t,
                    frame_bump_allocator_t, frame_bump_allocator_t,
                    nearest_decision_t, nearest_decision_t,
                    elimination_backlog_t, elimination_backlog_t,
                    avoidance_unit_boundary_t, avoidance_unit_boundary_t,
                    srt_active_goals_t, srt_active_goals_t,
                    bind_map_t, bind_map_t,
                    mhu_t, mhu_t,
                    cdcl_t, cdcl_t>;
    using quell_hub_t = dbuct_quell_frame_hub<
                    hub_t, hub_t,
                    goal_depths_t, goal_depths_t,
                    goal_work_values_t, goal_work_values_t,
                    remaining_work_t, remaining_work_t>;
    using dbuct_joint_t = dbuct_joint_elimination_generator<cdcl_t, mhu_t>;
    using resolution_recorder_t = dbuct_resolution_recorder<decision_memory_t, resolution_memory_t,
                                nearest_decision_t, nearest_decision_t, avoidance_unit_boundary_t>;

    using get_resolution_rule_t         = get_resolution_rule<db>;
    using conflict_detector_t           = conflict_detector<goal_candidate_rules_t>;
    using unit_goal_detector_t          = unit_goal_detector<goal_candidate_rules_t>;
    using solution_detector_t           = solution_detector<srt_active_goals_t>;
    using goal_activator_t              = goal_activator<goal_exprs_t, goal_candidate_rules_t,
                                          srt_active_goals_t, candidate_frame_offsets_t, get_resolution_rule_t>;
    using srt_goal_deactivator_t        = srt_goal_deactivator<goal_exprs_t, goal_candidate_rules_t>;
    using candidate_deactivator_t       = candidate_deactivator<candidate_frame_offsets_t, goal_candidate_rules_t>;
    using candidate_activator_t         = candidate_activator<frame_bump_allocator_t, candidate_frame_offsets_t,
                                          mhu_t, elimination_backlog_t, goal_exprs_t, db, goal_candidate_rules_t>;
    using elimination_router_t          = elimination_router<goal_candidate_rules_t, srt_active_goals_t,
                                          elimination_backlog_t, candidate_deactivator_t>;
    using get_unit_resolution_t         = get_unit_resolution<goal_candidate_rules_t, lineage_pool>;
    using make_initial_goal_lineage_t   = make_initial_goal_lineage<lineage_pool>;
    using initial_goal_activator_t      = initial_goal_activator<initial_goal_exprs,
                                          make_initial_goal_lineage_t, goal_exprs_t, goal_candidate_rules_t, srt_active_goals_t>;
    using goal_candidates_deactivator_t = goal_candidates_deactivator<goal_candidate_rules_t,
                                          lineage_pool, candidate_deactivator_t>;
    using querier_t                     = querier<goal_exprs_t, db, db>;
    using goal_candidates_activator_t   = goal_candidates_activator<querier_t, lineage_pool,
                                          candidate_activator_t, conflict_detector_t,
                                          unit_goal_detector_t, unit_goals_t>;
    using quell_goal_activator_t      = quell_goal_activator<goal_activator_t, goal_depths_t, goal_depths_t,
                                          goal_work_values_t, goal_work_function, remaining_work_t>;
    using quell_goal_deactivator_t    = quell_goal_deactivator<srt_goal_deactivator_t, goal_depths_t, goal_work_values_t>;
    using quell_initial_goal_activator_t = quell_initial_goal_activator<initial_goal_activator_t,
                                          make_initial_goal_lineage_t, goal_depths_t, goal_work_values_t,
                                          goal_work_function, remaining_work_t>;
    using subgoals_activator_t          = subgoals_activator<lineage_pool, quell_goal_activator_t,
                                          db, goal_candidates_activator_t>;
    using srt_subgoals_activator_t      = srt_subgoals_activator<srt_active_goals_t, srt_active_goals_t, subgoals_activator_t>;
    using initial_goals_activator_t     = initial_goals_activator<initial_goal_exprs,
                                          quell_initial_goal_activator_t, make_initial_goal_lineage_t, goal_candidates_activator_t>;
    using srt_initial_goals_activator_t = srt_initial_goals_activator<srt_active_goals_t, initial_goals_activator_t>;
    using resolver_t                    = resolver<quell_goal_deactivator_t, srt_subgoals_activator_t,
                                          goal_candidates_deactivator_t, chosen_goal_candidates_t>;
    using quell_resolver_t            = quell_resolver<resolver_t, goal_work_values_t, remaining_work_t>;
    using quell_reward_t              = quell_reward<remaining_work_t>;

    using dbuct_sim_t              = dbuct_sim<mcts_choice,
                                          quell_hub_t, quell_hub_t,
                                          solver_frame_depth_tracker_t,
                                          decision_memory_t,
                                          avoidance_unit_boundary_t, avoidance_unit_boundary_t,
                                          dbuct_t, dbuct_t, dbuct_t, dbuct_t,
                                          dbuct_t,
                                          check_mcts_choice_is_rule_choice>;
    using dbuct_quell_terminate_sim_t = dbuct_quell_terminate_sim<
                                          quell_reward_t, value_delta_t, dbuct_sim_t>;
    using mcts_decision_generator_t     = mcts_decision_generator<lineage_pool,
                                          srt_active_goals_t, srt_active_goals_t, srt_active_goals_t,
                                          dbuct_sim_t, goal_candidate_rules_t>;
    using run_sim_t                     = run_sim<dbuct_frontier_ready, solution_detector_t, conflict_detector_t,
                                          unit_goal_detector_t, unit_goals_t, unit_goals_t, mcts_decision_generator_t,
                                          dbuct_joint_t, elimination_router_t, quell_resolver_t, get_unit_resolution_t,
                                          resolution_recorder_t, resolution_recorder_t, resolution_memory_t>;
    using solver_t                      = dbuct_solver<srt_initial_goals_activator_t, run_sim_t, decision_memory_t,
                                          dbuct_quell_terminate_sim_t, dbuct_sim_t, cdcl_t, mhu_t, elimination_router_t,
                                          conflict_detector_t, unit_goal_detector_t, unit_goals_t>;
    using normalizer_t                  = normalizer<globalizer, expr_pool, expr_pool, bind_map_t>;

    dbuct_quell_manifest(
        db& database,
        initial_goal_exprs& initial_goals,
        uint32_t initial_frame_offset,
        size_t max_resolutions,
        uint32_t random_seed,
        double exploration_constant,
        double work_decay_k,
        double work_decay_j,
        size_t grant_increment_interval);

    globalizer                    globalizer_;
    bind_map_t                    bind_map_;
    bind_map_factory_t            bind_map_factory_;
    local_bind_map_pool_t         local_bind_map_pool_;
    unifier_factory_t             unifier_factory_;
    lineage_pool                  lineage_pool_;
    ra_rule_id_set_factory        ra_rule_id_set_factory_;
    srt_active_goals_t            srt_active_goals_;
    goal_exprs_t                  goal_exprs_;
    querier_t                     querier_;
    goal_depths_t                 goal_depths_;
    goal_work_values_t            goal_work_values_;
    remaining_work_t              remaining_work_;
    goal_work_function               goal_work_function_;
    goal_candidate_rules_t        goal_candidate_rules_;
    unit_goals_t                  unit_goals_;
    decision_memory_t             decision_memory_;
    resolution_memory_t           resolution_memory_;
    candidate_frame_offsets_t     candidate_frame_offsets_;
    chosen_goal_candidates_t      chosen_goal_candidates_;
    expr_pool                     expr_pool_;
    frame_bump_allocator_t        frame_allocator_;
    elimination_backlog_t         elimination_backlog_;
    nearest_decision_t            nearest_decision_;
    std::mt19937                  rng_;
    tree_walker_t                 tree_walker_;
    dbuct_visits_table_t          dbuct_visits_table_;
    dbuct_value_table_t           dbuct_value_table_;
    dbuct_dispatches_table_t      dbuct_dispatches_table_;
    dbuct_batch_t                 dbuct_batch_;
    dbuct_rollout_t               dbuct_rollout_;
    value_delta_t                 value_delta_;
    mcts_root_tree_node           mcts_root_tree_node_;
    exploration_constant_t        exploration_constant_;
    dbuct_t                       dbuct_;
    avoidance_unit_boundary_t     avoidance_unit_boundary_;
    solver_frame_depth_tracker_t  solver_frame_depth_tracker_;
    cdcl_t                        cdcl_;
    mhu_t                         mhu_;
    hub_t                         hub_;
    quell_hub_t                   quell_hub_;
    dbuct_joint_t                 dbuct_joint_;
    resolution_recorder_t         resolution_recorder_;
    get_resolution_rule_t         get_resolution_rule_;
    conflict_detector_t           conflict_detector_;
    unit_goal_detector_t          unit_goal_detector_;
    solution_detector_t           solution_detector_;
    goal_activator_t              goal_activator_;
    quell_goal_activator_t        quell_goal_activator_;
    srt_goal_deactivator_t        srt_goal_deactivator_;
    quell_goal_deactivator_t      quell_goal_deactivator_;
    candidate_activator_t         candidate_activator_;
    candidate_deactivator_t       candidate_deactivator_;
    elimination_router_t          elimination_router_;
    get_unit_resolution_t         get_unit_resolution_;
    make_initial_goal_lineage_t   make_initial_goal_lineage_;
    initial_goal_activator_t      initial_goal_activator_;
    quell_initial_goal_activator_t quell_initial_goal_activator_;
    goal_candidates_deactivator_t goal_candidates_deactivator_;
    goal_candidates_activator_t   goal_candidates_activator_;
    subgoals_activator_t          subgoals_activator_;
    srt_subgoals_activator_t      srt_subgoals_activator_;
    initial_goals_activator_t     initial_goals_activator_;
    srt_initial_goals_activator_t srt_initial_goals_activator_;
    resolver_t                    resolver_;
    quell_resolver_t              quell_resolver_;
    quell_reward_t                quell_reward_;
    check_mcts_choice_is_rule_choice check_mcts_choice_is_rule_choice_;
    dbuct_sim_t                   dbuct_sim_;
    dbuct_quell_terminate_sim_t   dbuct_quell_terminate_sim_;
    mcts_decision_generator_t     mcts_decision_generator_;
    dbuct_frontier_ready          frontier_ready_;
    run_sim_t                     run_sim_;
    solver_t                      solver_;
    normalizer_t                  normalizer_;
    solver_driver                 driver_;
};

#endif

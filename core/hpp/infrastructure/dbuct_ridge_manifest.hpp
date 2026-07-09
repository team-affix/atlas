#ifndef DBUCT_RIDGE_MANIFEST_HPP
#define DBUCT_RIDGE_MANIFEST_HPP

#include <cstddef>
#include <cstdint>
#include <random>

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
#include "infrastructure/initial_goal_activator.hpp"
#include "infrastructure/initial_goal_exprs.hpp"
#include "infrastructure/initial_goals_activator.hpp"
#include "infrastructure/lineage_pool.hpp"
#include "infrastructure/make_initial_goal_lineage.hpp"
#include "infrastructure/mcts_decision_generator.hpp"
#include "infrastructure/ra_rule_id_set_factory.hpp"
#include "infrastructure/resolver.hpp"
#include "infrastructure/ridge_reward.hpp"
#include "infrastructure/run_sim.hpp"
#include "infrastructure/solution_detector.hpp"
#include "infrastructure/srt_goal_deactivator.hpp"
#include "infrastructure/srt_initial_goals_activator.hpp"
#include "infrastructure/srt_subgoals_activator.hpp"
#include "infrastructure/subgoals_activator.hpp"
#include "infrastructure/trail.hpp"
#include "infrastructure/unit_goal_detector.hpp"
#include "infrastructure/unifier.hpp"
#include "infrastructure/unifier_factory.hpp"

#include "infrastructure/dbuct_avoidance_unit_boundary.hpp"
#include "infrastructure/dbuct_bind_map.hpp"
#include "infrastructure/dbuct_bind_map_factory.hpp"
#include "infrastructure/dbuct_candidate_frame_offsets.hpp"
#include "infrastructure/dbuct_cdcl_elimination_generator.hpp"
#include "infrastructure/dbuct_chosen_goal_candidates.hpp"
#include "infrastructure/dbuct_decision_memory.hpp"
#include "infrastructure/dbuct_decision_router.hpp"
#include "infrastructure/dbuct_elimination_backlog.hpp"
#include "infrastructure/dbuct_frame_bump_allocator.hpp"
#include "infrastructure/dbuct_frame_count.hpp"
#include "infrastructure/dbuct_frontier_ready.hpp"
#include "infrastructure/dbuct_goal_candidate_rules.hpp"
#include "infrastructure/dbuct_goal_exprs.hpp"
#include "infrastructure/dbuct_joint_elimination_generator.hpp"
#include "infrastructure/dbuct_learn_reapply.hpp"
#include "infrastructure/dbuct_mhu_elimination_generator.hpp"
#include "infrastructure/dbuct_nearest_decision.hpp"
#include "infrastructure/dbuct_resolution_memory.hpp"
#include "infrastructure/dbuct_sim.hpp"
#include "infrastructure/dbuct_solver.hpp"
#include "infrastructure/dbuct_srt_active_goals.hpp"
#include "infrastructure/dbuct_unit_goals.hpp"

// ridge_dbuct — delayed-backtracking CHC solver. A copy of ridge_manifest that
// swaps the restarting MCTS stack for the DBUCT camping stack and every per-sim
// state struct for its trail-backed dbuct_* counterpart, reusing all
// solver-agnostic infrastructure instantiated over the dbuct_* types. The
// concrete trail satisfies the ILogTrailAction / IPushFrame / IPopFrame
// interfaces those dbuct_* structs are templated on.
struct dbuct_ridge_manifest {
    using bind_map_t                = dbuct_bind_map<globalizer, trail>;
    using bind_map_factory_t        = dbuct_bind_map_factory<globalizer, trail>;
    using goal_exprs_t              = dbuct_goal_exprs<trail>;
    using goal_candidate_rules_t    = dbuct_goal_candidate_rules<trail>;
    using srt_active_goals_t        = dbuct_srt_active_goals<trail>;
    using unit_goals_t             = dbuct_unit_goals<trail>;
    using decision_memory_t        = dbuct_decision_memory<trail>;
    using resolution_memory_t      = dbuct_resolution_memory<trail>;
    using candidate_frame_offsets_t = dbuct_candidate_frame_offsets<trail>;
    using chosen_goal_candidates_t  = dbuct_chosen_goal_candidates<trail>;
    using frame_bump_allocator_t    = dbuct_frame_bump_allocator<trail>;
    using elimination_backlog_t     = dbuct_elimination_backlog<trail>;
    using frame_count_t             = dbuct_frame_count;
    using nearest_decision_t        = dbuct_nearest_decision<trail>;
    using avoidance_unit_boundary_t = dbuct_avoidance_unit_boundary<nearest_decision_t, frame_count_t, trail>;

    using unifier_factory_t = unifier_factory<globalizer, bind_map_t>;
    using cdcl_t  = dbuct_cdcl_elimination_generator<
                    chosen_goal_candidates_t, avoidance_unit_boundary_t, decision_memory_t,
                    avoidance_unit_boundary_t, avoidance_unit_boundary_t>;
    using mhu_t   = dbuct_mhu_elimination_generator<
                    bind_map_t, bind_map_factory_t, unifier<globalizer, bind_map_t>,
                    unifier_factory_t, lineage_pool, expr_pool, goal_candidate_rules_t, trail, trail>;
    using dbuct_joint_t = dbuct_joint_elimination_generator<cdcl_t, mhu_t>;
    using decision_router_t = dbuct_decision_router<decision_memory_t, resolution_memory_t,
                              nearest_decision_t, avoidance_unit_boundary_t>;

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
    using goal_candidates_activator_t   = goal_candidates_activator<db, lineage_pool, candidate_activator_t,
                                          conflict_detector_t, unit_goal_detector_t, unit_goals_t>;
    using subgoals_activator_t          = subgoals_activator<lineage_pool, goal_activator_t,
                                          db, goal_candidates_activator_t>;
    using srt_subgoals_activator_t      = srt_subgoals_activator<srt_active_goals_t, subgoals_activator_t>;
    using initial_goals_activator_t     = initial_goals_activator<initial_goal_exprs,
                                          initial_goal_activator_t, make_initial_goal_lineage_t, goal_candidates_activator_t>;
    using srt_initial_goals_activator_t = srt_initial_goals_activator<srt_active_goals_t, initial_goals_activator_t>;
    using resolver_t                    = resolver<srt_goal_deactivator_t, srt_subgoals_activator_t,
                                          goal_candidates_deactivator_t, chosen_goal_candidates_t>;
    using ridge_reward_t                = ridge_reward<decision_memory_t>;

    using dbuct_sim_t                   = dbuct_sim<trail, lineage_pool, cdcl_t, frame_count_t>;
    using mcts_decision_generator_t     = mcts_decision_generator<lineage_pool, srt_active_goals_t,
                                          dbuct_sim_t, goal_candidate_rules_t>;
    using run_sim_t                     = run_sim<dbuct_frontier_ready, solution_detector_t, conflict_detector_t,
                                          unit_goal_detector_t, unit_goals_t, unit_goals_t, mcts_decision_generator_t,
                                          dbuct_joint_t, elimination_router_t, resolver_t, get_unit_resolution_t,
                                          decision_router_t, decision_router_t, resolution_memory_t>;
    using learn_reapply_t               = dbuct_learn_reapply<mhu_t, elimination_router_t, conflict_detector_t,
                                          unit_goal_detector_t, unit_goals_t>;
    using solver_t                      = dbuct_solver<srt_initial_goals_activator_t, run_sim_t, decision_memory_t,
                                          ridge_reward_t, dbuct_sim_t, cdcl_t, learn_reapply_t>;

    dbuct_ridge_manifest(
        db& database,
        initial_goal_exprs& initial_goals,
        uint32_t initial_frame_offset,
        size_t max_resolutions,
        uint32_t random_seed,
        double exploration_constant,
        size_t grant_increment_interval);

    globalizer                    globalizer_;
    trail                         trail_;
    bind_map_t                    bind_map_;
    bind_map_factory_t            bind_map_factory_;
    unifier_factory_t             unifier_factory_;
    lineage_pool                  lineage_pool_;
    ra_rule_id_set_factory        ra_rule_id_set_factory_;
    srt_active_goals_t            srt_active_goals_;
    goal_exprs_t                  goal_exprs_;
    goal_candidate_rules_t        goal_candidate_rules_;
    unit_goals_t                  unit_goals_;
    decision_memory_t             decision_memory_;
    resolution_memory_t           resolution_memory_;
    candidate_frame_offsets_t     candidate_frame_offsets_;
    chosen_goal_candidates_t      chosen_goal_candidates_;
    expr_pool                     expr_pool_;
    frame_bump_allocator_t        frame_allocator_;
    elimination_backlog_t         elimination_backlog_;
    frame_count_t                 frame_count_;
    nearest_decision_t            nearest_decision_;
    avoidance_unit_boundary_t     avoidance_unit_boundary_;
    cdcl_t                        cdcl_;
    mhu_t                         mhu_;
    dbuct_joint_t                 dbuct_joint_;
    decision_router_t             decision_router_;
    get_resolution_rule_t         get_resolution_rule_;
    conflict_detector_t           conflict_detector_;
    unit_goal_detector_t          unit_goal_detector_;
    solution_detector_t           solution_detector_;
    goal_activator_t              goal_activator_;
    srt_goal_deactivator_t        srt_goal_deactivator_;
    candidate_activator_t         candidate_activator_;
    candidate_deactivator_t       candidate_deactivator_;
    elimination_router_t          elimination_router_;
    get_unit_resolution_t         get_unit_resolution_;
    make_initial_goal_lineage_t   make_initial_goal_lineage_;
    initial_goal_activator_t      initial_goal_activator_;
    goal_candidates_deactivator_t goal_candidates_deactivator_;
    goal_candidates_activator_t   goal_candidates_activator_;
    subgoals_activator_t          subgoals_activator_;
    srt_subgoals_activator_t      srt_subgoals_activator_;
    initial_goals_activator_t     initial_goals_activator_;
    srt_initial_goals_activator_t srt_initial_goals_activator_;
    resolver_t                    resolver_;
    ridge_reward_t                ridge_reward_;
    std::mt19937                  rng_;
    dbuct_sim_t                   dbuct_sim_;
    mcts_decision_generator_t     mcts_decision_generator_;
    dbuct_frontier_ready          frontier_ready_;
    run_sim_t                     run_sim_;
    learn_reapply_t               learn_reapply_;
    solver_t                      solver_;
};

#endif

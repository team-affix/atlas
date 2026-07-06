#ifndef DBUCT_MANIFEST_HPP
#define DBUCT_MANIFEST_HPP

#include <cstddef>
#include <cstdint>
#include <random>

// Reused, solver-agnostic infrastructure (templated over the state structs).
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
#include "infrastructure/joint_elimination_generator.hpp"
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

// Delayed-backtracking (DBUCT) infrastructure.
#include "infrastructure/dbuct_bind_map.hpp"
#include "infrastructure/dbuct_bind_map_factory.hpp"
#include "infrastructure/dbuct_candidate_frame_offsets.hpp"
#include "infrastructure/dbuct_cdcl_elimination_generator.hpp"
#include "infrastructure/dbuct_checkpoint_stack.hpp"
#include "infrastructure/dbuct_chosen_goal_candidates.hpp"
#include "infrastructure/dbuct_decision_memory.hpp"
#include "infrastructure/dbuct_elimination_backlog.hpp"
#include "infrastructure/dbuct_frame_bump_allocator.hpp"
#include "infrastructure/dbuct_frontier_ready.hpp"
#include "infrastructure/dbuct_goal_candidate_rules.hpp"
#include "infrastructure/dbuct_goal_exprs.hpp"
#include "infrastructure/dbuct_learn_reapply.hpp"
#include "infrastructure/dbuct_mhu_elimination_generator.hpp"
#include "infrastructure/dbuct_resolution_memory.hpp"
#include "infrastructure/dbuct_sim.hpp"
#include "infrastructure/dbuct_solver.hpp"
#include "infrastructure/dbuct_srt_active_goals.hpp"
#include "infrastructure/dbuct_unit_goals.hpp"

// ridge_dbuct — delayed-backtracking CHC solver.
//
// This is a copy of ridge_manifest that swaps the restarting MCTS stack
// (mcts_sim / set_up_sim / tear_down_sim / solver) for the DBUCT camping stack
// (dbuct_sim / checkpoint_stack / dbuct_solver) and swaps every per-sim state
// struct for its backtrackable dbuct_* counterpart. All solver-agnostic
// infrastructure (activators, detectors, resolver, elimination router, decision
// generator, joint eliminator) is reused unchanged, instantiated over the
// dbuct_* state types.
struct dbuct_manifest {
    // The concrete trail satisfies the trail-capability interfaces the DBUCT
    // state structs are templated on: ILogTrailAction (log) for every camped
    // store, and IPushFrame/IPopFrame (push/pop) for the checkpoint translator.
    // Nothing below names the concrete trail type directly except these aliases.
    using bind_map_t                = dbuct_bind_map<trail>;
    using bind_map_factory_t        = dbuct_bind_map_factory<trail>;
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

    using unifier_factory_t = unifier_factory<bind_map_t>;
    using cdcl_t  = dbuct_cdcl_elimination_generator<chosen_goal_candidates_t>;
    using mhu_t   = dbuct_mhu_elimination_generator<
                    bind_map_t, bind_map_factory_t, unifier<bind_map_t>,
                    unifier_factory_t, lineage_pool, expr_pool, goal_candidate_rules_t, trail, trail>;
    using joint_t = joint_elimination_generator<cdcl_t, mhu_t>;

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

    using checkpoint_stack_t            = dbuct_checkpoint_stack<trail, trail>;
    using dbuct_sim_t                   = dbuct_sim<checkpoint_stack_t, lineage_pool>;
    using mcts_decision_generator_t     = mcts_decision_generator<lineage_pool, srt_active_goals_t,
                                          dbuct_sim_t, goal_candidate_rules_t>;
    using run_sim_t                     = run_sim<dbuct_frontier_ready, solution_detector_t, conflict_detector_t,
                                          unit_goal_detector_t, unit_goals_t, unit_goals_t, mcts_decision_generator_t,
                                          joint_t, elimination_router_t, resolver_t, get_unit_resolution_t,
                                          decision_memory_t, resolution_memory_t, resolution_memory_t>;
    using learn_reapply_t               = dbuct_learn_reapply<cdcl_t, elimination_router_t, conflict_detector_t,
                                          unit_goal_detector_t, unit_goals_t>;
    using solver_t                      = dbuct_solver<srt_initial_goals_activator_t, run_sim_t, decision_memory_t,
                                          decision_memory_t, ridge_reward_t, dbuct_sim_t, cdcl_t, learn_reapply_t>;

    dbuct_manifest(
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
    cdcl_t                        cdcl_;
    mhu_t                         mhu_;
    joint_t                       joint_;
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
    checkpoint_stack_t            checkpoints_;
    std::mt19937                  rng_;
    dbuct_sim_t                   dbuct_sim_;
    mcts_decision_generator_t     mcts_decision_generator_;
    dbuct_frontier_ready          frontier_ready_;
    run_sim_t                     run_sim_;
    learn_reapply_t               learn_reapply_;
    solver_t                      solver_;
};

inline dbuct_manifest::dbuct_manifest(
    db& database,
    initial_goal_exprs& initial_goals,
    uint32_t initial_frame_offset,
    size_t max_resolutions,
    uint32_t random_seed,
    double exploration_constant,
    size_t grant_increment_interval)
    : globalizer_(),
      trail_(),
      bind_map_(globalizer_, trail_),
      bind_map_factory_(globalizer_, trail_),
      unifier_factory_(globalizer_),
      lineage_pool_(),
      ra_rule_id_set_factory_(),
      srt_active_goals_(trail_),
      goal_exprs_(trail_),
      goal_candidate_rules_(trail_, ra_rule_id_set_factory_),
      unit_goals_(trail_),
      decision_memory_(trail_),
      resolution_memory_(trail_),
      candidate_frame_offsets_(trail_),
      chosen_goal_candidates_(trail_),
      expr_pool_(),
      frame_allocator_(trail_, initial_frame_offset),
      elimination_backlog_(trail_),
      cdcl_(chosen_goal_candidates_),
      mhu_(bind_map_, lineage_pool_, expr_pool_,
           bind_map_factory_, unifier_factory_, goal_candidate_rules_, trail_, trail_),
      joint_(cdcl_, mhu_),
      get_resolution_rule_(database),
      conflict_detector_(goal_candidate_rules_),
      unit_goal_detector_(goal_candidate_rules_),
      solution_detector_(srt_active_goals_),
      goal_activator_(goal_exprs_, goal_candidate_rules_, srt_active_goals_,
                      candidate_frame_offsets_, get_resolution_rule_),
      srt_goal_deactivator_(goal_exprs_, goal_candidate_rules_),
      candidate_activator_(frame_allocator_, candidate_frame_offsets_, mhu_,
                           elimination_backlog_, goal_exprs_, database,
                           goal_candidate_rules_),
      candidate_deactivator_(candidate_frame_offsets_, goal_candidate_rules_),
      elimination_router_(goal_candidate_rules_, srt_active_goals_,
                          elimination_backlog_, candidate_deactivator_),
      get_unit_resolution_(goal_candidate_rules_, lineage_pool_),
      make_initial_goal_lineage_(lineage_pool_),
      initial_goal_activator_(initial_goals, make_initial_goal_lineage_,
                              goal_exprs_, goal_candidate_rules_, srt_active_goals_),
      goal_candidates_deactivator_(goal_candidate_rules_, lineage_pool_,
                                   candidate_deactivator_),
      goal_candidates_activator_(database, lineage_pool_, candidate_activator_,
                                 conflict_detector_, unit_goal_detector_, unit_goals_),
      subgoals_activator_(lineage_pool_, goal_activator_, database,
                          goal_candidates_activator_),
      srt_subgoals_activator_(srt_active_goals_, subgoals_activator_),
      initial_goals_activator_(initial_goals, initial_goal_activator_,
                               make_initial_goal_lineage_, goal_candidates_activator_),
      srt_initial_goals_activator_(srt_active_goals_, initial_goals_activator_),
      resolver_(srt_goal_deactivator_, srt_subgoals_activator_,
                goal_candidates_deactivator_, chosen_goal_candidates_),
      ridge_reward_(decision_memory_),
      checkpoints_(trail_, trail_),
      rng_(random_seed),
      dbuct_sim_(checkpoints_, lineage_pool_, rng_, exploration_constant,
                 grant_increment_interval),
      mcts_decision_generator_(lineage_pool_, srt_active_goals_,
                               dbuct_sim_, goal_candidate_rules_),
      frontier_ready_(),
      run_sim_(frontier_ready_, solution_detector_, conflict_detector_,
               unit_goal_detector_, unit_goals_, unit_goals_,
               mcts_decision_generator_, joint_, elimination_router_,
               resolver_, get_unit_resolution_, decision_memory_,
               resolution_memory_, resolution_memory_, max_resolutions),
      learn_reapply_(cdcl_, elimination_router_, conflict_detector_,
                     unit_goal_detector_, unit_goals_),
      solver_(srt_initial_goals_activator_, run_sim_, decision_memory_,
              decision_memory_, ridge_reward_, dbuct_sim_, cdcl_, learn_reapply_) {}

#endif

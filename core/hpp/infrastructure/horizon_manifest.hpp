#ifndef HORIZON_MANIFEST_HPP
#define HORIZON_MANIFEST_HPP

#include <cstddef>
#include <cstdint>
#include <random>
#include "infrastructure/bind_map.hpp"
#include "infrastructure/bind_map_factory.hpp"
#include "infrastructure/candidate_activator.hpp"
#include "infrastructure/candidate_deactivator.hpp"
#include "infrastructure/candidate_frame_offsets.hpp"
#include "infrastructure/cdcl_elimination_generator.hpp"
#include "infrastructure/chosen_goal_candidates.hpp"
#include "infrastructure/conflict_detector.hpp"
#include "infrastructure/cumulative_grounded_weight.hpp"
#include "infrastructure/db.hpp"
#include "infrastructure/decision_memory.hpp"
#include "infrastructure/elimination_backlog.hpp"
#include "infrastructure/elimination_router.hpp"
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/frame_bump_allocator.hpp"
#include "infrastructure/get_resolution_rule.hpp"
#include "infrastructure/get_unit_resolution.hpp"
#include "infrastructure/globalizer.hpp"
#include "infrastructure/goal_activator.hpp"
#include "infrastructure/goal_candidate_rules.hpp"
#include "infrastructure/goal_candidates_activator.hpp"
#include "infrastructure/goal_candidates_deactivator.hpp"
#include "infrastructure/goal_exprs.hpp"
#include "infrastructure/querier.hpp"
#include "infrastructure/goal_weights.hpp"
#include "infrastructure/horizon_goal_activator.hpp"
#include "infrastructure/horizon_goal_deactivator.hpp"
#include "infrastructure/horizon_initial_goal_activator.hpp"
#include "infrastructure/horizon_resolver.hpp"
#include "infrastructure/horizon_reward.hpp"
#include "infrastructure/horizon_set_up_sim.hpp"
#include "infrastructure/horizon_tear_down_sim.hpp"
#include "infrastructure/initial_goal_activator.hpp"
#include "infrastructure/initial_goal_exprs.hpp"
#include "infrastructure/initial_goal_weight.hpp"
#include "infrastructure/initial_goals_activator.hpp"
#include "infrastructure/joint_elimination_generator.hpp"
#include "infrastructure/lineage_pool.hpp"
#include "infrastructure/make_initial_goal_lineage.hpp"
#include "infrastructure/mcts_decision_generator.hpp"
#include "infrastructure/mcts_sim.hpp"
#include "uniform_value_delta.hpp"
#include "infrastructure/mhu_elimination_generator.hpp"
#include "infrastructure/ra_rule_id_set_factory.hpp"
#include "infrastructure/resolution_memory.hpp"
#include "infrastructure/resolution_recorder.hpp"
#include "infrastructure/resolver.hpp"
#include "infrastructure/rule_id_set_factory.hpp"
#include "infrastructure/run_sim.hpp"
#include "infrastructure/set_up_sim.hpp"
#include "infrastructure/solution_detector.hpp"
#include "infrastructure/solver.hpp"
#include "infrastructure/srt_active_goals.hpp"
#include "infrastructure/srt_goal_deactivator.hpp"
#include "infrastructure/srt_initial_goals_activator.hpp"
#include "infrastructure/srt_subgoals_activator.hpp"
#include "infrastructure/subgoals_activator.hpp"
#include "infrastructure/tear_down_sim.hpp"
#include "infrastructure/unit_goal_detector.hpp"
#include "infrastructure/unit_goals.hpp"
#include "infrastructure/unifier.hpp"
#include "infrastructure/unifier_factory.hpp"
#include "infrastructure/normalizer.hpp"
#include "infrastructure/solver_driver.hpp"

struct horizon_manifest {
    using bind_map_t        = bind_map<globalizer>;
    using bind_map_factory_t = bind_map_factory<globalizer>;
    using unifier_factory_t = unifier_factory<globalizer, bind_map_t>;
    using cdcl_t  = cdcl_elimination_generator<chosen_goal_candidates>;
    using mhu_t   = mhu_elimination_generator<
                    bind_map_t, bind_map_t, bind_map_t, bind_map_factory_t,
                    unifier<globalizer, bind_map_t>, unifier_factory_t,
                    lineage_pool, expr_pool, goal_candidate_rules>;
    using joint_t = joint_elimination_generator<cdcl_t, mhu_t>;

    using get_resolution_rule_t         = get_resolution_rule<db>;
    using conflict_detector_t          = conflict_detector<goal_candidate_rules>;
    using unit_goal_detector_t          = unit_goal_detector<goal_candidate_rules>;
    using solution_detector_t          = solution_detector<srt_active_goals>;
    using goal_activator_t             = goal_activator<goal_exprs, goal_candidate_rules,
                                        srt_active_goals, candidate_frame_offsets, get_resolution_rule_t>;
    using srt_goal_deactivator_t        = srt_goal_deactivator<goal_exprs, goal_candidate_rules>;
    using candidate_deactivator_t      = candidate_deactivator<candidate_frame_offsets, goal_candidate_rules>;
    using candidate_activator_t        = candidate_activator<frame_bump_allocator, candidate_frame_offsets,
                                        mhu_t, elimination_backlog, goal_exprs, db, goal_candidate_rules>;
    using elimination_router_t         = elimination_router<goal_candidate_rules, srt_active_goals,
                                        elimination_backlog, candidate_deactivator_t>;
    using get_unit_resolution_t         = get_unit_resolution<goal_candidate_rules, lineage_pool>;
    using make_initial_goal_lineage_t    = make_initial_goal_lineage<lineage_pool>;
    using initial_goal_activator_t      = initial_goal_activator<initial_goal_exprs,
                                        make_initial_goal_lineage_t, goal_exprs, goal_candidate_rules, srt_active_goals>;
    using goal_candidates_deactivator_t = goal_candidates_deactivator<goal_candidate_rules,
                                        lineage_pool, candidate_deactivator_t>;
    using querier_t                     = querier<goal_exprs, db, db>;
    using goal_candidates_activator_t   = goal_candidates_activator<querier_t, lineage_pool,
                                        candidate_activator_t, conflict_detector_t,
                                        unit_goal_detector_t, unit_goals>;
    using horizon_goal_activator_t      = horizon_goal_activator<goal_activator_t, goal_weights, goal_weights, db>;
    using horizon_goal_deactivator_t    = horizon_goal_deactivator<srt_goal_deactivator_t, goal_weights>;
    using horizon_initial_goal_activator_t = horizon_initial_goal_activator<initial_goal_activator_t,
                                        make_initial_goal_lineage_t, goal_weights, initial_goal_weight>;
    using subgoals_activator_t         = subgoals_activator<lineage_pool, horizon_goal_activator_t,
                                        db, goal_candidates_activator_t>;
    using srt_subgoals_activator_t      = srt_subgoals_activator<srt_active_goals, srt_active_goals, subgoals_activator_t>;
    using initial_goals_activator_t     = initial_goals_activator<initial_goal_exprs,
                                        horizon_initial_goal_activator_t, make_initial_goal_lineage_t, goal_candidates_activator_t>;
    using srt_initial_goals_activator_t  = srt_initial_goals_activator<srt_active_goals, initial_goals_activator_t>;
    using resolver_t                  = resolver<horizon_goal_deactivator_t, srt_subgoals_activator_t, goal_candidates_deactivator_t, chosen_goal_candidates>;
    using horizon_resolver_t           = horizon_resolver<resolver_t, db, goal_weights, cumulative_grounded_weight>;
    using set_up_sim_t      = set_up_sim<elimination_backlog>;
    using tear_down_sim_t      = tear_down_sim<elimination_backlog, unit_goals, decision_memory, resolution_memory,
                            goal_candidate_rules, goal_exprs, srt_active_goals, candidate_frame_offsets,
                            mhu_t, bind_map_t, lineage_pool, frame_bump_allocator, cdcl_t, chosen_goal_candidates>;
    using horizon_reward_t     = horizon_reward<cumulative_grounded_weight>;
    using value_delta_t    = monte_carlo::uniform_value_delta<double>;
    using mcts_sim_t       = mcts_sim<value_delta_t, lineage_pool>;
    using horizon_set_up_sim_t = horizon_set_up_sim<mcts_sim_t, set_up_sim_t>;
    using horizon_tear_down_sim_t = horizon_tear_down_sim<
        horizon_reward_t, value_delta_t, mcts_sim_t,
        goal_weights, cumulative_grounded_weight, tear_down_sim_t>;
    using mcts_decision_generator_t = mcts_decision_generator<lineage_pool, srt_active_goals,
                                    srt_active_goals, srt_active_goals, mcts_sim_t, goal_candidate_rules>;
    using resolution_recorder_t = resolution_recorder<decision_memory, resolution_memory>;
    using run_sim_t        = run_sim<srt_initial_goals_activator_t, solution_detector_t, conflict_detector_t,
                            unit_goal_detector_t, unit_goals, unit_goals, mcts_decision_generator_t,
                            joint_t, elimination_router_t, horizon_resolver_t, get_unit_resolution_t,
                            resolution_recorder_t, resolution_recorder_t, resolution_memory>;
    using solver_t        = solver<horizon_set_up_sim_t, horizon_tear_down_sim_t, run_sim_t,
                            decision_memory, decision_memory,
                            lineage_pool, cdcl_t, elimination_router_t>;
    using normalizer_t    = normalizer<globalizer, expr_pool, expr_pool, bind_map_t>;

    horizon_manifest(
        db& database,
        initial_goal_exprs& initial_goals,
        uint32_t initial_frame_offset,
        size_t max_resolutions,
        uint32_t random_seed,
        double exploration_constant);

    globalizer              globalizer_;
    bind_map_t              bind_map_;
    bind_map_factory_t      bind_map_factory_;
    unifier_factory_t          unifier_factory_;
    lineage_pool            lineage_pool_;
    rule_id_set_factory     rule_id_set_factory_;
    ra_rule_id_set_factory  ra_rule_id_set_factory_;
    srt_active_goals        srt_active_goals_;
    goal_exprs              goal_exprs_;
    querier_t               querier_;
    goal_weights            goal_weights_;
    cumulative_grounded_weight cumulative_grounded_weight_;
    initial_goal_weight     initial_goal_weight_;
    goal_candidate_rules    goal_candidate_rules_;
    unit_goals              unit_goals_;
    decision_memory         decision_memory_;
    resolution_memory       resolution_memory_;
    candidate_frame_offsets candidate_frame_offsets_;
    chosen_goal_candidates  chosen_goal_candidates_;
    expr_pool               expr_pool_;
    frame_bump_allocator    frame_allocator_;
    elimination_backlog     elimination_backlog_;
    cdcl_t                    cdcl_;
    mhu_t                     mhu_;
    joint_t                   joint_;
    get_resolution_rule_t           get_resolution_rule_;
    conflict_detector_t            conflict_detector_;
    unit_goal_detector_t            unit_goal_detector_;
    solution_detector_t            solution_detector_;
    goal_activator_t               goal_activator_;
    horizon_goal_activator_t        horizon_goal_activator_;
    srt_goal_deactivator_t          srt_goal_deactivator_;
    horizon_goal_deactivator_t      horizon_goal_deactivator_;
    candidate_activator_t          candidate_activator_;
    candidate_deactivator_t        candidate_deactivator_;
    elimination_router_t           elimination_router_;
    get_unit_resolution_t           get_unit_resolution_;
    make_initial_goal_lineage_t      make_initial_goal_lineage_;
    initial_goal_activator_t        initial_goal_activator_;
    horizon_initial_goal_activator_t horizon_initial_goal_activator_;
    goal_candidates_deactivator_t   goal_candidates_deactivator_;
    goal_candidates_activator_t     goal_candidates_activator_;
    subgoals_activator_t           subgoals_activator_;
    srt_subgoals_activator_t        srt_subgoals_activator_;
    initial_goals_activator_t       initial_goals_activator_;
    srt_initial_goals_activator_t    srt_initial_goals_activator_;
    resolver_t                    resolver_;
    horizon_resolver_t             horizon_resolver_;
    set_up_sim_t                    set_up_sim_;
    tear_down_sim_t                    tear_down_sim_;
    horizon_reward_t               horizon_reward_;
    value_delta_t                  value_delta_;
    std::mt19937                rng_;
    mcts_sim_t                 mcts_sim_;
    horizon_set_up_sim_t           horizon_set_up_sim_;
    horizon_tear_down_sim_t        horizon_tear_down_sim_;
    mcts_decision_generator_t       mcts_decision_generator_;
    resolution_recorder_t            resolution_recorder_;
    run_sim_t                      run_sim_;
    solver_t                      solver_;
    normalizer_t                  normalizer_;
    solver_driver                 driver_;
};

#endif

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
#include "infrastructure/goal_weights.hpp"
#include "infrastructure/horizon_goal_activator.hpp"
#include "infrastructure/horizon_goal_deactivator.hpp"
#include "infrastructure/horizon_initial_goal_activator.hpp"
#include "infrastructure/horizon_resolver.hpp"
#include "infrastructure/horizon_reward.hpp"
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
#include "infrastructure/mhu_elimination_generator.hpp"
#include "infrastructure/ra_rule_id_set_factory.hpp"
#include "infrastructure/resolution_memory.hpp"
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
#include "infrastructure/trail.hpp"
#include "infrastructure/unit_goal_detector.hpp"
#include "infrastructure/unit_goals.hpp"
#include "infrastructure/unifier.hpp"
#include "infrastructure/unifier_factory.hpp"

struct horizon_manifest {
    using UnifierFactory = unifier_factory<bind_map>;
    using Cdcl  = cdcl_elimination_generator<chosen_goal_candidates, chosen_goal_candidates>;
    using Mhu   = mhu_elimination_generator<
                    bind_map, bind_map_factory, unifier<bind_map>, UnifierFactory,
                    lineage_pool, expr_pool, goal_candidate_rules>;
    using Joint = joint_elimination_generator<Cdcl, Mhu>;

    using GetResolutionRule         = get_resolution_rule<db>;
    using ConflictDetector          = conflict_detector<goal_candidate_rules>;
    using UnitGoalDetector          = unit_goal_detector<goal_candidate_rules>;
    using SolutionDetector          = solution_detector<srt_active_goals>;
    using GoalActivator             = goal_activator<goal_exprs, goal_candidate_rules,
                                        srt_active_goals, candidate_frame_offsets, GetResolutionRule>;
    using SrtGoalDeactivator        = srt_goal_deactivator<goal_exprs, goal_candidate_rules>;
    using CandidateDeactivator      = candidate_deactivator<candidate_frame_offsets, goal_candidate_rules>;
    using CandidateActivator        = candidate_activator<frame_bump_allocator, candidate_frame_offsets,
                                        Mhu, elimination_backlog, goal_exprs, db, goal_candidate_rules>;
    using EliminationRouter         = elimination_router<goal_candidate_rules, srt_active_goals,
                                        elimination_backlog, CandidateDeactivator>;
    using GetUnitResolution         = get_unit_resolution<goal_candidate_rules, lineage_pool>;
    using MakeInitialGoalLineage    = make_initial_goal_lineage<lineage_pool>;
    using InitialGoalActivator      = initial_goal_activator<initial_goal_exprs,
                                        MakeInitialGoalLineage, goal_exprs, goal_candidate_rules, srt_active_goals>;
    using GoalCandidatesDeactivator = goal_candidates_deactivator<goal_candidate_rules,
                                        lineage_pool, CandidateDeactivator>;
    using GoalCandidatesActivator   = goal_candidates_activator<db, lineage_pool, CandidateActivator,
                                        ConflictDetector, UnitGoalDetector, unit_goals>;
    using HorizonGoalActivator      = horizon_goal_activator<GoalActivator, goal_weights, db>;
    using HorizonGoalDeactivator    = horizon_goal_deactivator<SrtGoalDeactivator, goal_weights>;
    using HorizonInitialGoalActivator = horizon_initial_goal_activator<InitialGoalActivator,
                                        MakeInitialGoalLineage, goal_weights, initial_goal_weight>;
    using SubgoalsActivator         = subgoals_activator<lineage_pool, HorizonGoalActivator,
                                        db, GoalCandidatesActivator>;
    using SrtSubgoalsActivator      = srt_subgoals_activator<srt_active_goals, SubgoalsActivator>;
    using InitialGoalsActivator     = initial_goals_activator<initial_goal_exprs,
                                        HorizonInitialGoalActivator, MakeInitialGoalLineage, GoalCandidatesActivator>;
    using SrtInitialGoalsActivator  = srt_initial_goals_activator<srt_active_goals, InitialGoalsActivator>;
    using Resolver                  = resolver<HorizonGoalDeactivator, SrtSubgoalsActivator, GoalCandidatesDeactivator>;
    using HorizonResolver           = horizon_resolver<Resolver, db, goal_weights, cumulative_grounded_weight>;
    using SetUpSim      = set_up_sim<trail>;
    using TearDown      = tear_down_sim<trail, unit_goals, decision_memory, resolution_memory,
                            goal_candidate_rules, goal_exprs, srt_active_goals, candidate_frame_offsets,
                            Mhu, bind_map, lineage_pool, frame_bump_allocator, Cdcl, chosen_goal_candidates>;
    using HorizonTearDown   = horizon_tear_down_sim<TearDown, goal_weights, cumulative_grounded_weight>;
    using HorizonReward     = horizon_reward<cumulative_grounded_weight>;
    using MctsSimType       = mcts_sim<SetUpSim, HorizonTearDown, HorizonReward>;
    using MctsDecisionGenerator = mcts_decision_generator<lineage_pool, srt_active_goals,
                                    MctsSimType, goal_candidate_rules>;
    using RunSim        = run_sim<SrtInitialGoalsActivator, SolutionDetector, ConflictDetector,
                            UnitGoalDetector, unit_goals, unit_goals, MctsDecisionGenerator,
                            Joint, EliminationRouter, HorizonResolver, GetUnitResolution,
                            decision_memory, resolution_memory>;
    using Solver        = solver<MctsSimType, MctsSimType, RunSim, decision_memory, decision_memory,
                            lineage_pool, Cdcl, EliminationRouter>;

    horizon_manifest(
        db& database,
        initial_goal_exprs& initial_goals,
        uint32_t initial_frame_offset,
        size_t max_resolutions,
        uint32_t random_seed,
        double exploration_constant);

    db& database_;
    initial_goal_exprs& initial_goals_;
    size_t max_resolutions_;

    globalizer& globalizer_;
    trail& trail_;
    bind_map& bind_map_;
    bind_map_factory& bind_map_factory_;
    UnifierFactory& unifier_factory_;
    expr_pool& expr_pool_;
    frame_bump_allocator& frame_allocator_;
    lineage_pool& lineage_pool_;
    srt_active_goals& srt_active_goals_;
    goal_exprs& goal_exprs_;
    goal_weights& goal_weights_;
    cumulative_grounded_weight& cumulative_grounded_weight_;
    initial_goal_weight& initial_goal_weight_;
    goal_candidate_rules& goal_candidate_rules_;
    unit_goals& unit_goals_;
    decision_memory& decision_memory_;
    resolution_memory& resolution_memory_;
    elimination_backlog& elimination_backlog_;
    Cdcl& cdcl_;
    Mhu& mhu_;
    Joint& joint_;
    candidate_frame_offsets& candidate_frame_offsets_;
    GetResolutionRule& get_resolution_rule_;
    ConflictDetector& conflict_detector_;
    UnitGoalDetector& unit_goal_detector_;
    SolutionDetector& solution_detector_;
    GoalActivator& goal_activator_;
    HorizonGoalActivator& horizon_goal_activator_;
    SrtGoalDeactivator& srt_goal_deactivator_;
    HorizonGoalDeactivator& horizon_goal_deactivator_;
    CandidateActivator& candidate_activator_;
    CandidateDeactivator& candidate_deactivator_;
    EliminationRouter& elimination_router_;
    GetUnitResolution& get_unit_resolution_;
    MakeInitialGoalLineage& make_initial_goal_lineage_;
    InitialGoalActivator& initial_goal_activator_;
    HorizonInitialGoalActivator& horizon_initial_goal_activator_;
    std::mt19937& rng_;
    MctsDecisionGenerator& mcts_decision_generator_;
    Resolver& resolver_;
    HorizonResolver& horizon_resolver_;
    SetUpSim& set_up_sim_;
    HorizonTearDown& tear_down_sim_;
    HorizonReward& horizon_reward_;
    MctsSimType& mcts_sim_;
    RunSim& run_sim_;
    Solver& solver_;

private:
    globalizer              globalizer_obj_;
    trail                   trail_obj_;
    bind_map                bind_map_obj_;
    bind_map_factory        bind_map_factory_obj_;
    UnifierFactory          unifier_factory_obj_;
    lineage_pool            lineage_pool_obj_;
    rule_id_set_factory     rule_id_set_factory_obj_;
    ra_rule_id_set_factory  ra_rule_id_set_factory_obj_;
    srt_active_goals        srt_active_goals_obj_;
    goal_exprs              goal_exprs_obj_;
    goal_weights            goal_weights_obj_;
    cumulative_grounded_weight cumulative_grounded_weight_obj_;
    initial_goal_weight     initial_goal_weight_obj_;
    goal_candidate_rules    goal_candidate_rules_obj_;
    unit_goals              unit_goals_obj_;
    decision_memory         decision_memory_obj_;
    resolution_memory       resolution_memory_obj_;
    candidate_frame_offsets candidate_frame_offsets_obj_;
    chosen_goal_candidates  chosen_goal_candidates_obj_;
    expr_pool               expr_pool_obj_;
    frame_bump_allocator    frame_allocator_obj_;
    elimination_backlog     elimination_backlog_obj_;
    Cdcl                    cdcl_obj_;
    Mhu                     mhu_obj_;
    Joint                   joint_obj_;
    GetResolutionRule           get_resolution_rule_obj_;
    ConflictDetector            conflict_detector_obj_;
    UnitGoalDetector            unit_goal_detector_obj_;
    SolutionDetector            solution_detector_obj_;
    GoalActivator               goal_activator_obj_;
    HorizonGoalActivator        horizon_goal_activator_obj_;
    SrtGoalDeactivator          srt_goal_deactivator_obj_;
    HorizonGoalDeactivator      horizon_goal_deactivator_obj_;
    CandidateActivator          candidate_activator_obj_;
    CandidateDeactivator        candidate_deactivator_obj_;
    EliminationRouter           elimination_router_obj_;
    GetUnitResolution           get_unit_resolution_obj_;
    MakeInitialGoalLineage      make_initial_goal_lineage_obj_;
    InitialGoalActivator        initial_goal_activator_obj_;
    HorizonInitialGoalActivator horizon_initial_goal_activator_obj_;
    GoalCandidatesDeactivator   goal_candidates_deactivator_obj_;
    GoalCandidatesActivator     goal_candidates_activator_obj_;
    SubgoalsActivator           subgoals_activator_obj_;
    SrtSubgoalsActivator        srt_subgoals_activator_obj_;
    InitialGoalsActivator       initial_goals_activator_obj_;
    SrtInitialGoalsActivator    srt_initial_goals_activator_obj_;
    std::mt19937                rng_obj_;
    Resolver                    resolver_obj_;
    HorizonResolver             horizon_resolver_obj_;
    SetUpSim                    set_up_sim_obj_;
    TearDown                    tear_down_base_obj_;
    HorizonTearDown             horizon_tear_down_obj_;
    HorizonReward               horizon_reward_obj_;
    MctsSimType                 mcts_sim_obj_;
    MctsDecisionGenerator       mcts_decision_generator_obj_;
    RunSim                      run_sim_obj_;
    Solver                      solver_obj_;
};

#endif

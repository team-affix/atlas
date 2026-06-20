#ifndef BASIC_MANIFEST_HPP
#define BASIC_MANIFEST_HPP

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
#include "infrastructure/goal_deactivator.hpp"
#include "infrastructure/goal_exprs.hpp"
#include "infrastructure/initial_goal_activator.hpp"
#include "infrastructure/initial_goal_exprs.hpp"
#include "infrastructure/initial_goals_activator.hpp"
#include "infrastructure/joint_elimination_generator.hpp"
#include "infrastructure/lineage_pool.hpp"
#include "infrastructure/make_initial_goal_lineage.hpp"
#include "infrastructure/mhu_elimination_generator.hpp"
#include "infrastructure/ra_active_goals.hpp"
#include "infrastructure/ra_rule_id_set_factory.hpp"
#include "infrastructure/random_decision_generator.hpp"
#include "infrastructure/resolution_memory.hpp"
#include "infrastructure/resolver.hpp"
#include "infrastructure/rule_id_set_factory.hpp"
#include "infrastructure/run_sim.hpp"
#include "infrastructure/set_up_sim.hpp"
#include "infrastructure/solution_detector.hpp"
#include "infrastructure/solver.hpp"
#include "infrastructure/subgoals_activator.hpp"
#include "infrastructure/tear_down_sim.hpp"
#include "infrastructure/trail.hpp"
#include "infrastructure/unifier.hpp"
#include "infrastructure/unifier_factory.hpp"
#include "infrastructure/unit_goal_detector.hpp"
#include "infrastructure/unit_goals.hpp"

struct basic_manifest {
    using UnifierFactory = unifier_factory<bind_map>;
    using Cdcl  = cdcl_elimination_generator<chosen_goal_candidates>;
    using Mhu   = mhu_elimination_generator<
                    bind_map, bind_map_factory, unifier<bind_map>, UnifierFactory,
                    lineage_pool, expr_pool, goal_candidate_rules>;
    using Joint = joint_elimination_generator<Cdcl, Mhu>;

    using GetResolutionRule         = get_resolution_rule<db>;
    using ConflictDetector          = conflict_detector<goal_candidate_rules>;
    using UnitGoalDetector          = unit_goal_detector<goal_candidate_rules>;
    using SolutionDetector          = solution_detector<ra_active_goals>;
    using GoalActivator             = goal_activator<goal_exprs, goal_candidate_rules,
                                        ra_active_goals, candidate_frame_offsets, GetResolutionRule>;
    using GoalDeactivator           = goal_deactivator<goal_exprs, goal_candidate_rules, ra_active_goals>;
    using CandidateDeactivator      = candidate_deactivator<candidate_frame_offsets, goal_candidate_rules>;
    using CandidateActivator        = candidate_activator<frame_bump_allocator, candidate_frame_offsets,
                                        Mhu, elimination_backlog, goal_exprs, db, goal_candidate_rules>;
    using EliminationRouter         = elimination_router<goal_candidate_rules, ra_active_goals,
                                        elimination_backlog, CandidateDeactivator>;
    using GetUnitResolution         = get_unit_resolution<goal_candidate_rules, lineage_pool>;
    using MakeInitialGoalLineage    = make_initial_goal_lineage<lineage_pool>;
    using InitialGoalActivator      = initial_goal_activator<initial_goal_exprs,
                                        MakeInitialGoalLineage, goal_exprs, goal_candidate_rules, ra_active_goals>;
    using GoalCandidatesDeactivator = goal_candidates_deactivator<goal_candidate_rules,
                                        lineage_pool, CandidateDeactivator>;
    using GoalCandidatesActivator   = goal_candidates_activator<db, lineage_pool, CandidateActivator,
                                        ConflictDetector, UnitGoalDetector, unit_goals>;
    using SubgoalsActivator         = subgoals_activator<lineage_pool, GoalActivator,
                                        db, GoalCandidatesActivator>;
    using InitialGoalsActivator     = initial_goals_activator<initial_goal_exprs,
                                        InitialGoalActivator, MakeInitialGoalLineage, GoalCandidatesActivator>;
    using Resolver                  = resolver<GoalDeactivator, SubgoalsActivator, GoalCandidatesDeactivator, chosen_goal_candidates>;
    using RandomDecisionGenerator   = random_decision_generator<lineage_pool, ra_active_goals, goal_candidate_rules>;
    using SetUpSim  = set_up_sim<trail>;
    using TearDown  = tear_down_sim<trail, unit_goals, decision_memory, resolution_memory,
                        goal_candidate_rules, goal_exprs, ra_active_goals, candidate_frame_offsets,
                        Mhu, bind_map, lineage_pool, frame_bump_allocator, Cdcl, chosen_goal_candidates>;
    using RunSim    = run_sim<InitialGoalsActivator, SolutionDetector, ConflictDetector,
                        UnitGoalDetector, unit_goals, unit_goals, RandomDecisionGenerator,
                        Joint, EliminationRouter, Resolver, GetUnitResolution,
                        decision_memory, resolution_memory>;
    using Solver    = solver<SetUpSim, TearDown, RunSim, decision_memory, decision_memory,
                        lineage_pool, Cdcl, EliminationRouter>;

    basic_manifest(
        db& database,
        initial_goal_exprs& initial_goals,
        uint32_t initial_frame_offset,
        size_t max_resolutions,
        uint32_t random_seed);

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
    ra_active_goals& ra_active_goals_;
    goal_exprs& goal_exprs_;
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
    GoalDeactivator& goal_deactivator_;
    CandidateActivator& candidate_activator_;
    CandidateDeactivator& candidate_deactivator_;
    EliminationRouter& elimination_router_;
    GetUnitResolution& get_unit_resolution_;
    MakeInitialGoalLineage& make_initial_goal_lineage_;
    InitialGoalActivator& initial_goal_activator_;
    std::mt19937& rng_;
    RandomDecisionGenerator& random_decision_generator_;
    Resolver& resolver_;
    SetUpSim& set_up_sim_;
    TearDown& tear_down_sim_;
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
    ra_active_goals         ra_active_goals_obj_;
    goal_exprs              goal_exprs_obj_;
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
    GoalDeactivator             goal_deactivator_obj_;
    CandidateActivator          candidate_activator_obj_;
    CandidateDeactivator        candidate_deactivator_obj_;
    EliminationRouter           elimination_router_obj_;
    GetUnitResolution           get_unit_resolution_obj_;
    MakeInitialGoalLineage      make_initial_goal_lineage_obj_;
    InitialGoalActivator        initial_goal_activator_obj_;
    GoalCandidatesDeactivator   goal_candidates_deactivator_obj_;
    GoalCandidatesActivator     goal_candidates_activator_obj_;
    SubgoalsActivator           subgoals_activator_obj_;
    InitialGoalsActivator       initial_goals_activator_obj_;
    std::mt19937                rng_obj_;
    RandomDecisionGenerator     random_decision_generator_obj_;
    Resolver                    resolver_obj_;
    SetUpSim                    set_up_sim_obj_;
    TearDown                    tear_down_obj_;
    RunSim                      run_sim_obj_;
    Solver                      solver_obj_;
};

#endif

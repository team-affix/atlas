// Integration: ridge_manifest — locator wiring identity + sim lifecycle via MCTS stack.

#include <gtest/gtest.h>
#include "infrastructure/db.hpp"
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/initial_goal_exprs.hpp"
#include "infrastructure/locator.hpp"
#include "infrastructure/ridge_manifest.hpp"
#include "infrastructure/subgoals_activator.hpp"
#include "infrastructure/initial_goals_activator.hpp"
#include "infrastructure/trail.hpp"
#include "interfaces/i_log_to_current_trail_frame.hpp"
#include "interfaces/i_set_up_sim.hpp"
#include "interfaces/i_tear_down_sim.hpp"
#include "interfaces/i_run_sim.hpp"
#include "interfaces/i_generate_decision.hpp"
#include "interfaces/i_mcts_choose.hpp"
#include "interfaces/i_solve.hpp"
#include "interfaces/i_activate_subgoals_and_candidates.hpp"
#include "interfaces/i_activate_initial_goals_and_candidates.hpp"
#include "interfaces/i_iterate_root_goals.hpp"
#include "interfaces/i_iterate_child_goals.hpp"
#include "interfaces/i_is_active_goal.hpp"
#include "interfaces/i_insert_active_goal.hpp"
#include "interfaces/i_clear_active_goals.hpp"
#include "interfaces/i_check_active_goals_empty.hpp"
#include "interfaces/i_srt_link_goal_batch_parent.hpp"
#include "interfaces/i_srt_flush_goal_batch.hpp"
#include "interfaces/i_push_trail_frame.hpp"
#include "interfaces/i_pop_trail_frame.hpp"
#include "interfaces/i_compute_mcts_reward.hpp"
#include "interfaces/i_get_decision_count.hpp"
#include "interfaces/i_record_decision.hpp"
#include "interfaces/i_clear_recorded_decisions.hpp"
#include "interfaces/i_derive_decision_lemma.hpp"
#include "value_objects/expr.hpp"
#include "value_objects/lemma.hpp"
#include "value_objects/sim_termination.hpp"
#include "functor_fixture.hpp"

namespace {

#include "functor_fixture.hpp"

class RidgeManifestIntegrationTest : public ::testing::Test {
protected:
    test_functors functors;
    static constexpr size_t kInitialVarCount = 0;
    static constexpr size_t kMaxResolutions = 64;
    static constexpr uint32_t kSeed = 42;
    static constexpr double kExplorationConstant = 1.414;

    db database;
    initial_goal_exprs initial_goals;

    locator saved_loc_;
    expr_pool saved_expr_pool_;

    ridge_manifest make_manifest(size_t max_resolutions = kMaxResolutions) {
        return ridge_manifest{
            database,
            initial_goals,
            kInitialVarCount,
            max_resolutions,
            kSeed,
            kExplorationConstant};
    }
};

// Tier W — wiring / shared-instance identity (no sim run)

TEST_F(RidgeManifestIntegrationTest, WiringConstructsWithEmptyDbAndNoGoals) {
    /*
     * Intent: ridge_manifest constructs without throwing on an empty problem.
     * initial goals: (none)
     * rules: (none)
     */
    EXPECT_NO_THROW(make_manifest());
}

TEST_F(RidgeManifestIntegrationTest, WiringMaxResolutionsStored) {
    /*
     * Intent: constructor stores the max_resolutions budget on the manifest.
     * initial goals: (none)
     * rules: (none)
     */
    ridge_manifest manifest = make_manifest();
    EXPECT_EQ(manifest.max_resolutions_, kMaxResolutions);
}

TEST_F(RidgeManifestIntegrationTest, WiringMctsSimIsSetUpTearDownAndMctsChoose) {
    /*
     * Intent: mcts_sim_ backs the locator's sim setup/teardown and MCTS choose interfaces.
     * initial goals: (none)
     * rules: (none)
     */
    ridge_manifest manifest = make_manifest();
    EXPECT_EQ(static_cast<i_set_up_sim*>(&manifest.mcts_sim_), &manifest.loc_.locate<i_set_up_sim>());
    EXPECT_EQ(static_cast<i_tear_down_sim*>(&manifest.mcts_sim_), &manifest.loc_.locate<i_tear_down_sim>());
    EXPECT_EQ(static_cast<i_mcts_choose*>(&manifest.mcts_sim_), &manifest.loc_.locate<i_mcts_choose>());
}

TEST_F(RidgeManifestIntegrationTest, WiringInnerSetUpSimDistinctFromLocatorBinding) {
    /*
     * Intent: inner set_up_sim_/tear_down_sim_ are not the locator's i_set_up_sim/i_tear_down_sim.
     * initial goals: (none)
     * rules: (none)
     */
    ridge_manifest manifest = make_manifest();
    EXPECT_NE(
        static_cast<void*>(&manifest.set_up_sim_),
        static_cast<void*>(&manifest.loc_.locate<i_set_up_sim>()));
    EXPECT_NE(
        static_cast<void*>(&manifest.tear_down_sim_),
        static_cast<void*>(&manifest.loc_.locate<i_tear_down_sim>()));
}

TEST_F(RidgeManifestIntegrationTest, WiringMctsDecisionGeneratorIsGenerateDecision) {
    /*
     * Intent: mcts_decision_generator_ is the locator's i_generate_decision binding.
     * initial goals: (none)
     * rules: (none)
     */
    ridge_manifest manifest = make_manifest();
    EXPECT_EQ(
        static_cast<i_generate_decision*>(&manifest.mcts_decision_generator_),
        &manifest.loc_.locate<i_generate_decision>());
}

TEST_F(RidgeManifestIntegrationTest, WiringSrtActiveGoalsSharedForGoalInterfaces) {
    /*
     * Intent: srt_active_goals_ backs goal registry, iteration, and SRT batch interfaces.
     * initial goals: (none)
     * rules: (none)
     */
    ridge_manifest manifest = make_manifest();
    EXPECT_EQ(
        static_cast<i_insert_active_goal*>(&manifest.srt_active_goals_),
        &manifest.loc_.locate<i_insert_active_goal>());
    EXPECT_EQ(
        static_cast<i_iterate_root_goals*>(&manifest.srt_active_goals_),
        &manifest.loc_.locate<i_iterate_root_goals>());
    EXPECT_EQ(
        static_cast<i_iterate_child_goals*>(&manifest.srt_active_goals_),
        &manifest.loc_.locate<i_iterate_child_goals>());
    EXPECT_EQ(
        static_cast<i_is_active_goal*>(&manifest.srt_active_goals_),
        &manifest.loc_.locate<i_is_active_goal>());
    EXPECT_EQ(
        static_cast<i_srt_link_goal_batch_parent*>(&manifest.srt_active_goals_),
        &manifest.loc_.locate<i_srt_link_goal_batch_parent>());
    EXPECT_EQ(
        static_cast<i_srt_flush_goal_batch*>(&manifest.srt_active_goals_),
        &manifest.loc_.locate<i_srt_flush_goal_batch>());
    EXPECT_EQ(
        static_cast<i_clear_active_goals*>(&manifest.srt_active_goals_),
        &manifest.loc_.locate<i_clear_active_goals>());
}

TEST_F(RidgeManifestIntegrationTest, WiringSrtActivatorsWrapInnerConcreteActivators) {
    /*
     * Intent: SRT wrappers are bound for activator interfaces; inner activators stay concrete-only.
     * initial goals: (none)
     * rules: (none)
     */
    ridge_manifest manifest = make_manifest();
    EXPECT_NE(
        static_cast<void*>(&manifest.loc_.locate<i_activate_subgoals_and_candidates>()),
        static_cast<void*>(&manifest.loc_.locate<subgoals_activator>()));
    EXPECT_NE(
        static_cast<void*>(&manifest.loc_.locate<i_activate_initial_goals_and_candidates>()),
        static_cast<void*>(&manifest.loc_.locate<initial_goals_activator>()));
}

TEST_F(RidgeManifestIntegrationTest, WiringRunSimAndSolverBound) {
    /*
     * Intent: run_sim_ and solver_ are the locator's i_run_sim and i_solve bindings.
     * initial goals: (none)
     * rules: (none)
     */
    ridge_manifest manifest = make_manifest();
    EXPECT_EQ(static_cast<i_run_sim*>(&manifest.run_sim_), &manifest.loc_.locate<i_run_sim>());
    EXPECT_EQ(static_cast<i_solve*>(&manifest.solver_), &manifest.loc_.locate<i_solve>());
}

TEST_F(RidgeManifestIntegrationTest, WiringTrailSharedForPushPopLog) {
    /*
     * Intent: trail_ backs push/pop/log trail interfaces.
     * initial goals: (none)
     * rules: (none)
     */
    ridge_manifest manifest = make_manifest();
    EXPECT_EQ(static_cast<i_push_trail_frame*>(&manifest.trail_), &manifest.loc_.locate<i_push_trail_frame>());
    EXPECT_EQ(static_cast<i_pop_trail_frame*>(&manifest.trail_), &manifest.loc_.locate<i_pop_trail_frame>());
    EXPECT_EQ(
        static_cast<i_log_to_current_trail_frame*>(&manifest.trail_),
        &manifest.loc_.locate<i_log_to_current_trail_frame>());
}

TEST_F(RidgeManifestIntegrationTest, WiringDecisionMemorySharedForRecordCountDerive) {
    /*
     * Intent: decision_memory_ backs record/clear/count decision interfaces.
     * initial goals: (none)
     * rules: (none)
     */
    ridge_manifest manifest = make_manifest();
    EXPECT_EQ(
        static_cast<i_record_decision*>(&manifest.decision_memory_),
        &manifest.loc_.locate<i_record_decision>());
    EXPECT_EQ(
        static_cast<i_clear_recorded_decisions*>(&manifest.decision_memory_),
        &manifest.loc_.locate<i_clear_recorded_decisions>());
    EXPECT_EQ(
        static_cast<i_get_decision_count*>(&manifest.decision_memory_),
        &manifest.loc_.locate<i_get_decision_count>());
}

TEST_F(RidgeManifestIntegrationTest, WiringRidgeRewardIsComputeMctsReward) {
    /*
     * Intent: ridge_reward_ backs i_compute_mcts_reward for MCTS tear_down scoring.
     * initial goals: (none)
     * rules: (none)
     */
    ridge_manifest manifest = make_manifest();
    EXPECT_EQ(
        static_cast<i_compute_mcts_reward*>(&manifest.ridge_reward_),
        &manifest.loc_.locate<i_compute_mcts_reward>());
    EXPECT_DOUBLE_EQ(manifest.ridge_reward_.compute_mcts_reward(), 0.0);
}

// Tier L — sim lifecycle via MCTS-wrapped set_up/tear_down

TEST_F(RidgeManifestIntegrationTest, SimLifecycleTrailDepthRestoresAfterEmptyRun) {
    /*
     * Intent: MCTS set_up/tear_down on an empty problem restores trail depth.
     * initial goals: (none)
     * rules: (none)
     */
    ridge_manifest manifest = make_manifest();
    const size_t depth_before = manifest.trail_.depth();
    manifest.loc_.locate<i_set_up_sim>().set_up();
    EXPECT_EQ(manifest.run_sim_.run(), sim_termination::solved);
    manifest.loc_.locate<i_tear_down_sim>().tear_down();
    EXPECT_EQ(manifest.trail_.depth(), depth_before);
}

TEST_F(RidgeManifestIntegrationTest, SimLifecycleClearsActiveGoalsAfterEmptyRun) {
    /*
     * Intent: tear_down clears the SRT active goal registry after a trivial solved run.
     * initial goals: (none)
     * rules: (none)
     */
    ridge_manifest manifest = make_manifest();
    manifest.loc_.locate<i_set_up_sim>().set_up();
    EXPECT_EQ(manifest.run_sim_.run(), sim_termination::solved);
    manifest.loc_.locate<i_tear_down_sim>().tear_down();
    EXPECT_TRUE(manifest.loc_.locate<i_check_active_goals_empty>().empty());
    EXPECT_EQ(manifest.decision_memory_.count(), 0u);
}

TEST_F(RidgeManifestIntegrationTest, SimLifecycleMctsSimAliasMatchesLocatorSetUp) {
    /*
     * Intent: manifest.mcts_sim_ and locate<i_set_up_sim> are the same object used in lifecycle.
     * initial goals: (none)
     * rules: (none)
     */
    ridge_manifest manifest = make_manifest();
    EXPECT_EQ(static_cast<i_set_up_sim*>(&manifest.mcts_sim_), &manifest.loc_.locate<i_set_up_sim>());
    manifest.mcts_sim_.set_up();
    EXPECT_EQ(manifest.run_sim_.run(), sim_termination::solved);
    manifest.mcts_sim_.tear_down();
    EXPECT_TRUE(manifest.srt_active_goals_.empty());
}

TEST_F(RidgeManifestIntegrationTest, SimLifecycleTrailDepthRestoresAfterConflictedRun) {
    /*
     * Intent: trail depth restores after a conflicted sim run with no matching rules.
     * initial goals: f.
     * rules: (none)
     */
    const expr* goal = saved_expr_pool_.make_functor(functors.id("f"), {});
    initial_goals.push(goal);
    ridge_manifest manifest = make_manifest();
    const size_t depth_before = manifest.trail_.depth();
    manifest.loc_.locate<i_set_up_sim>().set_up();
    EXPECT_EQ(manifest.run_sim_.run(), sim_termination::conflicted);
    manifest.loc_.locate<i_tear_down_sim>().tear_down();
    EXPECT_EQ(manifest.trail_.depth(), depth_before);
}

TEST_F(RidgeManifestIntegrationTest, SimMctsDecisionGeneratorRecordsDecisionWithFixedSeed) {
    /*
     * Intent: MCTS decision generator records exactly one f-branch decision at seed 42.
     * initial goals: f.
     * rules:
     *   0: f.
     *   1: f.
     */
    const expr* goal = saved_expr_pool_.make_functor(functors.id("f"), {});
    const expr* f_head0 = saved_expr_pool_.make_functor(functors.id("f"), {});
    const expr* f_head1 = saved_expr_pool_.make_functor(functors.id("f"), {});
    initial_goals.push(goal);
    database.push(rule{f_head0, {}});
    database.push(rule{f_head1, {}});

    ridge_manifest manifest = make_manifest();
    manifest.loc_.locate<i_set_up_sim>().set_up();
    EXPECT_EQ(manifest.run_sim_.run(), sim_termination::solved);
    EXPECT_EQ(manifest.decision_memory_.count(), 1u);
    const lemma dl = manifest.decision_memory_.derive_decision_lemma();
    ASSERT_EQ(dl.get_resolutions().size(), 1u);
    const rule_id chosen = (*dl.get_resolutions().begin())->idx;
    EXPECT_TRUE(chosen == 0 || chosen == 1);
    manifest.loc_.locate<i_tear_down_sim>().tear_down();
}

// Tier X — single solver tick

TEST_F(RidgeManifestIntegrationTest, SolverVacuousSolvedOnEmptyProblem) {
    /*
     * Intent: solver yields one vacuous solved tick on an empty problem and completes.
     * initial goals: (none)
     * rules: (none)
     */
    ridge_manifest manifest = make_manifest();
    auto sm = manifest.solver_.solve();
    sm.resume();
    ASSERT_TRUE(sm.has_yield());
    EXPECT_EQ(sm.consume_yield(), sim_termination::solved);
    EXPECT_TRUE(manifest.decision_memory_.derive_decision_lemma().get_resolutions().empty());
    sm.resume();
    EXPECT_FALSE(sm.has_yield());
}

TEST_F(RidgeManifestIntegrationTest, SolverFindsSingleUnitSolution) {
    /*
     * Intent: a single ground rule solves f without any decision.
     * initial goals: f.
     * rules:
     *   0: f.
     */
    const expr* goal = saved_expr_pool_.make_functor(functors.id("f"), {});
    const expr* head = saved_expr_pool_.make_functor(functors.id("f"), {});
    initial_goals.push(goal);
    database.push(rule{head, {}});

    ridge_manifest manifest = make_manifest();
    auto sm = manifest.solver_.solve();
    sm.resume();
    ASSERT_TRUE(sm.has_yield());
    EXPECT_EQ(sm.consume_yield(), sim_termination::solved);
    EXPECT_TRUE(manifest.decision_memory_.derive_decision_lemma().get_resolutions().empty());
    sm.resume();
    EXPECT_FALSE(sm.has_yield());
}

}  // namespace

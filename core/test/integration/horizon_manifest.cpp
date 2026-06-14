// Integration: horizon_manifest — locator wiring identity + sim lifecycle via MCTS stack
// with CGW reward and goal weight stores.

#include <gtest/gtest.h>
#include "infrastructure/db.hpp"
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/goal_activator.hpp"
#include "infrastructure/horizon_manifest.hpp"
#include "infrastructure/initial_goal_activator.hpp"
#include "infrastructure/initial_goal_exprs.hpp"
#include "infrastructure/initial_goals_activator.hpp"
#include "infrastructure/locator.hpp"
#include "infrastructure/resolver.hpp"
#include "infrastructure/srt_goal_deactivator.hpp"
#include "infrastructure/subgoals_activator.hpp"
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
#include "interfaces/i_activate_initial_goal.hpp"
#include "interfaces/i_goal_activator.hpp"
#include "interfaces/i_goal_deactivator.hpp"
#include "interfaces/i_resolver.hpp"
#include "interfaces/i_get_goal_weight.hpp"
#include "interfaces/i_get_grounded_weight.hpp"
#include "interfaces/i_get_initial_goal_weight.hpp"
#include "interfaces/i_compute_mcts_reward.hpp"
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
#include "interfaces/i_get_decision_count.hpp"
#include "interfaces/i_record_decision.hpp"
#include "interfaces/i_clear_recorded_decisions.hpp"
#include "interfaces/i_derive_decision_lemma.hpp"
#include "value_objects/expr.hpp"
#include "value_objects/lemma.hpp"
#include "value_objects/sim_termination.hpp"
#include "functor_fixture.hpp"

namespace {

constexpr double kWeightEpsilon = 1e-9;
constexpr double kTotalWeight = 1.0;

#include "functor_fixture.hpp"

class HorizonManifestIntegrationTest : public ::testing::Test {
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

    horizon_manifest make_manifest(size_t max_resolutions = kMaxResolutions) {
        return horizon_manifest{
            database,
            initial_goals,
            kInitialVarCount,
            max_resolutions,
            kSeed,
            kExplorationConstant};
    }
};

TEST_F(HorizonManifestIntegrationTest, WiringConstructsWithEmptyDbAndNoGoals) {
    EXPECT_NO_THROW(make_manifest());
}

TEST_F(HorizonManifestIntegrationTest, WiringMaxResolutionsStored) {
    horizon_manifest manifest = make_manifest();
    EXPECT_EQ(manifest.max_resolutions_, kMaxResolutions);
}

TEST_F(HorizonManifestIntegrationTest, WiringGoalWeightsAndCgwSharedForInterfaces) {
    horizon_manifest manifest = make_manifest();
    EXPECT_EQ(
        static_cast<i_get_goal_weight*>(&manifest.goal_weights_),
        &manifest.loc_.locate<i_get_goal_weight>());
    EXPECT_EQ(
        static_cast<i_get_grounded_weight*>(&manifest.cumulative_grounded_weight_),
        &manifest.loc_.locate<i_get_grounded_weight>());
}

TEST_F(HorizonManifestIntegrationTest, WiringInitialGoalWeightMatchesOneOverK) {
    const expr* goal0 = saved_expr_pool_.make_functor(functors.id("f"), {});
    const expr* goal1 = saved_expr_pool_.make_functor(functors.id("g"), {});
    initial_goals.push(goal0);
    initial_goals.push(goal1);
    horizon_manifest manifest = make_manifest();
    EXPECT_NEAR(manifest.initial_goal_weight_.get(), kTotalWeight / 2.0, kWeightEpsilon);
    EXPECT_EQ(
        static_cast<i_get_initial_goal_weight*>(&manifest.initial_goal_weight_),
        &manifest.loc_.locate<i_get_initial_goal_weight>());
}

TEST_F(HorizonManifestIntegrationTest, WiringHorizonActivatorsWrapInnerConcrete) {
    horizon_manifest manifest = make_manifest();
    EXPECT_NE(
        static_cast<void*>(&manifest.loc_.locate<i_goal_activator>()),
        static_cast<void*>(&manifest.loc_.locate<goal_activator>()));
    EXPECT_NE(
        static_cast<void*>(&manifest.loc_.locate<i_goal_deactivator>()),
        static_cast<void*>(&manifest.loc_.locate<srt_goal_deactivator>()));
    EXPECT_NE(
        static_cast<void*>(&manifest.loc_.locate<i_activate_initial_goal>()),
        static_cast<void*>(&manifest.loc_.locate<initial_goal_activator>()));
    EXPECT_NE(
        static_cast<void*>(&manifest.loc_.locate<i_resolver>()),
        static_cast<void*>(&manifest.loc_.locate<resolver>()));
}

TEST_F(HorizonManifestIntegrationTest, WiringHorizonRewardIsComputeMctsReward) {
    horizon_manifest manifest = make_manifest();
    EXPECT_EQ(
        static_cast<i_compute_mcts_reward*>(&manifest.horizon_reward_),
        &manifest.loc_.locate<i_compute_mcts_reward>());
    EXPECT_DOUBLE_EQ(manifest.horizon_reward_.compute_mcts_reward(), 0.0);
}

TEST_F(HorizonManifestIntegrationTest, WiringMctsSimIsSetUpTearDownAndMctsChoose) {
    horizon_manifest manifest = make_manifest();
    EXPECT_EQ(static_cast<i_set_up_sim*>(&manifest.mcts_sim_), &manifest.loc_.locate<i_set_up_sim>());
    EXPECT_EQ(static_cast<i_tear_down_sim*>(&manifest.mcts_sim_), &manifest.loc_.locate<i_tear_down_sim>());
    EXPECT_EQ(static_cast<i_mcts_choose*>(&manifest.mcts_sim_), &manifest.loc_.locate<i_mcts_choose>());
}

TEST_F(HorizonManifestIntegrationTest, WiringSrtActivatorsWrapInnerConcreteActivators) {
    horizon_manifest manifest = make_manifest();
    EXPECT_NE(
        static_cast<void*>(&manifest.loc_.locate<i_activate_subgoals_and_candidates>()),
        static_cast<void*>(&manifest.loc_.locate<subgoals_activator>()));
    EXPECT_NE(
        static_cast<void*>(&manifest.loc_.locate<i_activate_initial_goals_and_candidates>()),
        static_cast<void*>(&manifest.loc_.locate<initial_goals_activator>()));
}

TEST_F(HorizonManifestIntegrationTest, SimLifecycleClearsCgwAfterEmptyRun) {
    horizon_manifest manifest = make_manifest();
    manifest.loc_.locate<i_set_up_sim>().set_up();
    EXPECT_EQ(manifest.run_sim_.run(), sim_termination::solved);
    manifest.loc_.locate<i_tear_down_sim>().tear_down();
    EXPECT_DOUBLE_EQ(manifest.cumulative_grounded_weight_.get(), 0.0);
    EXPECT_TRUE(manifest.loc_.locate<i_check_active_goals_empty>().empty());
}

TEST_F(HorizonManifestIntegrationTest, SolverFindsSingleUnitSolutionWithFullCgw) {
    const expr* goal = saved_expr_pool_.make_functor(functors.id("f"), {});
    const expr* head = saved_expr_pool_.make_functor(functors.id("f"), {});
    initial_goals.push(goal);
    database.push(rule{head, {}});

    horizon_manifest manifest = make_manifest();
    auto sm = manifest.solver_.solve();
    sm.resume();
    ASSERT_TRUE(sm.has_yield());
    EXPECT_EQ(sm.consume_yield(), sim_termination::solved);
    EXPECT_NEAR(manifest.horizon_reward_.compute_mcts_reward(), kTotalWeight, kWeightEpsilon);
    sm.resume();
    EXPECT_FALSE(sm.has_yield());
}

}  // namespace

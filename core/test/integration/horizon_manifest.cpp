// Integration: horizon_manifest — wiring identity + sim lifecycle via MCTS stack
// with CGW reward and goal weight stores.

#include <gtest/gtest.h>
#include "infrastructure/db.hpp"
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/horizon_manifest.hpp"
#include "infrastructure/initial_goal_exprs.hpp"
#include "infrastructure/trail.hpp"
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

TEST_F(HorizonManifestIntegrationTest, WiringInitialGoalWeightMatchesOneOverK) {
    const expr* goal0 = saved_expr_pool_.make_functor(functors.id("f"), {});
    const expr* goal1 = saved_expr_pool_.make_functor(functors.id("g"), {});
    initial_goals.push(goal0);
    initial_goals.push(goal1);
    horizon_manifest manifest = make_manifest();
    EXPECT_NEAR(manifest.initial_goal_weight_.get(), kTotalWeight / 2.0, kWeightEpsilon);
}

TEST_F(HorizonManifestIntegrationTest, WiringHorizonActivatorsDistinctFromInnerConcrete) {
    horizon_manifest manifest = make_manifest();
    EXPECT_NE(static_cast<void*>(&manifest.horizon_goal_activator_),
              static_cast<void*>(&manifest.goal_activator_));
    EXPECT_NE(static_cast<void*>(&manifest.horizon_goal_deactivator_),
              static_cast<void*>(&manifest.srt_goal_deactivator_));
    EXPECT_NE(static_cast<void*>(&manifest.horizon_initial_goal_activator_),
              static_cast<void*>(&manifest.initial_goal_activator_));
}

TEST_F(HorizonManifestIntegrationTest, WiringHorizonRewardReturnsZeroInitially) {
    horizon_manifest manifest = make_manifest();
    EXPECT_DOUBLE_EQ(manifest.horizon_reward_.compute_mcts_reward(), 0.0);
}

TEST_F(HorizonManifestIntegrationTest, WiringMctsSimDistinctFromInnerSetUpTearDown) {
    horizon_manifest manifest = make_manifest();
    EXPECT_NE(static_cast<void*>(&manifest.set_up_sim_),
              static_cast<void*>(&manifest.mcts_sim_));
    EXPECT_NE(static_cast<void*>(&manifest.tear_down_sim_),
              static_cast<void*>(&manifest.mcts_sim_));
}

TEST_F(HorizonManifestIntegrationTest, SimLifecycleClearsCgwAfterEmptyRun) {
    horizon_manifest manifest = make_manifest();
    manifest.set_up_sim_.set_up();
    EXPECT_EQ(manifest.run_sim_.run(), sim_termination::solved);
    manifest.tear_down_sim_.tear_down();
    EXPECT_DOUBLE_EQ(manifest.cumulative_grounded_weight_.get(), 0.0);
    EXPECT_TRUE(manifest.srt_active_goals_.empty());
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

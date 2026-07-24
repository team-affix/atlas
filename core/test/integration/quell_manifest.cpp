// Integration: quell_manifest — wiring identity + sim lifecycle via MCTS stack
// with remaining-work reward and goal depth/work stores.

#include <cmath>
#include <gtest/gtest.h>
#include "infrastructure/db.hpp"
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/quell_manifest.hpp"
#include "infrastructure/initial_goal_exprs.hpp"
#include "value_objects/expr.hpp"
#include "value_objects/lemma.hpp"
#include "value_objects/sim_termination.hpp"
#include "functor_fixture.hpp"

namespace {

constexpr double kWorkEpsilon = 1e-9;
constexpr double kWorkDecayK = 0.2;
constexpr double kWorkDecayJ = 10.0;

double work_at_depth(size_t depth) {
    return 1.0 + std::exp(-kWorkDecayK * (static_cast<double>(depth) - kWorkDecayJ));
}

class QuellManifestIntegrationTest : public ::testing::Test {
protected:
    test_functors functors;
    static constexpr size_t kInitialVarCount = 0;
    static constexpr size_t kMaxResolutions = 64;
    static constexpr uint32_t kSeed = 42;
    static constexpr double kExplorationConstant = 1.414;

    db database;
    initial_goal_exprs initial_goals;

    expr_pool saved_expr_pool_;

    quell_manifest make_manifest(size_t max_resolutions = kMaxResolutions) {
        return quell_manifest{
            database,
            initial_goals,
            kInitialVarCount,
            max_resolutions,
            kSeed,
            kExplorationConstant,
            kWorkDecayK,
            kWorkDecayJ};
    }
};

TEST_F(QuellManifestIntegrationTest, WiringConstructsWithEmptyDbAndNoGoals) {
    EXPECT_NO_THROW(make_manifest());
}

TEST_F(QuellManifestIntegrationTest, WiringQuellActivatorsDistinctFromInnerConcrete) {
    quell_manifest manifest = make_manifest();
    EXPECT_NE(static_cast<void*>(&manifest.quell_goal_activator_),
              static_cast<void*>(&manifest.goal_activator_));
    EXPECT_NE(static_cast<void*>(&manifest.quell_goal_deactivator_),
              static_cast<void*>(&manifest.srt_goal_deactivator_));
    EXPECT_NE(static_cast<void*>(&manifest.quell_initial_goal_activator_),
              static_cast<void*>(&manifest.initial_goal_activator_));
}

TEST_F(QuellManifestIntegrationTest, WiringQuellRewardReturnsZeroInitially) {
    quell_manifest manifest = make_manifest();
    EXPECT_DOUBLE_EQ(manifest.quell_reward_.compute_mcts_reward(), 0.0);
}

TEST_F(QuellManifestIntegrationTest, WiringMctsSimDistinctFromOuterAndInnerLifecycle) {
    quell_manifest manifest = make_manifest();
    EXPECT_NE(static_cast<void*>(&manifest.set_up_sim_),
              static_cast<void*>(&manifest.mcts_sim_));
    EXPECT_NE(static_cast<void*>(&manifest.tear_down_sim_),
              static_cast<void*>(&manifest.mcts_sim_));
    EXPECT_NE(static_cast<void*>(&manifest.quell_set_up_sim_),
              static_cast<void*>(&manifest.mcts_sim_));
    EXPECT_NE(static_cast<void*>(&manifest.quell_tear_down_sim_),
              static_cast<void*>(&manifest.mcts_sim_));
}

TEST_F(QuellManifestIntegrationTest, SimLifecycleClearsRemainingWorkAfterEmptyRun) {
    quell_manifest manifest = make_manifest();
    manifest.quell_set_up_sim_.set_up();
    EXPECT_EQ(manifest.run_sim_.run(), sim_termination::solved);
    manifest.quell_tear_down_sim_.tear_down();
    EXPECT_DOUBLE_EQ(manifest.remaining_work_.get(), 0.0);
    EXPECT_TRUE(manifest.srt_active_goals_.empty());
}

TEST_F(QuellManifestIntegrationTest, InitialGoalActivationLeavesRemainingEqualToF0) {
    const expr* goal = saved_expr_pool_.make_functor(functors.id("f"), {});
    initial_goals.push(goal);
    // No matching rule → candidates may fail; drive activator path via set_up +
    // quell_initial_goal_activator alone without candidate activation.
    quell_manifest manifest = make_manifest();
    manifest.quell_set_up_sim_.set_up();
    manifest.quell_initial_goal_activator_.activate_initial_goal(0);
    EXPECT_NEAR(manifest.remaining_work_.get(), work_at_depth(0), kWorkEpsilon);
    EXPECT_NEAR(manifest.goal_work_values_.get(manifest.make_initial_goal_lineage_.make(0)),
                work_at_depth(0), kWorkEpsilon);
    EXPECT_EQ(manifest.goal_depths_.get(manifest.make_initial_goal_lineage_.make(0)), 0u);
    manifest.quell_tear_down_sim_.tear_down();
    EXPECT_DOUBLE_EQ(manifest.remaining_work_.get(), 0.0);
}

TEST_F(QuellManifestIntegrationTest, SolverFindsSingleUnitSolutionClearsRemainingWork) {
    const expr* goal = saved_expr_pool_.make_functor(functors.id("f"), {});
    const expr* head = saved_expr_pool_.make_functor(functors.id("f"), {});
    initial_goals.push(goal);
    database.push(rule{head, {}});

    quell_manifest manifest = make_manifest();
    auto sm = manifest.solver_.solve();
    sm.resume();
    ASSERT_TRUE(sm.has_yield());
    EXPECT_EQ(sm.consume_yield(), sim_termination::solved);
    EXPECT_NEAR(manifest.remaining_work_.get(), 0.0, kWorkEpsilon);
    EXPECT_NEAR(manifest.quell_reward_.compute_mcts_reward(), 0.0, kWorkEpsilon);
    sm.resume();
    EXPECT_FALSE(sm.has_yield());
}

TEST_F(QuellManifestIntegrationTest, RuntimeRemainingClearedAcrossCycles) {
    const expr* goal = saved_expr_pool_.make_functor(functors.id("f"), {});
    const expr* head = saved_expr_pool_.make_functor(functors.id("f"), {});
    initial_goals.push(goal);
    database.push(rule{head, {}});

    quell_manifest manifest = make_manifest();
    manifest.quell_set_up_sim_.set_up();
    EXPECT_EQ(manifest.run_sim_.run(), sim_termination::solved);
    EXPECT_NEAR(manifest.remaining_work_.get(), 0.0, kWorkEpsilon);
    manifest.quell_tear_down_sim_.tear_down();
    EXPECT_DOUBLE_EQ(manifest.remaining_work_.get(), 0.0);

    manifest.quell_set_up_sim_.set_up();
    EXPECT_EQ(manifest.run_sim_.run(), sim_termination::solved);
    EXPECT_NEAR(manifest.remaining_work_.get(), 0.0, kWorkEpsilon);
    manifest.quell_tear_down_sim_.tear_down();
    EXPECT_DOUBLE_EQ(manifest.remaining_work_.get(), 0.0);
}

}  // namespace

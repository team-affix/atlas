// Integration: genius_manifest / genius_runtime composition-root contracts.

#include <gtest/gtest.h>
#include "infrastructure/db.hpp"
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/genius_manifest.hpp"
#include "infrastructure/genius_runtime.hpp"
#include "infrastructure/initial_goal_exprs.hpp"
#include "value_objects/expr.hpp"
#include "value_objects/sim_termination.hpp"
#include "functor_fixture.hpp"

namespace {

constexpr double kWeightEpsilon = 1e-9;

struct GeniusManifestIntegrationTest : public ::testing::Test {
    test_functors functors;
    static constexpr size_t kMaxResolutions = 64;
    static constexpr uint32_t kSeed = 42;
    static constexpr double kExplorationConstant = 1.414;

    db database;
    initial_goal_exprs initial_goals;
    expr_pool pool;

    genius_manifest make_manifest(uint32_t initial_var_count = 0) {
        return genius_manifest{
            database, initial_goals, initial_var_count, kMaxResolutions, kSeed,
            kExplorationConstant, kExplorationConstant};
    }

    genius_runtime make_runtime(uint32_t initial_var_count = 0) {
        return genius_runtime{
            database, initial_goals, initial_var_count, kMaxResolutions, kSeed,
            kExplorationConstant, kExplorationConstant};
    }

    const expr* fun(const char* name, std::vector<const expr*> args = {}) {
        return pool.make_functor(functors.id(name), std::move(args));
    }
};

TEST_F(GeniusManifestIntegrationTest, WiringConstructsWithEmptyProblem) {
    EXPECT_NO_THROW(make_manifest());
}

TEST_F(GeniusManifestIntegrationTest, VacuousSolvedThenExhausts) {
    genius_runtime rt = make_runtime();
    ASSERT_TRUE(rt.next());
    EXPECT_TRUE(rt.solved());
    EXPECT_TRUE(rt.derive_decision_lemma().get_resolutions().empty());
    EXPECT_FALSE(rt.next());
}

TEST_F(GeniusManifestIntegrationTest, SingleUnitSolutionMakesNoDecision) {
    initial_goals.push(fun("f"));
    database.push(rule{fun("f"), {}});
    genius_runtime rt = make_runtime();
    ASSERT_TRUE(rt.next());
    EXPECT_TRUE(rt.solved());
    EXPECT_EQ(rt.decision_depth(), 0u);
    EXPECT_GE(rt.resolution_depth(), 1u);
    EXPECT_FALSE(rt.next());
}

TEST_F(GeniusManifestIntegrationTest, TrailDepthRestoresAfterEmptyRun) {
    genius_manifest m = make_manifest();
    const size_t depth_before = m.elimination_backlog_.depth();
    m.genius_set_up_sim_.set_up();
    EXPECT_EQ(m.run_sim_.run(), sim_termination::solved);
    m.genius_tear_down_sim_.tear_down();
    EXPECT_EQ(m.elimination_backlog_.depth(), depth_before);
}

TEST_F(GeniusManifestIntegrationTest, TearDownClearsDecisionMemoryAndCgw) {
    initial_goals.push(fun("f"));
    database.push(rule{fun("f"), {}});
    genius_manifest m = make_manifest();
    m.genius_set_up_sim_.set_up();
    EXPECT_EQ(m.run_sim_.run(), sim_termination::solved);
    EXPECT_GT(m.cumulative_grounded_weight_.get(), 0.0);
    m.genius_tear_down_sim_.tear_down();
    EXPECT_EQ(m.decision_memory_.count(), 0u);
    EXPECT_DOUBLE_EQ(m.cumulative_grounded_weight_.get(), 0.0);
    EXPECT_TRUE(m.srt_active_goals_.empty());
}

TEST_F(GeniusManifestIntegrationTest, FixedSeedPicksAmongCompetingFacts) {
    const expr* a = fun("a");
    const expr* b = fun("b");
    database.push(rule{fun("f", {a}), {}});
    database.push(rule{fun("f", {b}), {}});
    initial_goals.push(fun("f", {pool.make_var(0)}));

    genius_runtime rt = make_runtime(1);
    ASSERT_TRUE(rt.next());
    ASSERT_TRUE(rt.solved());
    const expr* bound = pool.import(rt.normalize({pool.make_var(0), 0}));
    EXPECT_TRUE(*bound == *a || *bound == *b);
    EXPECT_GE(rt.decision_depth(), 1u);
}

TEST_F(GeniusManifestIntegrationTest, UnitSolutionGroundsFullCgwReward) {
    initial_goals.push(fun("f"));
    database.push(rule{fun("f"), {}});
    genius_manifest m = make_manifest();
    auto sm = m.solver_.solve();
    sm.resume();
    ASSERT_TRUE(sm.has_yield());
    EXPECT_EQ(sm.consume_yield(), sim_termination::solved);
    EXPECT_NEAR(m.horizon_reward_.compute_mcts_reward(), 1.0, kWeightEpsilon);
    EXPECT_NEAR(m.cumulative_grounded_weight_.get(), 1.0, kWeightEpsilon);
}

TEST_F(GeniusManifestIntegrationTest, RuntimeCgwMatchesManifestAfterUnitSolve) {
    initial_goals.push(fun("f"));
    database.push(rule{fun("f"), {}});
    genius_runtime rt = make_runtime();
    ASSERT_TRUE(rt.next());
    ASSERT_TRUE(rt.solved());
    EXPECT_NEAR(rt.cgw(), 1.0, kWeightEpsilon);
}

}  // namespace

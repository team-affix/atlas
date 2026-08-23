// Integration: ridge_lp_manifest — wiring identity + sim lifecycle via the MCTS
// stack, with the lazy-propagation CDCL store in place of the watchlist one.

#include <gtest/gtest.h>
#include "infrastructure/db.hpp"
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/initial_goal_exprs.hpp"
#include "infrastructure/ridge_lp_manifest.hpp"
#include "value_objects/expr.hpp"
#include "value_objects/lemma.hpp"
#include "value_objects/sim_termination.hpp"
#include "functor_fixture.hpp"

namespace {

class RidgeLpManifestIntegrationTest : public ::testing::Test {
protected:
    test_functors functors;
    static constexpr size_t kInitialVarCount = 0;
    static constexpr size_t kMaxResolutions = 64;
    static constexpr uint32_t kSeed = 42;
    static constexpr double kExplorationConstant = 1.414;

    db database;
    initial_goal_exprs initial_goals;

    expr_pool saved_expr_pool_;

    ridge_lp_manifest make_manifest(size_t max_resolutions = kMaxResolutions) {
        return ridge_lp_manifest{
            database,
            initial_goals,
            kInitialVarCount,
            max_resolutions,
            kSeed,
            kExplorationConstant};
    }
};

// Tier W — wiring checks (no sim run)

TEST_F(RidgeLpManifestIntegrationTest, WiringConstructsWithEmptyDbAndNoGoals) {
    /*
     * Intent: ridge_lp_manifest constructs without throwing on an empty problem.
     * initial goals: (none)
     * rules: (none)
     */
    EXPECT_NO_THROW(make_manifest());
}

TEST_F(RidgeLpManifestIntegrationTest, WiringCdclDistinctFromJoint) {
    /*
     * Intent: cdcl_ and joint_ are distinct objects.
     * initial goals: (none)
     * rules: (none)
     */
    ridge_lp_manifest manifest = make_manifest();
    EXPECT_NE(static_cast<void*>(&manifest.cdcl_),
              static_cast<void*>(&manifest.joint_));
}

TEST_F(RidgeLpManifestIntegrationTest, WiringDecisionRecorderDistinctFromPlainRecorder) {
    /*
     * Intent: the decision slot is the lp wrapper, the unit slot the plain
     * recorder, which is what makes only decisions drive a descent.
     * initial goals: (none)
     * rules: (none)
     */
    ridge_lp_manifest manifest = make_manifest();
    EXPECT_NE(static_cast<void*>(&manifest.lp_decision_recorder_),
              static_cast<void*>(&manifest.resolution_recorder_));
}

// Tier L — sim lifecycle via MCTS-wrapped set_up/tear_down

TEST_F(RidgeLpManifestIntegrationTest, SimLifecycleTrailDepthRestoresAfterEmptyRun) {
    /*
     * Intent: MCTS set_up/tear_down on an empty problem restores trail depth.
     * initial goals: (none)
     * rules: (none)
     */
    ridge_lp_manifest manifest = make_manifest();
    const size_t depth_before = manifest.elimination_backlog_.depth();
    manifest.ridge_set_up_sim_.set_up();
    EXPECT_EQ(manifest.run_sim_.run(), sim_termination::solved);
    manifest.ridge_tear_down_sim_.tear_down();
    EXPECT_EQ(manifest.elimination_backlog_.depth(), depth_before);
}

TEST_F(RidgeLpManifestIntegrationTest, SimLifecycleClearsActiveGoalsAfterEmptyRun) {
    /*
     * Intent: tear_down clears the SRT active goal registry after a trivial solved run.
     * initial goals: (none)
     * rules: (none)
     */
    ridge_lp_manifest manifest = make_manifest();
    manifest.ridge_set_up_sim_.set_up();
    EXPECT_EQ(manifest.run_sim_.run(), sim_termination::solved);
    manifest.ridge_tear_down_sim_.tear_down();
    EXPECT_TRUE(manifest.srt_active_goals_.empty());
    EXPECT_EQ(manifest.decision_memory_.count(), 0u);
}

TEST_F(RidgeLpManifestIntegrationTest, SimLifecycleTrailDepthRestoresAfterConflictedRun) {
    /*
     * Intent: trail depth restores after a conflicted sim run with no matching rules.
     * initial goals: f.
     * rules: (none)
     */
    const expr* goal = saved_expr_pool_.make_functor(functors.id("f"), {});
    initial_goals.push(goal);
    ridge_lp_manifest manifest = make_manifest();
    const size_t depth_before = manifest.elimination_backlog_.depth();
    manifest.ridge_set_up_sim_.set_up();
    EXPECT_EQ(manifest.run_sim_.run(), sim_termination::conflicted);
    manifest.ridge_tear_down_sim_.tear_down();
    EXPECT_EQ(manifest.elimination_backlog_.depth(), depth_before);
}

TEST_F(RidgeLpManifestIntegrationTest, SimMctsDecisionGeneratorRecordsDecisionWithFixedSeed) {
    /*
     * Intent: the lp decision recorder still records into decision_memory while
     * also driving descent.
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

    ridge_lp_manifest manifest = make_manifest();
    manifest.ridge_set_up_sim_.set_up();
    EXPECT_EQ(manifest.run_sim_.run(), sim_termination::solved);
    EXPECT_EQ(manifest.decision_memory_.count(), 1u);
    const lemma dl = manifest.decision_memory_.derive_decision_lemma();
    ASSERT_EQ(dl.get_resolutions().size(), 1u);
    const rule_id chosen = (*dl.get_resolutions().begin())->idx;
    EXPECT_TRUE(chosen == 0 || chosen == 1);
    manifest.ridge_tear_down_sim_.tear_down();
}

// Tier X — single solver tick

TEST_F(RidgeLpManifestIntegrationTest, SolverVacuousSolvedOnEmptyProblem) {
    /*
     * Intent: solver yields one vacuous solved tick on an empty problem and completes.
     * initial goals: (none)
     * rules: (none)
     */
    ridge_lp_manifest manifest = make_manifest();
    auto sm = manifest.solver_.solve();
    sm.resume();
    ASSERT_TRUE(sm.has_yield());
    EXPECT_EQ(sm.consume_yield(), sim_termination::solved);
    EXPECT_TRUE(manifest.decision_memory_.derive_decision_lemma().get_resolutions().empty());
    sm.resume();
    EXPECT_FALSE(sm.has_yield());
}

TEST_F(RidgeLpManifestIntegrationTest, SolverFindsSingleUnitSolution) {
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

    ridge_lp_manifest manifest = make_manifest();
    auto sm = manifest.solver_.solve();
    sm.resume();
    ASSERT_TRUE(sm.has_yield());
    EXPECT_EQ(sm.consume_yield(), sim_termination::solved);
    EXPECT_TRUE(manifest.decision_memory_.derive_decision_lemma().get_resolutions().empty());
    sm.resume();
    EXPECT_FALSE(sm.has_yield());
}

}  // namespace

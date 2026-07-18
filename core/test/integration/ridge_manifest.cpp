// Integration: ridge_manifest — wiring identity + sim lifecycle via MCTS stack.

#include <gtest/gtest.h>
#include <stdexcept>
#include "infrastructure/db.hpp"
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/initial_goal_exprs.hpp"
#include "infrastructure/ridge_manifest.hpp"
#include "value_objects/expr.hpp"
#include "value_objects/lemma.hpp"
#include "value_objects/sim_termination.hpp"
#include "functor_fixture.hpp"

namespace {

class RidgeManifestIntegrationTest : public ::testing::Test {
protected:
    test_functors functors;
    static constexpr size_t kInitialVarCount = 0;
    static constexpr size_t kMaxResolutions = 64;
    static constexpr uint32_t kSeed = 42;
    static constexpr double kExplorationConstant = 1.414;

    db database;
    initial_goal_exprs initial_goals;

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

// Tier W — wiring checks (no sim run)

TEST_F(RidgeManifestIntegrationTest, WiringConstructsWithEmptyDbAndNoGoals) {
    /*
     * Intent: ridge_manifest constructs without throwing on an empty problem.
     * initial goals: (none)
     * rules: (none)
     */
    EXPECT_NO_THROW(make_manifest());
}

TEST_F(RidgeManifestIntegrationTest, WiringMctsSimDistinctFromOuterAndInnerLifecycle) {
    /*
     * Intent: mcts_sim_ is distinct from outer and base setup/teardown components.
     * initial goals: (none)
     * rules: (none)
     */
    ridge_manifest manifest = make_manifest();
    EXPECT_NE(static_cast<void*>(&manifest.set_up_sim_),
              static_cast<void*>(&manifest.mcts_sim_));
    EXPECT_NE(static_cast<void*>(&manifest.tear_down_sim_),
              static_cast<void*>(&manifest.mcts_sim_));
    EXPECT_NE(static_cast<void*>(&manifest.ridge_set_up_sim_),
              static_cast<void*>(&manifest.mcts_sim_));
    EXPECT_NE(static_cast<void*>(&manifest.ridge_tear_down_sim_),
              static_cast<void*>(&manifest.mcts_sim_));
}

TEST_F(RidgeManifestIntegrationTest, WiringCdclDistinctFromJoint) {
    /*
     * Intent: cdcl_ and joint_ are distinct objects.
     * initial goals: (none)
     * rules: (none)
     */
    ridge_manifest manifest = make_manifest();
    EXPECT_NE(static_cast<void*>(&manifest.cdcl_),
              static_cast<void*>(&manifest.joint_));
}

// Tier L — sim lifecycle via MCTS-wrapped set_up/tear_down

TEST_F(RidgeManifestIntegrationTest, SimLifecycleTrailDepthRestoresAfterEmptyRun) {
    /*
     * Intent: MCTS set_up/tear_down on an empty problem restores trail depth.
     * initial goals: (none)
     * rules: (none)
     */
    ridge_manifest manifest = make_manifest();
    const size_t depth_before = manifest.elimination_backlog_.depth();
    manifest.ridge_set_up_sim_.set_up();
    EXPECT_EQ(manifest.run_sim_.run(), sim_termination::solved);
    manifest.ridge_tear_down_sim_.tear_down();
    EXPECT_EQ(manifest.elimination_backlog_.depth(), depth_before);
}

TEST_F(RidgeManifestIntegrationTest, SimLifecycleClearsActiveGoalsAfterEmptyRun) {
    /*
     * Intent: tear_down clears the SRT active goal registry after a trivial solved run.
     * initial goals: (none)
     * rules: (none)
     */
    ridge_manifest manifest = make_manifest();
    manifest.ridge_set_up_sim_.set_up();
    EXPECT_EQ(manifest.run_sim_.run(), sim_termination::solved);
    manifest.ridge_tear_down_sim_.tear_down();
    EXPECT_TRUE(manifest.srt_active_goals_.empty());
    EXPECT_EQ(manifest.decision_memory_.count(), 0u);
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
    const size_t depth_before = manifest.elimination_backlog_.depth();
    manifest.ridge_set_up_sim_.set_up();
    EXPECT_EQ(manifest.run_sim_.run(), sim_termination::conflicted);
    manifest.ridge_tear_down_sim_.tear_down();
    EXPECT_EQ(manifest.elimination_backlog_.depth(), depth_before);
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

TEST_F(RidgeManifestIntegrationTest, InitialActivateSetsFewerCandidateScore) {
    /*
     * Intent: after set_up + initial activate, root f with two fact candidates
     * scores -2.0 while it remains an active leaf (decision needed).
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
    manifest.ridge_set_up_sim_.set_up();
    ASSERT_TRUE(manifest.srt_initial_goals_activator_.activate_initial_goals_and_candidates());

    ASSERT_EQ(manifest.srt_active_goals_.active_goals_size(), 1u);
    const goal_lineage* root = nullptr;
    auto root_sm = manifest.srt_active_goals_.iterate_root_goals();
    while (!root_sm.done()) {
        root_sm.resume();
        if (root_sm.has_yield())
            root = root_sm.consume_yield();
    }
    ASSERT_NE(root, nullptr);
    EXPECT_TRUE(manifest.srt_active_goals_.is_active_goal(root));
    EXPECT_EQ(manifest.rp_srt_active_goals_.get(root), -2.0);

    manifest.ridge_tear_down_sim_.tear_down();
}

TEST_F(RidgeManifestIntegrationTest, TearDownClearsRpScores) {
    /*
     * Intent: tear_down clears RP score map along with SRT active goals.
     * initial goals: (none)
     * rules: (none)
     */
    ridge_manifest manifest = make_manifest();
    manifest.ridge_set_up_sim_.set_up();
    EXPECT_EQ(manifest.run_sim_.run(), sim_termination::solved);

    goal_lineage sentinel{nullptr, 99};
    manifest.rp_srt_active_goals_.insert_active_goal(&sentinel);
    EXPECT_EQ(manifest.rp_srt_active_goals_.get(&sentinel), 0.0);

    manifest.ridge_tear_down_sim_.tear_down();

    EXPECT_TRUE(manifest.srt_active_goals_.empty());
    EXPECT_THROW(manifest.rp_srt_active_goals_.get(&sentinel), std::out_of_range);
}

}  // namespace

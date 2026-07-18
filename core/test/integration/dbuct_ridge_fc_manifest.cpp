// Integration: dbuct_ridge_fc_manifest — fewer-candidate scores + frame undo.

#include <gtest/gtest.h>
#include <stdexcept>
#include "infrastructure/db.hpp"
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/initial_goal_exprs.hpp"
#include "infrastructure/dbuct_ridge_fc_manifest.hpp"
#include "value_objects/expr.hpp"
#include "value_objects/lineage.hpp"
#include "functor_fixture.hpp"

namespace {

class DbuctRidgeFcManifestIntegrationTest : public ::testing::Test {
protected:
    test_functors functors;
    static constexpr size_t kInitialVarCount = 0;
    static constexpr size_t kMaxResolutions = 64;
    static constexpr uint32_t kSeed = 42;
    static constexpr double kExplorationConstant = 1.414;
    static constexpr size_t kGrantInterval = 4;

    db database;
    initial_goal_exprs initial_goals;
    expr_pool saved_expr_pool_;

    dbuct_ridge_fc_manifest make_manifest(size_t max_resolutions = kMaxResolutions) {
        return dbuct_ridge_fc_manifest{
            database,
            initial_goals,
            kInitialVarCount,
            max_resolutions,
            kSeed,
            kExplorationConstant,
            kGrantInterval};
    }
};

TEST_F(DbuctRidgeFcManifestIntegrationTest, InitialActivateSetsFewerCandidateScore) {
    /*
     * Intent: after initial activate, root f with two fact candidates scores
     * -2.0 while it remains an active leaf (decision needed).
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

    dbuct_ridge_fc_manifest manifest = make_manifest();
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
}

TEST_F(DbuctRidgeFcManifestIntegrationTest, FramePopUndoesRpScores) {
    /*
     * Intent: DBUCT undoes RP scores via frames (no clear_active_goals).
     * Nested push/set/pop restores the outer score map.
     */
    dbuct_ridge_fc_manifest manifest = make_manifest();

    goal_lineage sentinel{nullptr, 99};
    manifest.rp_srt_active_goals_.insert_active_goal(&sentinel);
    EXPECT_EQ(manifest.rp_srt_active_goals_.get(&sentinel), 0.0);

    manifest.rp_srt_active_goals_.push_frame();
    manifest.rp_srt_active_goals_.set_active_goal_value(&sentinel, -7.0);
    EXPECT_EQ(manifest.rp_srt_active_goals_.get(&sentinel), -7.0);

    goal_lineage nested{nullptr, 100};
    manifest.rp_srt_active_goals_.insert_active_goal(&nested);
    EXPECT_EQ(manifest.rp_srt_active_goals_.get(&nested), 0.0);

    manifest.rp_srt_active_goals_.pop_frame();
    EXPECT_EQ(manifest.rp_srt_active_goals_.get(&sentinel), 0.0);
    EXPECT_THROW(manifest.rp_srt_active_goals_.get(&nested), std::out_of_range);
}

TEST_F(DbuctRidgeFcManifestIntegrationTest, WiringConstructsWithEmptyProblem) {
    EXPECT_NO_THROW(make_manifest());
}

}  // namespace

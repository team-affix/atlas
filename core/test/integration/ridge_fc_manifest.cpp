// Integration: ridge_fc_manifest — fewer-candidate score wiring on ridge reward.

#include <gtest/gtest.h>
#include <stdexcept>
#include "infrastructure/db.hpp"
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/initial_goal_exprs.hpp"
#include "infrastructure/ridge_fc_manifest.hpp"
#include "value_objects/expr.hpp"
#include "value_objects/lineage.hpp"
#include "value_objects/sim_termination.hpp"
#include "functor_fixture.hpp"

namespace {

class RidgeFcManifestIntegrationTest : public ::testing::Test {
protected:
    test_functors functors;
    static constexpr size_t kInitialVarCount = 0;
    static constexpr size_t kMaxResolutions = 64;
    static constexpr uint32_t kSeed = 42;
    static constexpr double kExplorationConstant = 1.414;

    db database;
    initial_goal_exprs initial_goals;

    expr_pool saved_expr_pool_;

    ridge_fc_manifest make_manifest(size_t max_resolutions = kMaxResolutions) {
        return ridge_fc_manifest{
            database,
            initial_goals,
            kInitialVarCount,
            max_resolutions,
            kSeed,
            kExplorationConstant};
    }
};

TEST_F(RidgeFcManifestIntegrationTest, InitialActivateSetsFewerCandidateScore) {
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

    ridge_fc_manifest manifest = make_manifest();
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

TEST_F(RidgeFcManifestIntegrationTest, TearDownClearsRpScores) {
    /*
     * Intent: tear_down clears RP score map along with SRT active goals.
     * initial goals: (none)
     * rules: (none)
     */
    ridge_fc_manifest manifest = make_manifest();
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

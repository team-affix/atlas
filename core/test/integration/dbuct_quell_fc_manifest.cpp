// Integration: dbuct_quell_fc_manifest — FC scores + quell stores + frame undo (camping).

#include <gtest/gtest.h>
#include <stdexcept>
#include "infrastructure/db.hpp"
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/initial_goal_exprs.hpp"
#include "infrastructure/dbuct_quell_fc_manifest.hpp"
#include "value_objects/expr.hpp"
#include "value_objects/lineage.hpp"
#include "functor_fixture.hpp"

namespace {

class DbuctQuellFcManifestIntegrationTest : public ::testing::Test {
protected:
    test_functors functors;
    static constexpr size_t kInitialVarCount = 0;
    static constexpr size_t kMaxResolutions = 64;
    static constexpr uint32_t kSeed = 42;
    static constexpr double kExplorationConstant = 1.414;
    static constexpr double kWorkDecayK = 0.2;
    static constexpr double kWorkDecayJ = 10.0;
    static constexpr size_t kGrantInterval = 4;

    db database;
    initial_goal_exprs initial_goals;
    expr_pool saved_expr_pool_;

    dbuct_quell_fc_manifest make_manifest(size_t max_resolutions = kMaxResolutions) {
        return dbuct_quell_fc_manifest{
            database,
            initial_goals,
            kInitialVarCount,
            max_resolutions,
            kSeed,
            kExplorationConstant,
            kWorkDecayK,
            kWorkDecayJ,
            kGrantInterval};
    }
};

TEST_F(DbuctQuellFcManifestIntegrationTest, InitialActivateSetsFewerCandidateScore) {
    const expr* goal = saved_expr_pool_.make_functor(functors.id("f"), {});
    const expr* f_head0 = saved_expr_pool_.make_functor(functors.id("f"), {});
    const expr* f_head1 = saved_expr_pool_.make_functor(functors.id("f"), {});
    initial_goals.push(goal);
    database.push(rule{f_head0, {}});
    database.push(rule{f_head1, {}});

    dbuct_quell_fc_manifest manifest = make_manifest();
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

TEST_F(DbuctQuellFcManifestIntegrationTest, FramePopUndoesRpScores) {
    dbuct_quell_fc_manifest manifest = make_manifest();

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

TEST_F(DbuctQuellFcManifestIntegrationTest, WiringConstructsWithEmptyProblem) {
    EXPECT_NO_THROW(make_manifest());
}

TEST_F(DbuctQuellFcManifestIntegrationTest, QuellHubPopRestoresDepthsWorkValuesAndRemainingWork) {
    dbuct_quell_fc_manifest m = make_manifest();
    goal_lineage gl{nullptr, 0};
    m.quell_hub_.push_solver_frame();
    m.goal_depths_.set(&gl, 1);
    m.goal_work_values_.set(&gl, 0.5);
    m.remaining_work_.add(0.5);
    EXPECT_EQ(m.goal_depths_.get(&gl), 1u);
    EXPECT_DOUBLE_EQ(m.goal_work_values_.get(&gl), 0.5);
    EXPECT_DOUBLE_EQ(m.remaining_work_.get(), 0.5);

    auto sm = m.quell_hub_.pop_solver_frame();
    while (!sm.done())
        sm.resume();

    EXPECT_THROW(m.goal_depths_.get(&gl), std::out_of_range);
    EXPECT_THROW(m.goal_work_values_.get(&gl), std::out_of_range);
    EXPECT_DOUBLE_EQ(m.remaining_work_.get(), 0.0);
}

}  // namespace

// Integration: rp_fewer_candidate_goal_candidates_activator sets initial leaf scores.

#include <gtest/gtest.h>
#include <vector>
#include "infrastructure/goal_candidate_rules.hpp"
#include "infrastructure/ra_rule_id_set_factory.hpp"
#include "infrastructure/rp_compute_fewer_candidate_goal_value.hpp"
#include "infrastructure/rp_fewer_candidate_goal_candidates_activator.hpp"
#include "infrastructure/rp_srt_active_goals.hpp"
#include "infrastructure/srt_active_goals.hpp"
#include "infrastructure/srt_initial_goals_activator.hpp"
#include "value_objects/lineage.hpp"

namespace {

using rp_srt_t = rp_srt_active_goals<
    srt_active_goals, srt_active_goals, srt_active_goals, srt_active_goals,
    srt_active_goals>;
using compute_t = rp_compute_fewer_candidate_goal_value<goal_candidate_rules>;

struct HandRollGoalCandidatesActivator {
    goal_candidate_rules* rules;
    std::vector<rule_id> ids_to_link;
    bool succeed;

    bool activate_goal_candidates(const goal_lineage* gl) {
        if (!succeed) return false;
        for (rule_id id : ids_to_link)
            rules->link_goal_candidate(gl, id);
        return true;
    }
};

using rp_activator_t = rp_fewer_candidate_goal_candidates_activator<
    HandRollGoalCandidatesActivator, compute_t, rp_srt_t>;

struct FlushOnlyInitial {
    bool activate_initial_goals_and_candidates() { return true; }
};

using flush_activator_t = srt_initial_goals_activator<srt_active_goals, FlushOnlyInitial>;

}  // namespace

struct RpFewerCandidateInitialGoalScoresIntegrationTest : public ::testing::Test {
    srt_active_goals srt;
    rp_srt_t rp{srt, srt, srt, srt, srt};
    ra_rule_id_set_factory factory;
    goal_candidate_rules rules{factory};
    compute_t compute{rules};
    HandRollGoalCandidatesActivator inner{&rules, {}, true};
    rp_activator_t activator{inner, compute, rp};
    FlushOnlyInitial flush_inner;
    flush_activator_t flusher{srt, flush_inner};

    goal_lineage root{nullptr, 0};
    goal_lineage r0{nullptr, 1};
    goal_lineage r1{nullptr, 2};

    void prepare_root(const goal_lineage* gl) {
        rp.insert_active_goal(gl);
        rules.insert(gl);
    }
};

TEST_F(RpFewerCandidateInitialGoalScoresIntegrationTest, SingleRootScoreIsNegCount) {
    prepare_root(&root);
    inner.ids_to_link = {0, 1, 2, 3};
    EXPECT_TRUE(activator.activate_goal_candidates(&root));
    EXPECT_TRUE(flusher.activate_initial_goals_and_candidates());

    EXPECT_EQ(rp.get(&root), -4.0);
    EXPECT_TRUE(srt.is_active_goal(&root));
    EXPECT_EQ(srt.get_parent_goal(&root), nullptr);
}

TEST_F(RpFewerCandidateInitialGoalScoresIntegrationTest, TwoRootsIndependentScores) {
    prepare_root(&r0);
    prepare_root(&r1);
    srt.flush_srt_goal_batch();

    inner.ids_to_link = {0};
    EXPECT_TRUE(activator.activate_goal_candidates(&r0));

    inner.ids_to_link = {0, 1, 2};
    EXPECT_TRUE(activator.activate_goal_candidates(&r1));

    EXPECT_TRUE(srt.is_active_goal(&r0));
    EXPECT_TRUE(srt.is_active_goal(&r1));
    EXPECT_EQ(rp.get(&r0), -1.0);
    EXPECT_EQ(rp.get(&r1), -3.0);
}

TEST_F(RpFewerCandidateInitialGoalScoresIntegrationTest, FailedActivateLeavesDefaultZero) {
    prepare_root(&root);
    srt.flush_srt_goal_batch();
    inner.succeed = false;
    inner.ids_to_link = {0, 1, 2, 3};
    EXPECT_FALSE(activator.activate_goal_candidates(&root));
    EXPECT_EQ(rp.get(&root), 0.0);
}

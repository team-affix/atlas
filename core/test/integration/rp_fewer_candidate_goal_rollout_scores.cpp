// Integration: rp_fewer_candidate_goal_rollout over real rp_srt_active_goals scores.

#include <gtest/gtest.h>
#include <random>
#include <set>
#include <vector>
#include "infrastructure/rp_fewer_candidate_goal_rollout.hpp"
#include "infrastructure/rp_heuristic_rollout.hpp"
#include "infrastructure/rp_srt_active_goals.hpp"
#include "infrastructure/rp_uniform_rule_rollout.hpp"
#include "infrastructure/srt_active_goals.hpp"
#include "value_objects/lineage.hpp"
#include "value_objects/mcts_choice.hpp"

namespace {

using rp_srt_t = rp_srt_active_goals<
    srt_active_goals, srt_active_goals, srt_active_goals, srt_active_goals,
    srt_active_goals>;
using goal_rollout_t = rp_fewer_candidate_goal_rollout<rp_srt_t>;
using heuristic_t = rp_heuristic_rollout<goal_rollout_t, rp_uniform_rule_rollout<std::mt19937>>;

struct ChoiceList {
    std::vector<mcts_choice> choices;
    size_t size() const { return choices.size(); }
    mcts_choice at(size_t i) const { return choices[i]; }
};

}  // namespace

struct RpFewerCandidateGoalRolloutScoresIntegrationTest : public ::testing::Test {
    srt_active_goals srt;
    rp_srt_t rp{srt, srt, srt, srt, srt};
    goal_rollout_t goal_rollout{rp};
    std::mt19937 rng{42};
    rp_uniform_rule_rollout<std::mt19937> rule_rollout{rng};
    heuristic_t heuristic{goal_rollout, rule_rollout};

    goal_lineage g0{nullptr, 0};
    goal_lineage g1{nullptr, 1};

    void insert_scored(const goal_lineage* gl, double score) {
        rp.insert_active_goal(gl);
        srt.flush_srt_goal_batch();
        rp.set_active_goal_value(gl, score);
    }
};

TEST_F(RpFewerCandidateGoalRolloutScoresIntegrationTest, ChoosesHigherScoredGoalFromStore) {
    insert_scored(&g0, -10.0);
    insert_scored(&g1, -2.0);
    const std::vector<const goal_lineage*> goals{&g0, &g1};
    EXPECT_EQ(goal_rollout.rollout_choose_goal(goals), &g1);
}

TEST_F(RpFewerCandidateGoalRolloutScoresIntegrationTest, AfterElimStyleRefreshChoiceChanges) {
    insert_scored(&g0, -3.0);
    insert_scored(&g1, -3.0);
    rp.set_active_goal_value(&g0, -1.0);
    const std::vector<const goal_lineage*> goals{&g0, &g1};
    EXPECT_EQ(goal_rollout.rollout_choose_goal(goals), &g0);
}

TEST_F(RpFewerCandidateGoalRolloutScoresIntegrationTest, HeuristicDispatchUsesGoalScores) {
    insert_scored(&g0, -10.0);
    insert_scored(&g1, -2.0);
    ChoiceList list;
    list.choices = {&g0, &g1};
    const mcts_choice result = heuristic.rollout_choose(list, list);
    EXPECT_EQ(std::get<const goal_lineage*>(result), &g1);
}

TEST_F(RpFewerCandidateGoalRolloutScoresIntegrationTest, HeuristicDispatchRuleBranch) {
    ChoiceList list;
    list.choices = {rule_id{10}, rule_id{20}};
    const mcts_choice result = heuristic.rollout_choose(list, list);
    const rule_id chosen = std::get<rule_id>(result);
    const std::set<rule_id> allowed{10, 20};
    EXPECT_NE(allowed.find(chosen), allowed.end());
}

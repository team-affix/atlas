// fewer_candidate_goal_rollout: argmax over stored heuristic scores.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <vector>
#include "infrastructure/fewer_candidate_goal_rollout.hpp"
#include "value_objects/lineage.hpp"

using ::testing::NiceMock;
using ::testing::Return;

namespace {

struct MockGetGoalHeuristicScore {
    MOCK_METHOD(double, get, (const goal_lineage*), (const));
};

}  // namespace

struct FewerCandidateGoalRolloutTest : public ::testing::Test {
    NiceMock<MockGetGoalHeuristicScore> scores;
    fewer_candidate_goal_rollout<MockGetGoalHeuristicScore> rollout{scores};
    goal_lineage a{nullptr, 0};
    goal_lineage b{nullptr, 1};
    goal_lineage c{nullptr, 2};
};

TEST_F(FewerCandidateGoalRolloutTest, PicksMaxScoringGoal) {
    EXPECT_CALL(scores, get(&a)).WillOnce(Return(-5.0));
    EXPECT_CALL(scores, get(&b)).WillOnce(Return(-1.0));
    EXPECT_CALL(scores, get(&c)).WillOnce(Return(-3.0));
    const std::vector<const goal_lineage*> goals{&a, &b, &c};
    EXPECT_EQ(rollout.rollout_choose_goal(goals), &b);
}

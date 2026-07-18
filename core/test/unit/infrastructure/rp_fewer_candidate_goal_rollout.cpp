// rp_fewer_candidate_goal_rollout: argmax over stored heuristic scores.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <limits>
#include <vector>
#include "infrastructure/rp_fewer_candidate_goal_rollout.hpp"
#include "value_objects/lineage.hpp"

using ::testing::NiceMock;
using ::testing::Return;

namespace {

constexpr double kNegInf = -std::numeric_limits<double>::infinity();

struct MockGetGoalHeuristicScore {
    MOCK_METHOD(double, get, (const goal_lineage*), (const));
};

}  // namespace

struct RpFewerCandidateGoalRolloutTest : public ::testing::Test {
    NiceMock<MockGetGoalHeuristicScore> scores;
    rp_fewer_candidate_goal_rollout<MockGetGoalHeuristicScore> rollout{scores};
    goal_lineage a{nullptr, 0};
    goal_lineage b{nullptr, 1};
    goal_lineage c{nullptr, 2};
};

TEST_F(RpFewerCandidateGoalRolloutTest, PicksMaxScoringGoal) {
    EXPECT_CALL(scores, get(&a)).WillOnce(Return(-5.0));
    EXPECT_CALL(scores, get(&b)).WillOnce(Return(-1.0));
    EXPECT_CALL(scores, get(&c)).WillOnce(Return(-3.0));
    const std::vector<const goal_lineage*> goals{&a, &b, &c};
    EXPECT_EQ(rollout.rollout_choose_goal(goals), &b);
}

TEST_F(RpFewerCandidateGoalRolloutTest, SingleGoalReturnsThatGoal) {
    EXPECT_CALL(scores, get(&a)).WillOnce(Return(-4.0));
    const std::vector<const goal_lineage*> goals{&a};
    EXPECT_EQ(rollout.rollout_choose_goal(goals), &a);
}

TEST_F(RpFewerCandidateGoalRolloutTest, TieKeepsFirstIndex) {
    EXPECT_CALL(scores, get(&a)).WillOnce(Return(-2.0));
    EXPECT_CALL(scores, get(&b)).WillOnce(Return(-2.0));
    EXPECT_CALL(scores, get(&c)).WillOnce(Return(-5.0));
    const std::vector<const goal_lineage*> goals{&a, &b, &c};
    EXPECT_EQ(rollout.rollout_choose_goal(goals), &a);
}

TEST_F(RpFewerCandidateGoalRolloutTest, AllNegInfReturnsFirst) {
    EXPECT_CALL(scores, get(&a)).WillOnce(Return(kNegInf));
    EXPECT_CALL(scores, get(&b)).WillOnce(Return(kNegInf));
    EXPECT_CALL(scores, get(&c)).WillOnce(Return(kNegInf));
    const std::vector<const goal_lineage*> goals{&a, &b, &c};
    EXPECT_EQ(rollout.rollout_choose_goal(goals), &a);
}

TEST_F(RpFewerCandidateGoalRolloutTest, FiniteBeatsNegInf) {
    EXPECT_CALL(scores, get(&a)).WillOnce(Return(kNegInf));
    EXPECT_CALL(scores, get(&b)).WillOnce(Return(-9.0));
    const std::vector<const goal_lineage*> goals{&a, &b};
    EXPECT_EQ(rollout.rollout_choose_goal(goals), &b);
}

TEST_F(RpFewerCandidateGoalRolloutTest, HigherIsBetterEvenIfMoreNegative) {
    EXPECT_CALL(scores, get(&a)).WillOnce(Return(-10.0));
    EXPECT_CALL(scores, get(&b)).WillOnce(Return(-3.0));
    const std::vector<const goal_lineage*> goals{&a, &b};
    EXPECT_EQ(rollout.rollout_choose_goal(goals), &b);
}

// initial_goals_activator: per-initial-goal activation + goal_candidates.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/initial_goals_activator.hpp"

using ::testing::Return;

struct MockGetInitialGoalCount {
    MOCK_METHOD(size_t, count, (), (const));
};

struct MockActivateInitialGoal {
    MOCK_METHOD(void, activate_initial_goal, (subgoal_id));
};

struct MockMakeInitialGoalLineage {
    MOCK_METHOD(const goal_lineage*, make, (subgoal_id));
};

struct MockActivateGoalCandidates {
    MOCK_METHOD(bool, activate_goal_candidates, (const goal_lineage*));
};

using test_initial_goals_activator_t = initial_goals_activator<
    MockGetInitialGoalCount, MockActivateInitialGoal,
    MockMakeInitialGoalLineage, MockActivateGoalCandidates>;

struct InitialGoalsActivatorTest : public ::testing::Test {
    MockGetInitialGoalCount get_initial_goal_count;
    MockActivateInitialGoal activate_initial_goal;
    MockMakeInitialGoalLineage make_initial_goal_lineage;
    MockActivateGoalCandidates activate_goal_candidates;
    test_initial_goals_activator_t activator{get_initial_goal_count, activate_initial_goal,
                                        make_initial_goal_lineage, activate_goal_candidates};

    goal_lineage gl{nullptr, 0};
};

TEST_F(InitialGoalsActivatorTest, ReturnsFalseWhenGoalHasNoCandidates) {
    EXPECT_CALL(get_initial_goal_count, count()).WillRepeatedly(Return(1));
    EXPECT_CALL(activate_initial_goal, activate_initial_goal(0)).Times(1);
    EXPECT_CALL(make_initial_goal_lineage, make(0)).WillOnce(Return(&gl));
    EXPECT_CALL(activate_goal_candidates, activate_goal_candidates(&gl)).WillOnce(Return(false));
    EXPECT_FALSE(activator.activate_initial_goals_and_candidates());
}

TEST_F(InitialGoalsActivatorTest, ActivatesEachInitialGoal) {
    goal_lineage gl0{nullptr, 0};
    goal_lineage gl1{nullptr, 1};
    EXPECT_CALL(get_initial_goal_count, count()).WillRepeatedly(Return(2));
    EXPECT_CALL(activate_initial_goal, activate_initial_goal(0)).Times(1);
    EXPECT_CALL(activate_initial_goal, activate_initial_goal(1)).Times(1);
    EXPECT_CALL(make_initial_goal_lineage, make(0)).WillOnce(Return(&gl0));
    EXPECT_CALL(make_initial_goal_lineage, make(1)).WillOnce(Return(&gl1));
    EXPECT_CALL(activate_goal_candidates, activate_goal_candidates(&gl0)).WillOnce(Return(true));
    EXPECT_CALL(activate_goal_candidates, activate_goal_candidates(&gl1)).WillOnce(Return(true));
    EXPECT_TRUE(activator.activate_initial_goals_and_candidates());
}

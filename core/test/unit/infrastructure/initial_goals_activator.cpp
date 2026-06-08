// initial_goals_activator: per-initial-goal activation + goal_candidates.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "locator_fixture.hpp"
#include "infrastructure/initial_goals_activator.hpp"
#include "interfaces/i_get_initial_goal_count.hpp"
#include "interfaces/i_activate_initial_goal.hpp"
#include "interfaces/i_make_initial_goal_lineage.hpp"
#include "interfaces/i_activate_goal_candidates.hpp"

using ::testing::Return;

struct MockGetInitialGoalCount : public i_get_initial_goal_count {
    MOCK_METHOD(size_t, count, (), (const, override));
};

struct MockActivateInitialGoal : public i_activate_initial_goal {
    MOCK_METHOD(void, activate_initial_goal, (subgoal_id), (override));
};

struct MockMakeInitialGoalLineage : public i_make_initial_goal_lineage {
    MOCK_METHOD(const goal_lineage*, make, (subgoal_id), (override));
};

struct MockActivateGoalCandidates : public i_activate_goal_candidates {
    MOCK_METHOD(bool, activate_goal_candidates, (const goal_lineage*), (override));
};

struct InitialGoalsActivatorTest : public ::testing::Test {
    locator loc;
    MockGetInitialGoalCount get_initial_goal_count;
    MockActivateInitialGoal activate_initial_goal;
    MockMakeInitialGoalLineage make_initial_goal_lineage;
    MockActivateGoalCandidates activate_goal_candidates;
    initial_goals_activator activator;

    InitialGoalsActivatorTest() : activator(init_activator()) {}

    initial_goals_activator init_activator() {
        loc.bind_as<i_get_initial_goal_count>(get_initial_goal_count);
        loc.bind_as<i_activate_initial_goal>(activate_initial_goal);
        loc.bind_as<i_make_initial_goal_lineage>(make_initial_goal_lineage);
        loc.bind_as<i_activate_goal_candidates>(activate_goal_candidates);
        return initial_goals_activator{loc};
    }

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

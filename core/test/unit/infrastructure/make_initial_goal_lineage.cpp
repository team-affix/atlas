// make_initial_goal_lineage: root goals use a null resolution parent.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/make_initial_goal_lineage.hpp"

using ::testing::Return;

struct MockMakeGoalLineage {
    MOCK_METHOD(const goal_lineage*, make_goal_lineage,
        (const resolution_lineage*, subgoal_id));
};

using test_make_initial_goal_lineage_t = make_initial_goal_lineage<MockMakeGoalLineage>;

struct MakeInitialGoalLineageTest : public ::testing::Test {
    static constexpr subgoal_id kIdx = 0;

    MockMakeGoalLineage make_goal_lineage;
    test_make_initial_goal_lineage_t maker{make_goal_lineage};

    goal_lineage gl0{nullptr, kIdx};
};

TEST_F(MakeInitialGoalLineageTest, MakeUsesNullParent) {
    EXPECT_CALL(make_goal_lineage, make_goal_lineage(nullptr, kIdx)).WillOnce(Return(&gl0));
    EXPECT_EQ(maker.make(kIdx), &gl0);
}

TEST_F(MakeInitialGoalLineageTest, MakeForSecondSubgoalForwardsIndex) {
    static constexpr subgoal_id kAltIdx = 2;
    goal_lineage gl2{nullptr, kAltIdx};
    EXPECT_CALL(make_goal_lineage, make_goal_lineage(nullptr, kAltIdx)).WillOnce(Return(&gl2));
    EXPECT_EQ(maker.make(kAltIdx), &gl2);
}

// make_initial_goal_lineage: root goals use a null resolution parent.

#include <gtest/gtest.h>
#include "locator_fixture.hpp"
#include <gmock/gmock.h>
#include "infrastructure/make_initial_goal_lineage.hpp"
#include "interfaces/i_make_goal_lineage.hpp"

using ::testing::Return;

struct MockMakeGoalLineage : public i_make_goal_lineage {
    MOCK_METHOD(const goal_lineage*, make_goal_lineage,
        (const resolution_lineage*, subgoal_id), (override));
};

struct MakeInitialGoalLineageTest : public ::testing::Test {
    static constexpr subgoal_id kIdx = 0;

    locator loc;
    MockMakeGoalLineage make_goal_lineage;
    make_initial_goal_lineage maker;

    MakeInitialGoalLineageTest()
        : maker(bind_and_make<make_initial_goal_lineage, i_make_goal_lineage>(
              loc, make_goal_lineage)) {}
    goal_lineage gl0{nullptr, kIdx};
};

TEST_F(MakeInitialGoalLineageTest, MakeUsesNullParent) {
    EXPECT_CALL(make_goal_lineage, make_goal_lineage(nullptr, kIdx)).WillOnce(Return(&gl0));
    EXPECT_EQ(maker.make(kIdx), &gl0);
}

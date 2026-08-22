// depth_tracking_quell_initial_goal_activator: records depth 0 for the initial
// goal before handing the index to the wrapped quell activator.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/depth_tracking_quell_initial_goal_activator.hpp"
#include "value_objects/expr.hpp"

using ::testing::AtLeast;
using ::testing::Expectation;
using ::testing::NiceMock;
using ::testing::Return;

namespace {

struct MockActivateQuellInitialGoal {
    MOCK_METHOD(void, activate_initial_goal, (subgoal_id));
};

struct MockMakeInitialGoalLineageForTracking {
    MOCK_METHOD((const goal_lineage*), make, (subgoal_id));
};

struct MockSetInitialGoalDepth {
    MOCK_METHOD(void, set, (const goal_lineage*, size_t));
};

} // namespace

using test_depth_tracking_quell_initial_goal_activator_t =
    depth_tracking_quell_initial_goal_activator<
        NiceMock<MockActivateQuellInitialGoal>,
        NiceMock<MockMakeInitialGoalLineageForTracking>,
        NiceMock<MockSetInitialGoalDepth>>;

struct DepthTrackingQuellInitialGoalActivatorTest : public ::testing::Test {
    NiceMock<MockActivateQuellInitialGoal> activate_quell_initial_goal;
    NiceMock<MockMakeInitialGoalLineageForTracking> make_initial_goal_lineage;
    NiceMock<MockSetInitialGoalDepth> set_goal_depth;

    goal_lineage gl{nullptr, 0};
    static constexpr subgoal_id kIdx = 0;

    test_depth_tracking_quell_initial_goal_activator_t activator{
        activate_quell_initial_goal, make_initial_goal_lineage, set_goal_depth};
};

TEST_F(DepthTrackingQuellInitialGoalActivatorTest, InitialGoalSitsAtDepthZero) {
    EXPECT_CALL(make_initial_goal_lineage, make(kIdx))
        .Times(AtLeast(1)).WillRepeatedly(Return(&gl));
    EXPECT_CALL(set_goal_depth, set(&gl, 0)).Times(1);
    activator.activate_initial_goal(kIdx);
}

TEST_F(DepthTrackingQuellInitialGoalActivatorTest, SetsDepthBeforeDelegating) {
    // The wrapped activator credits work looked up against this goal's depth,
    // so delegating first would price a goal that has no depth yet.
    EXPECT_CALL(make_initial_goal_lineage, make(kIdx))
        .Times(AtLeast(1)).WillRepeatedly(Return(&gl));
    Expectation set_depth = EXPECT_CALL(set_goal_depth, set(&gl, 0)).Times(1);
    EXPECT_CALL(activate_quell_initial_goal, activate_initial_goal(kIdx))
        .Times(1).After(set_depth);
    activator.activate_initial_goal(kIdx);
}

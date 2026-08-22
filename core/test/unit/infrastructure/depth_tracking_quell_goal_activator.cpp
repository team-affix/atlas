// depth_tracking_quell_goal_activator: records the child's depth as parent+1
// before handing the goal to the wrapped quell activator.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/depth_tracking_quell_goal_activator.hpp"
#include "value_objects/expr.hpp"

using ::testing::AtLeast;
using ::testing::Expectation;
using ::testing::NiceMock;
using ::testing::Return;

namespace {

struct MockActivateQuellGoal {
    MOCK_METHOD(void, activate, (const goal_lineage*));
};

struct MockGetGoalDepthForTracking {
    MOCK_METHOD(size_t, get, (const goal_lineage*), (const));
};

struct MockSetGoalDepthForTracking {
    MOCK_METHOD(void, set, (const goal_lineage*, size_t));
};

} // namespace

using test_depth_tracking_quell_goal_activator_t = depth_tracking_quell_goal_activator<
    NiceMock<MockActivateQuellGoal>, NiceMock<MockGetGoalDepthForTracking>,
    NiceMock<MockSetGoalDepthForTracking>>;

struct DepthTrackingQuellGoalActivatorTest : public ::testing::Test {
    NiceMock<MockActivateQuellGoal> activate_quell_goal;
    NiceMock<MockGetGoalDepthForTracking> get_goal_depth;
    NiceMock<MockSetGoalDepthForTracking> set_goal_depth;

    goal_lineage parent_gl{nullptr, 0};
    resolution_lineage rl{&parent_gl, 1};
    goal_lineage child_gl{&rl, 0};

    static constexpr size_t kParentDepth = 2;
    static constexpr size_t kChildDepth = 3;

    test_depth_tracking_quell_goal_activator_t activator{
        activate_quell_goal, get_goal_depth, set_goal_depth};
};

TEST_F(DepthTrackingQuellGoalActivatorTest, ChildDepthIsParentDepthPlusOne) {
    EXPECT_CALL(get_goal_depth, get(&parent_gl))
        .Times(AtLeast(1)).WillRepeatedly(Return(kParentDepth));
    EXPECT_CALL(set_goal_depth, set(&child_gl, kChildDepth)).Times(1);
    activator.activate(&child_gl);
}

TEST_F(DepthTrackingQuellGoalActivatorTest, SetsDepthBeforeDelegating) {
    // The wrapped activator credits work looked up against this goal's depth,
    // so delegating first would price a goal that has no depth yet.
    EXPECT_CALL(get_goal_depth, get(&parent_gl))
        .Times(AtLeast(1)).WillRepeatedly(Return(kParentDepth));
    Expectation set_depth = EXPECT_CALL(set_goal_depth, set(&child_gl, kChildDepth)).Times(1);
    EXPECT_CALL(activate_quell_goal, activate(&child_gl)).Times(1).After(set_depth);
    activator.activate(&child_gl);
}

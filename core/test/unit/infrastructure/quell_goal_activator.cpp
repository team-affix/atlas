// quell_goal_activator: delegates to goal_activator, then prices the goal's
// work, records it, and credits remaining_work.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/quell_goal_activator.hpp"
#include "value_objects/expr.hpp"

using ::testing::AtLeast;
using ::testing::NiceMock;
using ::testing::Return;

namespace {

struct MockGoalActivator {
    MOCK_METHOD(void, activate, (const goal_lineage*));
};

struct MockSetGoalWorkValue {
    MOCK_METHOD(void, set, (const goal_lineage*, double));
};

struct MockGetGoalWork {
    MOCK_METHOD(double, get, (const goal_lineage*), (const));
};

struct MockAddRemainingWork {
    MOCK_METHOD(void, add, (double));
};

} // namespace

using test_quell_goal_activator_t = quell_goal_activator<
    NiceMock<MockGoalActivator>, NiceMock<MockSetGoalWorkValue>,
    NiceMock<MockGetGoalWork>, NiceMock<MockAddRemainingWork>>;

struct QuellGoalActivatorTest : public ::testing::Test {
    NiceMock<MockGoalActivator> mock_goal_activator;
    NiceMock<MockSetGoalWorkValue> set_goal_work_value;
    NiceMock<MockGetGoalWork> get_goal_work;
    NiceMock<MockAddRemainingWork> add_remaining_work;

    goal_lineage parent_gl{nullptr, 0};
    resolution_lineage rl{&parent_gl, 1};
    goal_lineage child_gl{&rl, 0};

    static constexpr double kChildWork = 1.5;

    test_quell_goal_activator_t activator{
        mock_goal_activator, set_goal_work_value, get_goal_work, add_remaining_work};
};

TEST_F(QuellGoalActivatorTest, ActivatesThenSetsChildWorkAndCreditsRemaining) {
    EXPECT_CALL(get_goal_work, get(&child_gl))
        .Times(AtLeast(1)).WillRepeatedly(Return(kChildWork));
    testing::InSequence seq;
    EXPECT_CALL(mock_goal_activator, activate(&child_gl)).Times(1);
    EXPECT_CALL(set_goal_work_value, set(&child_gl, kChildWork)).Times(1);
    EXPECT_CALL(add_remaining_work, add(kChildWork)).Times(1);
    activator.activate(&child_gl);
}

// quell_goal_activator: delegates to goal_activator, then sets child depth = parent+1
// and work = f(depth).

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/quell_goal_activator.hpp"
#include "value_objects/expr.hpp"

using ::testing::AtLeast;
using ::testing::Return;

namespace {

struct MockGoalActivator {
    MOCK_METHOD(void, activate, (const goal_lineage*));
};

struct MockGetGoalDepth {
    MOCK_METHOD(size_t, get, (const goal_lineage*), (const));
};

struct MockSetGoalDepth {
    MOCK_METHOD(void, set, (const goal_lineage*, size_t));
};

struct MockSetGoalWorkValue {
    MOCK_METHOD(void, set, (const goal_lineage*, double));
};

struct MockGetGoalWork {
    MOCK_METHOD(double, get, (size_t), (const));
};

} // namespace

using test_quell_goal_activator_t = quell_goal_activator<
    MockGoalActivator, MockGetGoalDepth, MockSetGoalDepth,
    MockSetGoalWorkValue, MockGetGoalWork>;

struct QuellGoalActivatorTest : public ::testing::Test {
    MockGoalActivator mock_goal_activator;
    MockGetGoalDepth get_goal_depth;
    MockSetGoalDepth set_goal_depth;
    MockSetGoalWorkValue set_goal_work_value;
    MockGetGoalWork get_goal_work;

    goal_lineage parent_gl{nullptr, 0};
    resolution_lineage rl{&parent_gl, 1};
    goal_lineage child_gl{&rl, 0};

    static constexpr size_t kParentDepth = 2;
    static constexpr size_t kChildDepth = 3;
    static constexpr double kChildWork = 1.5;

    test_quell_goal_activator_t activator{
        mock_goal_activator, get_goal_depth, set_goal_depth,
        set_goal_work_value, get_goal_work};
};

TEST_F(QuellGoalActivatorTest, ActivatesThenSetsChildDepthAndWork) {
    EXPECT_CALL(get_goal_depth, get(&parent_gl))
        .Times(AtLeast(1)).WillRepeatedly(Return(kParentDepth));
    EXPECT_CALL(get_goal_work, get(kChildDepth))
        .Times(AtLeast(1)).WillRepeatedly(Return(kChildWork));
    testing::InSequence seq;
    EXPECT_CALL(mock_goal_activator, activate(&child_gl)).Times(1);
    EXPECT_CALL(set_goal_depth, set(&child_gl, kChildDepth)).Times(1);
    EXPECT_CALL(set_goal_work_value, set(&child_gl, kChildWork)).Times(1);
    activator.activate(&child_gl);
}

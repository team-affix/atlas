// quell_initial_goal_activator: delegates to initial_goal_activator, then sets depth 0,
// work f(0), and adds that work to remaining.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/quell_initial_goal_activator.hpp"
#include "value_objects/expr.hpp"

using ::testing::AtLeast;
using ::testing::Return;

struct MockInitialGoalActivator {
    MOCK_METHOD(void, activate_initial_goal, (subgoal_id));
};

struct MockMakeInitialGoalLineage {
    MOCK_METHOD((const goal_lineage*), make, (subgoal_id));
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

struct MockAddRemainingWork {
    MOCK_METHOD(void, add, (double));
};

using test_quell_initial_goal_activator_t = quell_initial_goal_activator<
    MockInitialGoalActivator, MockMakeInitialGoalLineage,
    MockSetGoalDepth, MockSetGoalWorkValue, MockGetGoalWork, MockAddRemainingWork>;

struct QuellInitialGoalActivatorTest : public ::testing::Test {
    MockInitialGoalActivator mock_initial;
    MockMakeInitialGoalLineage make_initial_goal_lineage;
    MockSetGoalDepth set_goal_depth;
    MockSetGoalWorkValue set_goal_work_value;
    MockGetGoalWork get_goal_work;
    MockAddRemainingWork add_remaining_work;

    goal_lineage gl{nullptr, 0};
    static constexpr subgoal_id kIdx = 0;
    static constexpr double kDecay0 = 8.38905609893065;

    test_quell_initial_goal_activator_t activator{
        mock_initial, make_initial_goal_lineage, set_goal_depth, set_goal_work_value,
        get_goal_work, add_remaining_work};
};

TEST_F(QuellInitialGoalActivatorTest, ActivatesThenSetsDepthWorkAndAddsRemaining) {
    EXPECT_CALL(get_goal_work, get(0)).Times(AtLeast(1)).WillRepeatedly(Return(kDecay0));
    testing::InSequence seq;
    EXPECT_CALL(mock_initial, activate_initial_goal(kIdx)).Times(1);
    EXPECT_CALL(make_initial_goal_lineage, make(kIdx)).WillOnce(Return(&gl));
    EXPECT_CALL(set_goal_depth, set(&gl, 0)).Times(1);
    EXPECT_CALL(set_goal_work_value, set(&gl, kDecay0)).Times(1);
    EXPECT_CALL(add_remaining_work, add(kDecay0)).Times(1);
    activator.activate_initial_goal(kIdx);
}

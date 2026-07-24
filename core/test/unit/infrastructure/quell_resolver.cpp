// quell_resolver: on success subtracts parent work (children already credited
// in quell_goal_activator); on resolve false leaves remaining_work untouched.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/quell_resolver.hpp"
#include "value_objects/lineage.hpp"

using ::testing::AtLeast;
using ::testing::Return;

struct MockResolver {
    MOCK_METHOD(bool, resolve, (const resolution_lineage*));
};

struct MockGetGoalWorkValue {
    MOCK_METHOD(double, get, (const goal_lineage*), (const));
};

struct MockSubtractRemainingWork {
    MOCK_METHOD(void, subtract, (double));
};

using test_quell_resolver_t = quell_resolver<
    MockResolver, MockGetGoalWorkValue, MockSubtractRemainingWork>;

struct QuellResolverTest : public ::testing::Test {
    MockResolver mock_resolver;
    MockGetGoalWorkValue get_goal_work_value;
    MockSubtractRemainingWork subtract_remaining_work;

    goal_lineage parent_gl{nullptr, 0};
    resolution_lineage rl{&parent_gl, 0};

    static constexpr double kParentWork = 4.0;

    test_quell_resolver_t resolver_sut{
        mock_resolver, get_goal_work_value, subtract_remaining_work};
};

TEST_F(QuellResolverTest, SuccessSubtractsParentWork) {
    EXPECT_CALL(get_goal_work_value, get(&parent_gl))
        .Times(AtLeast(1)).WillRepeatedly(Return(kParentWork));
    EXPECT_CALL(mock_resolver, resolve(&rl)).WillOnce(Return(true));
    EXPECT_CALL(subtract_remaining_work, subtract(kParentWork)).Times(1);
    EXPECT_TRUE(resolver_sut.resolve(&rl));
}

TEST_F(QuellResolverTest, ResolveFalseDoesNotSubtract) {
    EXPECT_CALL(get_goal_work_value, get(&parent_gl))
        .Times(AtLeast(1)).WillRepeatedly(Return(kParentWork));
    EXPECT_CALL(mock_resolver, resolve(&rl)).WillOnce(Return(false));
    EXPECT_CALL(subtract_remaining_work, subtract).Times(0);
    EXPECT_FALSE(resolver_sut.resolve(&rl));
}

// quell_resolver: on success subtracts parent work and adds |body|*f(parent_depth+1);
// on resolve false leaves remaining_work untouched.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/quell_resolver.hpp"
#include "value_objects/rule.hpp"
#include "value_objects/expr.hpp"

using ::testing::AtLeast;
using ::testing::Return;

struct MockResolver {
    MOCK_METHOD(bool, resolve, (const resolution_lineage*));
};

struct MockGetRule {
    MOCK_METHOD(const rule*, get_rule, (rule_id), (const));
};

struct MockGetGoalWorkValue {
    MOCK_METHOD(double, get, (const goal_lineage*), (const));
};

struct MockGetGoalDepth {
    MOCK_METHOD(size_t, get, (const goal_lineage*), (const));
};

struct MockGetGoalWork {
    MOCK_METHOD(double, get, (size_t), (const));
};

struct MockSubtractRemainingWork {
    MOCK_METHOD(void, subtract, (double));
};

struct MockAddRemainingWork {
    MOCK_METHOD(void, add, (double));
};

using test_quell_resolver_t = quell_resolver<
    MockResolver, MockGetRule, MockGetGoalWorkValue, MockGetGoalDepth,
    MockGetGoalWork, MockSubtractRemainingWork, MockAddRemainingWork>;

struct QuellResolverTest : public ::testing::Test {
    MockResolver mock_resolver;
    MockGetRule get_rule;
    MockGetGoalWorkValue get_goal_work_value;
    MockGetGoalDepth get_goal_depth;
    MockGetGoalWork get_goal_work;
    MockSubtractRemainingWork subtract_remaining_work;
    MockAddRemainingWork add_remaining_work;

    goal_lineage parent_gl{nullptr, 0};
    resolution_lineage rl{&parent_gl, 0};

    expr head{expr::var{0}};
    expr body0{expr::var{1}};
    expr body1{expr::var{2}};
    rule fact_rule{&head, {}};
    rule two_body_rule{&head, {&body0, &body1}, 3};

    static constexpr double kParentWork = 4.0;
    static constexpr size_t kParentDepth = 0;
    static constexpr double kChildWork = 2.0;

    test_quell_resolver_t resolver_sut{
        mock_resolver, get_rule, get_goal_work_value, get_goal_depth,
        get_goal_work, subtract_remaining_work, add_remaining_work};

    void expect_getter_stubs(const rule* r) {
        EXPECT_CALL(get_goal_work_value, get(&parent_gl))
            .Times(AtLeast(1)).WillRepeatedly(Return(kParentWork));
        EXPECT_CALL(get_goal_depth, get(&parent_gl))
            .Times(AtLeast(1)).WillRepeatedly(Return(kParentDepth));
        EXPECT_CALL(get_rule, get_rule(rl.idx))
            .Times(AtLeast(1)).WillRepeatedly(Return(r));
        EXPECT_CALL(get_goal_work, get(kParentDepth + 1))
            .Times(AtLeast(1)).WillRepeatedly(Return(kChildWork));
    }
};

TEST_F(QuellResolverTest, FactResolutionSubtractsParentAndAddsZero) {
    expect_getter_stubs(&fact_rule);
    EXPECT_CALL(mock_resolver, resolve(&rl)).WillOnce(Return(true));
    EXPECT_CALL(subtract_remaining_work, subtract(kParentWork)).Times(1);
    EXPECT_CALL(add_remaining_work, add(0.0)).Times(1);
    EXPECT_TRUE(resolver_sut.resolve(&rl));
}

TEST_F(QuellResolverTest, TwoBodyClauseSubtractsParentAndAddsTwiceChildWork) {
    expect_getter_stubs(&two_body_rule);
    EXPECT_CALL(mock_resolver, resolve(&rl)).WillOnce(Return(true));
    EXPECT_CALL(subtract_remaining_work, subtract(kParentWork)).Times(1);
    EXPECT_CALL(add_remaining_work, add(2.0 * kChildWork)).Times(1);
    EXPECT_TRUE(resolver_sut.resolve(&rl));
}

TEST_F(QuellResolverTest, ResolveFalseDoesNotMutateRemainingWork) {
    expect_getter_stubs(&fact_rule);
    EXPECT_CALL(mock_resolver, resolve(&rl)).WillOnce(Return(false));
    EXPECT_CALL(subtract_remaining_work, subtract).Times(0);
    EXPECT_CALL(add_remaining_work, add).Times(0);
    EXPECT_FALSE(resolver_sut.resolve(&rl));
}

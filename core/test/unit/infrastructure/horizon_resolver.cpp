// horizon_resolver: accumulates parent weight into CGW on fact resolution, then delegates.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/horizon_resolver.hpp"
#include "value_objects/rule.hpp"
#include "value_objects/expr.hpp"

using ::testing::Return;

struct MockResolver {
    MOCK_METHOD(bool, resolve, (const resolution_lineage*));
};

struct MockGetRule {
    MOCK_METHOD(const rule*, get, (rule_id), (const));
};

struct MockGoalWeights {
    MOCK_METHOD(double, get, (const goal_lineage*), (const));
};

struct MockCumulativeGroundedWeight {
    MOCK_METHOD(void, accumulate, (double));
};

using test_horizon_resolver_t = horizon_resolver<MockResolver, MockGetRule, MockGoalWeights, MockCumulativeGroundedWeight>;

struct HorizonResolverTest : public ::testing::Test {
    MockResolver mock_resolver;
    MockGetRule get_rule;
    MockGoalWeights goal_weights;
    MockCumulativeGroundedWeight cumulative_grounded_weight;

    goal_lineage parent_gl{nullptr, 0};
    resolution_lineage rl{&parent_gl, 0};

    expr head{expr::var{0}};
    rule fact_rule{&head, {}};
    rule clause_rule{&head, {&head}};

    static constexpr double kGoalWeight = 0.25;

    test_horizon_resolver_t resolver_sut{mock_resolver, get_rule, goal_weights, cumulative_grounded_weight};
};

TEST_F(HorizonResolverTest, FactResolutionAccumulatesWeightThenDelegates) {
    testing::InSequence seq;
    EXPECT_CALL(get_rule, get(rl.idx)).WillOnce(Return(&fact_rule));
    EXPECT_CALL(goal_weights, get(&parent_gl)).WillOnce(Return(kGoalWeight));
    EXPECT_CALL(cumulative_grounded_weight, accumulate(kGoalWeight)).Times(1);
    EXPECT_CALL(mock_resolver, resolve(&rl)).WillOnce(Return(true));
    EXPECT_TRUE(resolver_sut.resolve(&rl));
}

TEST_F(HorizonResolverTest, NonFactResolutionDelegatesWithoutAccumulating) {
    EXPECT_CALL(get_rule, get(rl.idx)).WillOnce(Return(&clause_rule));
    EXPECT_CALL(cumulative_grounded_weight, accumulate).Times(0);
    EXPECT_CALL(mock_resolver, resolve(&rl)).WillOnce(Return(true));
    EXPECT_TRUE(resolver_sut.resolve(&rl));
}

TEST_F(HorizonResolverTest, PropagatesInnerFalse) {
    EXPECT_CALL(get_rule, get(rl.idx)).WillOnce(Return(&clause_rule));
    EXPECT_CALL(mock_resolver, resolve(&rl)).WillOnce(Return(false));
    EXPECT_FALSE(resolver_sut.resolve(&rl));
}

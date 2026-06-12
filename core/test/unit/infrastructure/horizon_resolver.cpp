// horizon_resolver: accumulates parent weight into CGW on fact resolution, then delegates.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>
#include "locator_fixture.hpp"
#include "infrastructure/horizon_resolver.hpp"
#include "infrastructure/resolver.hpp"
#include "interfaces/i_get_rule.hpp"
#include "interfaces/i_get_goal_weight.hpp"
#include "interfaces/i_accumulate_grounded_weight.hpp"
#include "interfaces/i_goal_deactivator.hpp"
#include "interfaces/i_activate_subgoals_and_candidates.hpp"
#include "interfaces/i_deactivate_goal_candidates.hpp"
#include "value_objects/rule.hpp"
#include "value_objects/expr.hpp"

using ::testing::Return;

struct MockGetRule : public i_get_rule {
    MOCK_METHOD(const rule*, get, (rule_id), (const, override));
};

struct MockGetGoalWeight : public i_get_goal_weight {
    MOCK_METHOD(double, get, (const goal_lineage*), (const, override));
};

struct MockAccumulateGroundedWeight : public i_accumulate_grounded_weight {
    MOCK_METHOD(void, accumulate, (double), (override));
};

struct MockGoalDeactivator : public i_goal_deactivator {
    MOCK_METHOD(void, deactivate, (const goal_lineage*), (override));
};

struct MockActivateSubgoalsAndCandidates : public i_activate_subgoals_and_candidates {
    MOCK_METHOD(bool, activate_subgoals_and_candidates, (const resolution_lineage*), (override));
};

struct MockDeactivateGoalCandidates : public i_deactivate_goal_candidates {
    MOCK_METHOD(void, deactivate_goal_candidates, (const goal_lineage*), (override));
};

struct MockDelegateResolver : public resolver {
    explicit MockDelegateResolver(locator& loc) : resolver(loc) {}
    MOCK_METHOD(bool, resolve, (const resolution_lineage*), (override));
};

struct HorizonResolverTest : public ::testing::Test {
    locator loc;
    MockGetRule get_rule;
    MockGetGoalWeight get_goal_weight;
    MockAccumulateGroundedWeight accumulate_grounded_weight;
    MockGoalDeactivator goal_deactivator;
    MockActivateSubgoalsAndCandidates activate_subgoals;
    MockDeactivateGoalCandidates deactivate_candidates;
    std::unique_ptr<MockDelegateResolver> mock_delegate_resolver;
    std::unique_ptr<horizon_resolver> resolver_sut;

    goal_lineage parent_gl{nullptr, 0};
    resolution_lineage rl{&parent_gl, 0};

    expr head{expr::var{0}};
    rule fact_rule{&head, {}};
    rule clause_rule{&head, {&head}};

    static constexpr double kGoalWeight = 0.25;

    void SetUp() override {
        loc.bind_as<i_get_rule>(get_rule);
        loc.bind_as<i_get_goal_weight>(get_goal_weight);
        loc.bind_as<i_accumulate_grounded_weight>(accumulate_grounded_weight);
        loc.bind_as<i_goal_deactivator>(goal_deactivator);
        loc.bind_as<i_activate_subgoals_and_candidates>(activate_subgoals);
        loc.bind_as<i_deactivate_goal_candidates>(deactivate_candidates);
        mock_delegate_resolver = std::make_unique<MockDelegateResolver>(loc);
        loc.bind_as<resolver>(*mock_delegate_resolver);
        resolver_sut = std::make_unique<horizon_resolver>(loc);
    }
};

TEST_F(HorizonResolverTest, FactResolutionAccumulatesWeightThenDelegates) {
    testing::InSequence seq;
    EXPECT_CALL(get_rule, get(rl.idx)).WillOnce(Return(&fact_rule));
    EXPECT_CALL(get_goal_weight, get(&parent_gl)).WillOnce(Return(kGoalWeight));
    EXPECT_CALL(accumulate_grounded_weight, accumulate(kGoalWeight)).Times(1);
    EXPECT_CALL(*mock_delegate_resolver, resolve(&rl)).WillOnce(Return(true));
    EXPECT_TRUE(resolver_sut->resolve(&rl));
}

TEST_F(HorizonResolverTest, NonFactResolutionDelegatesWithoutAccumulating) {
    EXPECT_CALL(get_rule, get(rl.idx)).WillOnce(Return(&clause_rule));
    EXPECT_CALL(accumulate_grounded_weight, accumulate).Times(0);
    EXPECT_CALL(*mock_delegate_resolver, resolve(&rl)).WillOnce(Return(true));
    EXPECT_TRUE(resolver_sut->resolve(&rl));
}

TEST_F(HorizonResolverTest, PropagatesInnerFalse) {
    EXPECT_CALL(get_rule, get(rl.idx)).WillOnce(Return(&clause_rule));
    EXPECT_CALL(*mock_delegate_resolver, resolve(&rl)).WillOnce(Return(false));
    EXPECT_FALSE(resolver_sut->resolve(&rl));
}

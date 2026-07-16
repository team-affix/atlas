// querier: goal-sensitive DB rule lookup by outermost goal functor.

#include <gtest/gtest.h>
#include <gmock/gmock.h>

using ::testing::Return;
using ::testing::ReturnRef;
#include "infrastructure/querier.hpp"
#include "value_objects/lineage.hpp"
#include "functor_fixture.hpp"

struct MockGetGoalExpr {
    MOCK_METHOD(framed_expr, get, (const goal_lineage*), (const));
};

struct MockLookupAllRules {
    MOCK_METHOD(rule_id_set&, lookup_all_rules, ());
};

struct MockLookupRuleByOutermostFunctor {
    MOCK_METHOD(const rule_id_set&, lookup_rule_by_outermost_functor, (uint32_t), (const));
};

using test_querier_t = querier<MockGetGoalExpr, MockLookupAllRules, MockLookupRuleByOutermostFunctor>;

struct QuerierTest : public ::testing::Test {
    test_functors functors;
    goal_lineage gl{nullptr, 0};
    expr f_goal{expr::functor{functors.id("f"), {}}};
    expr var_goal{expr::var{0}};
    rule_id_set all_rules;
    rule_id_set f_rules;
    MockGetGoalExpr get_goal_expr;
    MockLookupAllRules lookup_all_rules;
    MockLookupRuleByOutermostFunctor lookup_rule_by_outermost_functor;
    test_querier_t sut{get_goal_expr, lookup_all_rules, lookup_rule_by_outermost_functor};
};

TEST_F(QuerierTest, FunctorGoalUsesFunctorLookup) {
    EXPECT_CALL(get_goal_expr, get(&gl)).WillOnce(Return(framed_expr{&f_goal, 0}));
    EXPECT_CALL(lookup_rule_by_outermost_functor, lookup_rule_by_outermost_functor(functors.id("f")))
        .WillOnce(ReturnRef(f_rules));
    EXPECT_CALL(lookup_all_rules, lookup_all_rules()).Times(0);

    EXPECT_EQ(&sut.get_candidate_rules(&gl), &f_rules);
}

TEST_F(QuerierTest, VarGoalUsesLookupAllRules) {
    EXPECT_CALL(get_goal_expr, get(&gl)).WillOnce(Return(framed_expr{&var_goal, 0}));
    EXPECT_CALL(lookup_all_rules, lookup_all_rules()).WillOnce(ReturnRef(all_rules));
    EXPECT_CALL(lookup_rule_by_outermost_functor, lookup_rule_by_outermost_functor).Times(0);

    EXPECT_EQ(&sut.get_candidate_rules(&gl), &all_rules);
}

// expr_querier: candidate rule lookup dispatch on framed_expr content.
// A functor skeleton uses functor-indexed lookup; a var skeleton uses all-rules lookup.

#include <gtest/gtest.h>
#include <gmock/gmock.h>

using ::testing::Return;
using ::testing::ReturnRef;
#include "infrastructure/expr_querier.hpp"
#include "functor_fixture.hpp"

struct MockLookupAllRules {
    MOCK_METHOD(rule_id_set&, lookup_all_rules, ());
};

struct MockLookupRuleByOutermostFunctor {
    MOCK_METHOD(const rule_id_set&, lookup_rule_by_outermost_functor, (uint32_t), (const));
};

using test_expr_querier_t = expr_querier<MockLookupAllRules, MockLookupRuleByOutermostFunctor>;

struct ExprQuerierTest : public ::testing::Test {
    test_functors functors;
    expr f_goal{expr::functor{functors.id("f"), {}}};
    expr var_goal{expr::var{0}};
    rule_id_set all_rules;
    rule_id_set f_rules;
    MockLookupAllRules lookup_all_rules;
    MockLookupRuleByOutermostFunctor lookup_rule_by_outermost_functor;
    test_expr_querier_t sut{lookup_all_rules, lookup_rule_by_outermost_functor};
};

TEST_F(ExprQuerierTest, FunctorSkeletonUsesFunctorLookup) {
    EXPECT_CALL(lookup_rule_by_outermost_functor,
                lookup_rule_by_outermost_functor(functors.id("f")))
        .WillOnce(ReturnRef(f_rules));
    EXPECT_CALL(lookup_all_rules, lookup_all_rules()).Times(0);

    EXPECT_EQ(&sut.get_candidate_rules({&f_goal, 0}), &f_rules);
}

TEST_F(ExprQuerierTest, VarSkeletonUsesLookupAllRules) {
    EXPECT_CALL(lookup_all_rules, lookup_all_rules()).WillOnce(ReturnRef(all_rules));
    EXPECT_CALL(lookup_rule_by_outermost_functor, lookup_rule_by_outermost_functor).Times(0);

    EXPECT_EQ(&sut.get_candidate_rules({&var_goal, 0}), &all_rules);
}

TEST_F(ExprQuerierTest, FrameOffsetDoesNotAffectDispatch) {
    EXPECT_CALL(lookup_rule_by_outermost_functor,
                lookup_rule_by_outermost_functor(functors.id("f")))
        .WillOnce(ReturnRef(f_rules));

    EXPECT_EQ(&sut.get_candidate_rules({&f_goal, 42}), &f_rules);
}

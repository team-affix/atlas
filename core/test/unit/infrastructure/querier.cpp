// querier: thin adapter that resolves a goal_lineage* to a framed_expr via
// IGetGoalExpr, then delegates candidate lookup to IQuery.

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

struct MockQuery {
    MOCK_METHOD(const rule_id_set&, get_candidate_rules, (framed_expr));
};

using test_querier_t = querier<MockGetGoalExpr, MockQuery>;

struct QuerierTest : public ::testing::Test {
    test_functors functors;
    goal_lineage gl{nullptr, 0};
    expr f_goal{expr::functor{functors.id("f"), {}}};
    rule_id_set f_rules;
    MockGetGoalExpr get_goal_expr;
    MockQuery query;
    test_querier_t sut{get_goal_expr, query};
};

TEST_F(QuerierTest, DelegatesToQueryWithResolvedFramedExpr) {
    const framed_expr fe{&f_goal, 7};
    EXPECT_CALL(get_goal_expr, get(&gl)).WillOnce(Return(fe));
    EXPECT_CALL(query, get_candidate_rules(fe)).WillOnce(ReturnRef(f_rules));

    EXPECT_EQ(&sut.get_candidate_rules(&gl), &f_rules);
}

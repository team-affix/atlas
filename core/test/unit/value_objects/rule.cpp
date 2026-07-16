// rule: stores clause head, body pointers, and var_count.

#include <gtest/gtest.h>
#include <vector>
#include "value_objects/expr.hpp"
#include "value_objects/rule.hpp"

struct RuleTest : public ::testing::Test {
    expr head{expr::functor{1, {}}};
    expr body0{expr::var{0}};
    expr body1{expr::var{1}};
    expr other_head{expr::functor{2, {}}};
};

TEST_F(RuleTest, TwoArgConstructorSetsVarCountToZero) {
    const rule r{&head, {&body0, &body1}};

    EXPECT_EQ(r.var_count, 0u);
    EXPECT_EQ(r.head, &head);
    EXPECT_EQ(r.body, (std::vector<const expr*>{&body0, &body1}));
}

TEST_F(RuleTest, ThreeArgConstructorPreservesVarCount) {
    const rule r{&head, {&body0}, 4};

    EXPECT_EQ(r.var_count, 4u);
    EXPECT_EQ(r.head, &head);
    EXPECT_EQ(r.body, (std::vector<const expr*>{&body0}));
}

TEST_F(RuleTest, OrderingDistinguishesHeadPointer) {
    const rule left{&head, {&body0}, 1};
    const rule right{&other_head, {&body0}, 1};

    EXPECT_LT(left, right);
    EXPECT_GT(right, left);
}

TEST_F(RuleTest, OrderingDistinguishesBody) {
    const rule left{&head, {&body0}, 1};
    const rule right{&head, {&body1}, 1};

    EXPECT_LT(left, right);
    EXPECT_GT(right, left);
}

TEST_F(RuleTest, OrderingDistinguishesVarCount) {
    const rule left{&head, {&body0}, 1};
    const rule right{&head, {&body0}, 2};

    EXPECT_LT(left, right);
    EXPECT_GT(right, left);
}

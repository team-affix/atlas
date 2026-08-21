// expr: the defaulted operator<=>, which expr_pool's interning depends on.
//
// Only expr's HASH is tested today (value_objects/expr_hash.cpp). That is half a
// contract: expr_pool interns by looking an expr up in an ordered/hashed
// container, so a wrong ORDERING silently interns two equal exprs as distinct
// entries -- and then unification compares skeleton pointers, decides two
// identical goals differ, and the solver explores the same branch twice while
// reporting different lineages for it.
//
// Note args is a vector of POINTERS, so ordering is by arg identity, not by
// structure. That is precisely why expr_pool must intern bottom-up: only once
// children are interned does pointer equality coincide with structural equality.

#include <gtest/gtest.h>
#include "value_objects/expr.hpp"

struct ExprTest : public ::testing::Test {
    static constexpr uint32_t kFunctorLow = 1;
    static constexpr uint32_t kFunctorHigh = 2;

    expr leaf_a{expr::var{0}};
    expr leaf_b{expr::var{1}};
};

TEST_F(ExprTest, OrderingComparesFunctorIdThenArgs) {
    const expr same_args_0{expr::functor{kFunctorLow, {&leaf_a}}};
    const expr same_args_1{expr::functor{kFunctorLow, {&leaf_a}}};
    // Equal id and equal arg identities must compare equal, or the pool interns a
    // duplicate for an expr it already holds.
    EXPECT_EQ(same_args_0, same_args_1);
    EXPECT_FALSE(same_args_0 < same_args_1);
    EXPECT_FALSE(same_args_1 < same_args_0);

    // The id dominates the args: a low id sorts first whatever its arguments are.
    const expr low_id{expr::functor{kFunctorLow, {&leaf_b}}};
    const expr high_id{expr::functor{kFunctorHigh, {&leaf_a}}};
    EXPECT_LT(low_id, high_id);
    EXPECT_GT(high_id, low_id);

    // Same id, differing arg identity: distinct, and strictly ordered one way.
    const expr other_args{expr::functor{kFunctorLow, {&leaf_b}}};
    EXPECT_NE(same_args_0, other_args);
    EXPECT_NE(same_args_0 < other_args, other_args < same_args_0);
}

TEST_F(ExprTest, ArityBreaksTiesOnEqualArgumentPrefix) {
    // std::vector compares lexicographically, so a prefix sorts before the longer
    // sequence that extends it. A functor and the same functor with an extra
    // argument are different terms and must never intern to the same slot.
    const expr unary{expr::functor{kFunctorLow, {&leaf_a}}};
    const expr binary{expr::functor{kFunctorLow, {&leaf_a, &leaf_b}}};
    const expr nullary{expr::functor{kFunctorLow, {}}};

    EXPECT_LT(nullary, unary);
    EXPECT_LT(unary, binary);
    EXPECT_NE(nullary, binary);
}

TEST_F(ExprTest, VarAndFunctorAlternativesOrderByVariantIndex) {
    // content is variant<functor, var>, so every functor sorts before every var no
    // matter how the ids and indices compare. Interning relies on this partition:
    // a variable can never collide with a functor term, however the numbers line up.
    const expr high_functor{expr::functor{0xFFFFFFFFu, {}}};
    const expr low_var{expr::var{0}};

    EXPECT_LT(high_functor, low_var);
    EXPECT_NE(high_functor, low_var);

    // Within the var alternative, ordering is by index.
    EXPECT_LT(leaf_a, leaf_b);
    EXPECT_EQ(leaf_a, expr{expr::var{0}});
}

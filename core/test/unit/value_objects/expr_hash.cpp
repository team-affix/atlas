// expr_hash: hashes expr variants for unordered containers. Unit tests assert
// equality contracts, alternative tagging, and argument-order sensitivity.

#include <gtest/gtest.h>
#include "infrastructure/expr_pool.hpp"
#include "value_objects/expr_hash.hpp"
#include "functor_fixture.hpp"

struct ExprHashTest : public ::testing::Test {
    test_functors functors;
    expr_pool pool;
    expr_hash hasher;
};

TEST_F(ExprHashTest, SameVarHashesEqual) {
    const expr* v0 = pool.make_var(0);
    EXPECT_EQ(hasher(*v0), hasher(*v0));
    EXPECT_EQ(hasher(*pool.make_var(0)), hasher(*v0));
}

TEST_F(ExprHashTest, DifferentVarHashesDiffer) {
    EXPECT_NE(hasher(*pool.make_var(0)), hasher(*pool.make_var(1)));
}

TEST_F(ExprHashTest, SameNullaryFunctorHashesEqual) {
    const expr* f = pool.make_functor(functors.id("f"), {});
    EXPECT_EQ(hasher(*f), hasher(*pool.make_functor(functors.id("f"), {})));
}

TEST_F(ExprHashTest, FunctorArgsAffectHash) {
    const expr* v0 = pool.make_var(0);
    const expr* v1 = pool.make_var(1);
    const expr* nullary = pool.make_functor(functors.id("f"), {});
    const expr* unary = pool.make_functor(functors.id("f"), {v0});
    const expr* binary = pool.make_functor(functors.id("f"), {v0, v1});

    EXPECT_NE(hasher(*nullary), hasher(*unary));
    EXPECT_NE(hasher(*unary), hasher(*binary));
    EXPECT_NE(hasher(*nullary), hasher(*binary));
}

TEST_F(ExprHashTest, VarAndFunctorAlternativesTaggedDistinctly) {
    const uint32_t shared_id = functors.id("f");
    const expr var_expr{expr::var{shared_id}};
    const expr functor_expr{expr::functor{shared_id, {}}};

    EXPECT_NE(hasher(var_expr), hasher(functor_expr));
}

TEST_F(ExprHashTest, HashCombineOrderChangesSeed) {
    const expr* v0 = pool.make_var(0);
    const expr* v1 = pool.make_var(1);
    const expr* ab = pool.make_functor(functors.id("f"), {v0, v1});
    const expr* ba = pool.make_functor(functors.id("f"), {v1, v0});

    EXPECT_NE(hasher(*ab), hasher(*ba));
}

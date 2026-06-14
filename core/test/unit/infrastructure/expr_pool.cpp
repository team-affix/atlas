// expr_pool canonicalizes expr nodes in a persistent set. Unit tests assert
// pointer identity and import behavior.

#include <gtest/gtest.h>
#include "infrastructure/expr_pool.hpp"
#include "functor_fixture.hpp"

struct ExprPoolTest : public ::testing::Test {
    test_functors functors;
    expr_pool pool;
};

// ---------------------------------------------------------------------------
// Interning / identity
// ---------------------------------------------------------------------------

TEST_F(ExprPoolTest, VarInternedTwiceReturnsSamePointer) {
    EXPECT_EQ(pool.make_var(0), pool.make_var(0));
}

TEST_F(ExprPoolTest, DifferentVarsReturnDifferentPointers) {
    EXPECT_NE(pool.make_var(0), pool.make_var(1));
}

TEST_F(ExprPoolTest, NullaryFunctorInternedTwiceReturnsSamePointer) {
    EXPECT_EQ(pool.make_functor(functors.id("f"), {}), pool.make_functor(functors.id("f"), {}));
}

TEST_F(ExprPoolTest, DifferentFunctorsReturnDifferentPointers) {
    EXPECT_NE(pool.make_functor(functors.id("f"), {}), pool.make_functor(functors.id("g"), {}));
}

TEST_F(ExprPoolTest, FunctorWithVarArgInternedTwiceReturnsSamePointer) {
    const expr* v = pool.make_var(0);
    EXPECT_EQ(pool.make_functor(functors.id("f"), {v}), pool.make_functor(functors.id("f"), {v}));
}

TEST_F(ExprPoolTest, FunctorsSameNameDifferentArityReturnDifferentPointers) {
    const expr* v = pool.make_var(0);
    EXPECT_NE(pool.make_functor(functors.id("f"), {}), pool.make_functor(functors.id("f"), {v}));
}

TEST_F(ExprPoolTest, BinaryFunctorInternedTwiceReturnsSamePointer) {
    const expr* v0 = pool.make_var(0);
    const expr* v1 = pool.make_var(1);
    EXPECT_EQ(pool.make_functor(functors.id("f"), {v0, v1}), pool.make_functor(functors.id("f"), {v0, v1}));
}

TEST_F(ExprPoolTest, FunctorsWithDifferentArgsReturnDifferentPointers) {
    const expr* v0 = pool.make_var(0);
    const expr* v1 = pool.make_var(1);
    EXPECT_NE(pool.make_functor(functors.id("f"), {v0, v1}), pool.make_functor(functors.id("f"), {v1, v1}));
}

// ---------------------------------------------------------------------------
// import
// ---------------------------------------------------------------------------

TEST_F(ExprPoolTest, ImportVarInternsIntoPool) {
    const expr* pooled = pool.make_var(0);

    expr e{expr::var{0}};
    EXPECT_EQ(pool.import(&e), pooled);
    EXPECT_EQ(pool.import(&e), pooled);
}

TEST_F(ExprPoolTest, TernaryFunctorInternedTwiceReturnsSamePointer) {
    const expr* v0 = pool.make_var(0);
    const expr* v1 = pool.make_var(1);
    const expr* v2 = pool.make_var(2);
    EXPECT_EQ(pool.make_functor(functors.id("h"), {v0, v1, v2}), pool.make_functor(functors.id("h"), {v0, v1, v2}));
}

TEST_F(ExprPoolTest, ImportFunctorInternsArgsRecursively) {
    expr arg{expr::var{1}};
    expr root{expr::functor{functors.id("f"), {&arg}}};

    const expr* p = pool.import(&root);

    const expr::functor& f = std::get<expr::functor>(p->content);
    EXPECT_NE(f.args[0], &arg);
    EXPECT_EQ(f.args[0], pool.make_var(1));
}

TEST_F(ExprPoolTest, ImportTernaryFunctorInternsEachArg) {
    expr a{expr::var{0}};
    expr b{expr::var{1}};
    expr c{expr::var{2}};
    expr root{expr::functor{functors.id("h"), {&a, &b, &c}}};

    const expr* p = pool.import(&root);

    const expr::functor& f = std::get<expr::functor>(p->content);
    ASSERT_EQ(f.args.size(), 3u);
    EXPECT_EQ(f.args[0], pool.make_var(0));
    EXPECT_EQ(f.args[1], pool.make_var(1));
    EXPECT_EQ(f.args[2], pool.make_var(2));
}

TEST_F(ExprPoolTest, ImportNullaryAtomDedupesWithMakeFunctor) {
    expr e{expr::functor{functors.id("abc"), {}}};
    const expr* pooled = pool.make_functor(functors.id("abc"), {});
    EXPECT_EQ(pool.import(&e), pooled);
}

TEST_F(ExprPoolTest, ImportFunctorWithAtomArgs) {
    expr abc{expr::functor{functors.id("abc"), {}}};
    expr num{expr::functor{functors.id("123"), {}}};
    expr root{expr::functor{functors.id("f"), {&abc, &num}}};

    const expr* p = pool.import(&root);
    const expr::functor& f = std::get<expr::functor>(p->content);
    ASSERT_EQ(f.args.size(), 2u);
    EXPECT_EQ(f.args[0], pool.make_functor(functors.id("abc"), {}));
    EXPECT_EQ(f.args[1], pool.make_functor(functors.id("123"), {}));
}

TEST_F(ExprPoolTest, ImportRoundTripIdentity) {
    const expr* pooled = pool.make_functor(functors.id("abc"), {});
    EXPECT_EQ(pool.import(pooled), pooled);
}

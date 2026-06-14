// expr_pool canonicalizes expr nodes in a persistent set. Unit tests assert
// pointer identity and import behavior.

#include <gtest/gtest.h>
#include "infrastructure/expr_pool.hpp"

struct ExprPoolTest : public ::testing::Test {
    expr_pool pool;
};

// ---------------------------------------------------------------------------
// Interning / identity
// ---------------------------------------------------------------------------

TEST_F(ExprPoolTest, VarInternedTwiceReturnsSamePointer) {
    EXPECT_EQ(pool.make(0), pool.make(0));
}

TEST_F(ExprPoolTest, DifferentVarsReturnDifferentPointers) {
    EXPECT_NE(pool.make(0), pool.make(1));
}

TEST_F(ExprPoolTest, NullaryFunctorInternedTwiceReturnsSamePointer) {
    EXPECT_EQ(pool.make("f", {}), pool.make("f", {}));
}

TEST_F(ExprPoolTest, DifferentFunctorsReturnDifferentPointers) {
    EXPECT_NE(pool.make("f", {}), pool.make("g", {}));
}

TEST_F(ExprPoolTest, FunctorWithVarArgInternedTwiceReturnsSamePointer) {
    const expr* v = pool.make(0);
    EXPECT_EQ(pool.make("f", {v}), pool.make("f", {v}));
}

TEST_F(ExprPoolTest, FunctorsSameNameDifferentArityReturnDifferentPointers) {
    const expr* v = pool.make(0);
    EXPECT_NE(pool.make("f", {}), pool.make("f", {v}));
}

TEST_F(ExprPoolTest, BinaryFunctorInternedTwiceReturnsSamePointer) {
    const expr* v0 = pool.make(0);
    const expr* v1 = pool.make(1);
    EXPECT_EQ(pool.make("f", {v0, v1}), pool.make("f", {v0, v1}));
}

TEST_F(ExprPoolTest, FunctorsWithDifferentArgsReturnDifferentPointers) {
    const expr* v0 = pool.make(0);
    const expr* v1 = pool.make(1);
    EXPECT_NE(pool.make("f", {v0, v1}), pool.make("f", {v1, v1}));
}

// ---------------------------------------------------------------------------
// import
// ---------------------------------------------------------------------------

TEST_F(ExprPoolTest, ImportVarInternsIntoPool) {
    const expr* pooled = pool.make(0);

    expr e{expr::var{0}};
    EXPECT_EQ(pool.import(&e), pooled);
    EXPECT_EQ(pool.import(&e), pooled);
}

TEST_F(ExprPoolTest, TernaryFunctorInternedTwiceReturnsSamePointer) {
    const expr* v0 = pool.make(0);
    const expr* v1 = pool.make(1);
    const expr* v2 = pool.make(2);
    EXPECT_EQ(pool.make("h", {v0, v1, v2}), pool.make("h", {v0, v1, v2}));
}

TEST_F(ExprPoolTest, ImportFunctorInternsArgsRecursively) {
    expr arg{expr::var{1}};
    expr root{expr::functor{"f", {&arg}}};

    const expr* p = pool.import(&root);

    const expr::functor& f = std::get<expr::functor>(p->content);
    EXPECT_NE(f.args[0], &arg);
    EXPECT_EQ(f.args[0], pool.make(1));
}

TEST_F(ExprPoolTest, ImportTernaryFunctorInternsEachArg) {
    expr a{expr::var{0}};
    expr b{expr::var{1}};
    expr c{expr::var{2}};
    expr root{expr::functor{"h", {&a, &b, &c}}};

    const expr* p = pool.import(&root);

    const expr::functor& f = std::get<expr::functor>(p->content);
    ASSERT_EQ(f.args.size(), 3u);
    EXPECT_EQ(f.args[0], pool.make(0));
    EXPECT_EQ(f.args[1], pool.make(1));
    EXPECT_EQ(f.args[2], pool.make(2));
}

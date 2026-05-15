#include <gtest/gtest.h>
#include <optional>
#include "../../../core/hpp/infrastructure/expr_pool.hpp"
#include "../../../core/hpp/bootstrap/bindings.hpp"
#include "../../../core/hpp/bootstrap/locator.hpp"
#include "../../../core/hpp/utility/trail.hpp"

class ExprPoolTest : public ::testing::Test {
protected:
    void SetUp() override {
        b.bind<i_trail>(t);
        locator::register_bindings(&b);
        pool.emplace();
    }
    void TearDown() override { locator::register_bindings(nullptr); }
    trail t;
    bindings b;
    std::optional<expr_pool> pool;
};

// ---------------------------------------------------------------------------
// Interning / identity
// ---------------------------------------------------------------------------

TEST_F(ExprPoolTest, VarInternedTwiceReturnsSamePointer) {
    EXPECT_EQ(pool->var(0), pool->var(0));
}

TEST_F(ExprPoolTest, DifferentVarsReturnDifferentPointers) {
    EXPECT_NE(pool->var(0), pool->var(1));
}

TEST_F(ExprPoolTest, NullaryFunctorInternedTwiceReturnsSamePointer) {
    EXPECT_EQ(pool->functor("f", {}), pool->functor("f", {}));
}

TEST_F(ExprPoolTest, DifferentFunctorsReturnDifferentPointers) {
    EXPECT_NE(pool->functor("f", {}), pool->functor("g", {}));
}

TEST_F(ExprPoolTest, FunctorWithVarArgInternedTwiceReturnsSamePointer) {
    const expr* v = pool->var(0);
    EXPECT_EQ(pool->functor("f", {v}), pool->functor("f", {v}));
}

TEST_F(ExprPoolTest, FunctorsSameNameDifferentArityReturnDifferentPointers) {
    const expr* v = pool->var(0);
    EXPECT_NE(pool->functor("f", {}), pool->functor("f", {v}));
}

TEST_F(ExprPoolTest, BinaryFunctorInternedTwiceReturnsSamePointer) {
    const expr* v0 = pool->var(0);
    const expr* v1 = pool->var(1);
    EXPECT_EQ(pool->functor("f", {v0, v1}), pool->functor("f", {v0, v1}));
}

TEST_F(ExprPoolTest, BinaryFunctorsDifferingInFirstArgReturnDifferentPointers) {
    const expr* v0 = pool->var(0);
    const expr* v1 = pool->var(1);
    EXPECT_NE(pool->functor("f", {v0, v1}), pool->functor("f", {v1, v1}));
}

TEST_F(ExprPoolTest, BinaryFunctorsDifferingInSecondArgReturnDifferentPointers) {
    const expr* v0 = pool->var(0);
    const expr* v1 = pool->var(1);
    EXPECT_NE(pool->functor("f", {v0, v0}), pool->functor("f", {v0, v1}));
}

TEST_F(ExprPoolTest, TernaryFunctorInternedTwiceReturnsSamePointer) {
    const expr* v0 = pool->var(0);
    const expr* v1 = pool->var(1);
    const expr* v2 = pool->var(2);
    EXPECT_EQ(pool->functor("f", {v0, v1, v2}), pool->functor("f", {v0, v1, v2}));
}

TEST_F(ExprPoolTest, TernaryFunctorsDifferingInFirstArgReturnDifferentPointers) {
    const expr* v0 = pool->var(0);
    const expr* v1 = pool->var(1);
    const expr* v2 = pool->var(2);
    EXPECT_NE(pool->functor("f", {v0, v1, v2}), pool->functor("f", {v2, v1, v2}));
}

TEST_F(ExprPoolTest, TernaryFunctorsDifferingInSecondArgReturnDifferentPointers) {
    const expr* v0 = pool->var(0);
    const expr* v1 = pool->var(1);
    const expr* v2 = pool->var(2);
    EXPECT_NE(pool->functor("f", {v0, v1, v2}), pool->functor("f", {v0, v2, v2}));
}

TEST_F(ExprPoolTest, TernaryFunctorsDifferingInThirdArgReturnDifferentPointers) {
    const expr* v0 = pool->var(0);
    const expr* v1 = pool->var(1);
    const expr* v2 = pool->var(2);
    EXPECT_NE(pool->functor("f", {v0, v1, v2}), pool->functor("f", {v0, v1, v0}));
}

// ---------------------------------------------------------------------------
// Backtracking
// ---------------------------------------------------------------------------

TEST_F(ExprPoolTest, VarInternedBeforePushSurvivesPop) {
    const expr* p = pool->var(0);
    t.push();
    t.pop();
    EXPECT_EQ(pool->var(0), p);
}

TEST_F(ExprPoolTest, PoolIsStableAfterPop) {
    t.push();
    pool->var(0);
    t.pop();
    const expr* p_a = pool->var(0);
    EXPECT_EQ(pool->var(0), p_a);  // idempotent after re-intern
}

TEST_F(ExprPoolTest, ExprInternedBeforeFrameSurvivesPop) {
    const expr* p0 = pool->var(0);
    t.push();
    pool->var(1);
    t.pop();
    EXPECT_EQ(pool->var(0), p0);
}

TEST_F(ExprPoolTest, TwoNestedFramesOuterExprSurvivesInnerPop) {
    t.push();
    const expr* p0 = pool->var(0);
    t.push();
    pool->var(1);
    t.pop();
    EXPECT_EQ(pool->var(0), p0);
}

// ---------------------------------------------------------------------------
// import
// ---------------------------------------------------------------------------

TEST_F(ExprPoolTest, ImportVarInternsIntoPool) {
    expr e{expr::var{0}};
    const expr* p = pool->import(&e);
    EXPECT_NE(p, nullptr);
    EXPECT_EQ(pool->import(&e), p);
}

TEST_F(ExprPoolTest, ImportFunctorInternsArgsRecursively) {
    expr arg{expr::var{1}};
    expr root{expr::functor{"f", {&arg}}};  // raw, not interned
    const expr* p = pool->import(&root);
    EXPECT_NE(p, nullptr);
    const expr::functor& f = std::get<expr::functor>(p->content);
    EXPECT_NE(f.args[0], &arg);         // arg was re-interned into the pool
    EXPECT_EQ(f.args[0], pool->var(1)); // the re-interned arg equals pool's canonical var(1)
}

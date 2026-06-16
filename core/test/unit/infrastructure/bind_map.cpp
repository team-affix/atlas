// bind_map stores variable renamings and implements WHNF with path compression over
// var chains. Unit tests exercise bind/whnf directly (no collaborators to mock).

#include <gtest/gtest.h>
#include "infrastructure/bind_map.hpp"
#include "infrastructure/globalizer.hpp"
#include "functor_fixture.hpp"

// Helper: whnf with global frame (offset 0), return skeleton.
static const expr* whnf0(bind_map& bm, const expr* e) {
    return bm.whnf({e, 0}).skeleton;
}

struct BindMapTest : public ::testing::Test {
protected:
    test_functors functors;
    globalizer g;
    bind_map bm{g};
    expr var0{expr::var{0}};
    expr var1{expr::var{1}};
    expr var2{expr::var{2}};
    expr func{expr::functor{functors.id("f"), {}}};
    expr func2{expr::functor{functors.id("g"), {}}};
};

TEST_F(BindMapTest, WhnfUnboundVariableReturnsSelf) {
    EXPECT_EQ(whnf0(bm, &var0), &var0);
}

TEST_F(BindMapTest, WhnfFunctorReturnsSelf) {
    EXPECT_EQ(whnf0(bm, &func), &func);
}

TEST_F(BindMapTest, WhnfBoundVarReturnsBoundExpr) {
    bm.bind(0, {&func, 0});
    EXPECT_EQ(whnf0(bm, &var0), &func);
}

TEST_F(BindMapTest, BindAcceptsYoungerVarBoundToOlderVar) {
    bm.bind(1, {&var0, 0});
    EXPECT_EQ(whnf0(bm, &var1), &var0);
}

TEST_F(BindMapTest, BindRejectsOlderVarBoundToYoungerVar) {
    EXPECT_THROW(bm.bind(0, {&var1, 0}), std::logic_error);
}

TEST_F(BindMapTest, BindRejectsDuplicateBinding) {
    bm.bind(0, {&func, 0});
    EXPECT_THROW(bm.bind(0, {&func2, 0}), std::logic_error);
}

TEST_F(BindMapTest, WhnfChainVar0ToVar1ToFunctorReturnsFunctor) {
    bm.bind(1, {&var0, 0});
    bm.bind(0, {&func, 0});
    EXPECT_EQ(whnf0(bm, &var0), &func);
    EXPECT_EQ(whnf0(bm, &var1), &func);
}

TEST_F(BindMapTest, WhnfPathCompressionUpdatesChainWithoutPublicRebind) {
    bm.bind(1, {&var0, 0});
    bm.bind(0, {&func, 0});

    EXPECT_EQ(whnf0(bm, &var0), &func);
    EXPECT_EQ(whnf0(bm, &var1), &func);

    EXPECT_THROW(bm.bind(0, {&func2, 0}), std::logic_error);
    EXPECT_THROW(bm.bind(1, {&var0, 0}), std::logic_error);
}

TEST_F(BindMapTest, WhnfDoesNotRecurseIntoFunctorArguments) {
    expr fa{expr::functor{functors.id("f"), {&var0}}};
    bm.bind(0, {&func, 0});
    EXPECT_EQ(whnf0(bm, &fa), &fa);
}

TEST_F(BindMapTest, WhnfBoundToBinaryFunctor) {
    expr v0{expr::var{0}};
    expr v1{expr::var{1}};
    expr fab{expr::functor{functors.id("f"), {&v0, &v1}}};
    bm.bind(0, {&fab, 0});
    EXPECT_EQ(whnf0(bm, &v0), &fab);
}

TEST_F(BindMapTest, WhnfBoundToTernaryFunctor) {
    expr v0{expr::var{0}};
    expr v1{expr::var{1}};
    expr v2{expr::var{2}};
    expr habc{expr::functor{functors.id("h"), {&v0, &v1, &v2}}};
    bm.bind(0, {&habc, 0});
    EXPECT_EQ(whnf0(bm, &v0), &habc);
}

TEST_F(BindMapTest, WhnfChainOfLength3ResolvesAllIntermediateVars) {
    bm.bind(1, {&var0, 0});
    bm.bind(2, {&var1, 0});
    bm.bind(0, {&func, 0});
    EXPECT_EQ(whnf0(bm, &var0), &func);
    EXPECT_EQ(whnf0(bm, &var1), &func);
    EXPECT_EQ(whnf0(bm, &var2), &func);
}

TEST_F(BindMapTest, ClearBindingsRemovesAllBindings) {
    bm.clear_bindings();
    bm.bind(1, {&var0, 0});
    bm.bind(0, {&func, 0});
    EXPECT_EQ(whnf0(bm, &var0), &func);

    bm.clear_bindings();

    EXPECT_EQ(whnf0(bm, &var0), &var0);
    EXPECT_EQ(whnf0(bm, &var1), &var1);
}

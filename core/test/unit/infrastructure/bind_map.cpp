// bind_map stores variable renamings and implements WHNF with path compression over
// var chains. Unit tests exercise bind/whnf directly (no collaborators to mock).

#include <gtest/gtest.h>
#include "infrastructure/bind_map.hpp"
#include "functor_fixture.hpp"

struct BindMapTest : public ::testing::Test {
protected:
    test_functors functors;
    bind_map bm;
    expr var0{expr::var{0}};
    expr var1{expr::var{1}};
    expr var2{expr::var{2}};
    expr func{expr::functor{functors.id("f"), {}}};
    expr func2{expr::functor{functors.id("g"), {}}};
};

TEST_F(BindMapTest, WhnfUnboundVariableReturnsSelf) {
    EXPECT_EQ(bm.whnf(&var0), &var0);
}

TEST_F(BindMapTest, WhnfFunctorReturnsSelf) {
    EXPECT_EQ(bm.whnf(&func), &func);
}

TEST_F(BindMapTest, WhnfBoundVarReturnsBoundExpr) {
    bm.bind(0, &func);
    EXPECT_EQ(bm.whnf(&var0), &func);
}

TEST_F(BindMapTest, BindAcceptsYoungerVarBoundToOlderVar) {
    bm.bind(1, &var0);
    EXPECT_EQ(bm.whnf(&var1), &var0);
}

TEST_F(BindMapTest, BindRejectsOlderVarBoundToYoungerVar) {
    EXPECT_THROW(bm.bind(0, &var1), std::logic_error);
}

TEST_F(BindMapTest, BindRejectsDuplicateBinding) {
    bm.bind(0, &func);
    EXPECT_THROW(bm.bind(0, &func2), std::logic_error);
}

TEST_F(BindMapTest, WhnfChainVar0ToVar1ToFunctorReturnsFunctor) {
    bm.bind(1, &var0);
    bm.bind(0, &func);
    EXPECT_EQ(bm.whnf(&var0), &func);
    EXPECT_EQ(bm.whnf(&var1), &func);
}

TEST_F(BindMapTest, WhnfPathCompressionUpdatesChainWithoutPublicRebind) {
    bm.bind(1, &var0);
    bm.bind(0, &func);

    EXPECT_EQ(bm.whnf(&var0), &func);
    EXPECT_EQ(bm.whnf(&var1), &func);

    EXPECT_THROW(bm.bind(0, &func2), std::logic_error);
    EXPECT_THROW(bm.bind(1, &var0), std::logic_error);
}

TEST_F(BindMapTest, WhnfDoesNotRecurseIntoFunctorArguments) {
    expr fa{expr::functor{functors.id("f"), {&var0}}};
    bm.bind(0, &func);
    EXPECT_EQ(bm.whnf(&fa), &fa);
}

TEST_F(BindMapTest, WhnfBoundToBinaryFunctor) {
    expr v0{expr::var{0}};
    expr v1{expr::var{1}};
    expr fab{expr::functor{functors.id("f"), {&v0, &v1}}};
    bm.bind(0, &fab);
    EXPECT_EQ(bm.whnf(&v0), &fab);
}

TEST_F(BindMapTest, WhnfBoundToTernaryFunctor) {
    expr v0{expr::var{0}};
    expr v1{expr::var{1}};
    expr v2{expr::var{2}};
    expr habc{expr::functor{functors.id("h"), {&v0, &v1, &v2}}};
    bm.bind(0, &habc);
    EXPECT_EQ(bm.whnf(&v0), &habc);
}

TEST_F(BindMapTest, WhnfChainOfLength3ResolvesAllIntermediateVars) {
    bm.bind(1, &var0);
    bm.bind(2, &var1);
    bm.bind(0, &func);
    EXPECT_EQ(bm.whnf(&var0), &func);
    EXPECT_EQ(bm.whnf(&var1), &func);
    EXPECT_EQ(bm.whnf(&var2), &func);
}

TEST_F(BindMapTest, ClearBindingsRemovesAllBindings) {
    bm.clear_bindings();
    bm.bind(1, &var0);
    bm.bind(0, &func);
    EXPECT_EQ(bm.whnf(&var0), &func);

    bm.clear_bindings();

    EXPECT_EQ(bm.whnf(&var0), &var0);
    EXPECT_EQ(bm.whnf(&var1), &var1);
}

#include <gtest/gtest.h>
#include "../../../core/hpp/infrastructure/bind_map.hpp"

class BindMapTest : public ::testing::Test {
protected:
    bind_map bm;
    expr var0{expr::var{0}};
    expr var1{expr::var{1}};
    expr var2{expr::var{2}};
    expr func{expr::functor{"f", {}}};
    expr func2{expr::functor{"g", {}}};
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

TEST_F(BindMapTest, WhnfChainVar0ToVar1ToFunctorReturnsFunctor) {
    bm.bind(0, &var1);
    bm.bind(1, &func);
    EXPECT_EQ(bm.whnf(&var0), &func);
}

TEST_F(BindMapTest, PathCompressionAfterChainResolutionVar0IgnoresReboundVar1) {
    bm.bind(0, &var1);
    bm.bind(1, &func);
    bm.whnf(&var0);
    bm.bind(1, &func2);
    EXPECT_EQ(bm.whnf(&var0), &func);
}

TEST_F(BindMapTest, BindOverwritesExistingBinding) {
    bm.bind(0, &func);
    bm.bind(0, &var1);
    EXPECT_EQ(bm.whnf(&var0), &var1);
}

TEST_F(BindMapTest, WhnfDoesNotRecurseIntoFunctorArguments) {
    expr fa{expr::functor{"f", {&var0}}};
    bm.bind(0, &func);
    EXPECT_EQ(bm.whnf(&fa), &fa);
}

TEST_F(BindMapTest, ChainOfLength3CollapsesWithPathCompression) {
    bm.bind(0, &var1);
    bm.bind(1, &var2);
    bm.bind(2, &func);
    EXPECT_EQ(bm.whnf(&var0), &func);
    EXPECT_EQ(bm.whnf(&var1), &func);
    EXPECT_EQ(bm.whnf(&var2), &func);
}

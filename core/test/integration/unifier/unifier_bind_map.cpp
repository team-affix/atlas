#include <gtest/gtest.h>
#include "../../../../core/hpp/infrastructure/unifier.hpp"
#include "../../../../core/hpp/infrastructure/bind_map.hpp"

class UnifierBindMapTest : public ::testing::Test {
protected:
    void SetUp() override {
        u = std::make_unique<unifier>(bm);
    }

    bind_map bm;
    std::unique_ptr<unifier> u;
    std::unordered_set<uint32_t> snk;
    expr var0{expr::var{0}};
    expr var1{expr::var{1}};
    expr var2{expr::var{2}};
    expr func{expr::functor{"f", {}}};
    expr func2{expr::functor{"g", {}}};
};

TEST_F(UnifierBindMapTest, UnifyVarAndFunctorBindsVarInRealBindMap) {
    EXPECT_TRUE(u->unify(&var0, &func, snk));
    EXPECT_EQ(snk, (std::unordered_set<uint32_t>{0}));
    EXPECT_EQ(bm.whnf(&var0), &func);
}

TEST_F(UnifierBindMapTest, UnifyTwoDistinctVarsBindsYoungerInRealBindMap) {
    EXPECT_TRUE(u->unify(&var0, &var1, snk));
    EXPECT_EQ(snk, (std::unordered_set<uint32_t>{1}));
    EXPECT_EQ(bm.whnf(&var1), &var0);
}

TEST_F(UnifierBindMapTest, UnifyAlreadyBoundVarThroughWhnfSucceeds) {
    bm.bind(0, &func);
    EXPECT_TRUE(u->unify(&var0, &func, snk));
    EXPECT_TRUE(snk.empty());
}

TEST_F(UnifierBindMapTest, UnifyChainedVarsThroughBindMapSucceeds) {
    std::unordered_set<uint32_t> snk2;
    EXPECT_TRUE(u->unify(&var0, &var1, snk));
    EXPECT_TRUE(u->unify(&var1, &func, snk2));
    EXPECT_EQ(bm.whnf(&var0), &func);
}

TEST_F(UnifierBindMapTest, UnifyVarToFunctorContainingSameVarFails) {
    expr f_var0{expr::functor{"f", {&var0}}};
    EXPECT_FALSE(u->unify(&var0, &f_var0, snk));
    EXPECT_TRUE(snk.empty());
    EXPECT_EQ(bm.whnf(&var0), &var0);
}

TEST_F(UnifierBindMapTest, UnifyBinaryFunctorsWithVarArgsBindsBoth) {
    expr lhs{expr::functor{"f", {&var0, &var1}}};
    expr rhs{expr::functor{"f", {&func, &func2}}};
    EXPECT_TRUE(u->unify(&lhs, &rhs, snk));
    EXPECT_EQ(snk, (std::unordered_set<uint32_t>{0, 1}));
    EXPECT_EQ(bm.whnf(&var0), &func);
    EXPECT_EQ(bm.whnf(&var1), &func2);
}

TEST_F(UnifierBindMapTest, UnifyTernaryFunctorsWithVarArgsBindsAll) {
    expr lhs{expr::functor{"f", {&var0, &var1, &var2}}};
    expr rhs{expr::functor{"f", {&func, &func2, &func}}};
    EXPECT_TRUE(u->unify(&lhs, &rhs, snk));
    EXPECT_EQ(snk, (std::unordered_set<uint32_t>{0, 1, 2}));
    EXPECT_EQ(bm.whnf(&var0), &func);
    EXPECT_EQ(bm.whnf(&var1), &func2);
    EXPECT_EQ(bm.whnf(&var2), &func);
}

TEST_F(UnifierBindMapTest, UnifyDepth2FunctorsBindsInnerVar) {
    expr f_var0{expr::functor{"f", {&var0}}};
    expr f_func{expr::functor{"f", {&func}}};
    expr lhs{expr::functor{"f", {&f_var0}}};
    expr rhs{expr::functor{"f", {&f_func}}};
    EXPECT_TRUE(u->unify(&lhs, &rhs, snk));
    EXPECT_EQ(snk, (std::unordered_set<uint32_t>{0}));
    EXPECT_EQ(bm.whnf(&var0), &func);
}

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "../../../core/hpp/infrastructure/unifier.hpp"

using ::testing::Return;
using ::testing::InSequence;

class MockBindMap : public i_bind_map {
public:
    MOCK_METHOD(void,        bind, (uint32_t, const expr*), (override));
    MOCK_METHOD(const expr*, whnf, (const expr*),           (override));
};

class UnifierTest : public ::testing::Test {
protected:
    void SetUp() override {
        auto uptr = std::make_unique<MockBindMap>();
        bm = uptr.get();
        u = std::make_unique<unifier>(std::move(uptr));
    }

    MockBindMap* bm;
    std::unique_ptr<unifier> u;
    std::unordered_set<uint32_t> snk;

    expr var0{expr::var{0}};
    expr var1{expr::var{1}};
    expr var2{expr::var{2}};
    expr func{expr::functor{"f", {}}};
    expr func2{expr::functor{"g", {}}};

    expr f_var0{expr::functor{"f", {&var0}}};
    expr f_var1{expr::functor{"f", {&var1}}};
    expr g_var0{expr::functor{"g", {&var0}}};
    expr f_func{expr::functor{"f", {&func}}};

    expr f_f_var0{expr::functor{"f", {&f_var0}}};
    expr f_f_var1{expr::functor{"f", {&f_var1}}};
    expr f_f_func{expr::functor{"f", {&f_func}}};
    expr g_f_var0{expr::functor{"g", {&f_var0}}};

    expr f2_var0_var1{expr::functor{"f", {&var0, &var1}}};
    expr f2_func_func2{expr::functor{"f", {&func, &func2}}};
    expr f2_func_var1{expr::functor{"f", {&func, &var1}}};

    expr f3_var0_var1_var2{expr::functor{"f", {&var0, &var1, &var2}}};
    expr f3_func_func2_func{expr::functor{"f", {&func, &func2, &func}}};
};

// ---------------------------------------------------------------------------
// Both sides are variables
// ---------------------------------------------------------------------------

TEST_F(UnifierTest, UnifyIdenticalVarsBothSidesReturnsTrueNoBind) {
    EXPECT_CALL(*bm, bind).Times(0);
    InSequence seq;
    EXPECT_CALL(*bm, whnf(&var0)).WillOnce(Return(&var0));
    EXPECT_CALL(*bm, whnf(&var0)).WillOnce(Return(&var0));

    EXPECT_TRUE(u->unify(&var0, &var0, snk));
    EXPECT_TRUE(snk.empty());
}

TEST_F(UnifierTest, UnifyVarLhsVarRhsBindsHigherIndexToLower) {
    InSequence seq;
    EXPECT_CALL(*bm, whnf(&var0)).WillOnce(Return(&var0));
    EXPECT_CALL(*bm, whnf(&var1)).WillOnce(Return(&var1));
    EXPECT_CALL(*bm, whnf(&var0)).WillOnce(Return(&var0));  // occurs_check
    EXPECT_CALL(*bm, bind(1u, &var0));

    EXPECT_TRUE(u->unify(&var0, &var1, snk));
    EXPECT_EQ(snk, (std::unordered_set<uint32_t>{1}));
}

TEST_F(UnifierTest, UnifyVarRhsVarLhsBindsHigherIndexToLower) {
    InSequence seq;
    EXPECT_CALL(*bm, whnf(&var1)).WillOnce(Return(&var1));
    EXPECT_CALL(*bm, whnf(&var0)).WillOnce(Return(&var0));
    EXPECT_CALL(*bm, whnf(&var0)).WillOnce(Return(&var0));  // occurs_check
    EXPECT_CALL(*bm, bind(1u, &var0));

    EXPECT_TRUE(u->unify(&var1, &var0, snk));
    EXPECT_EQ(snk, (std::unordered_set<uint32_t>{1}));
}

// ---------------------------------------------------------------------------
// One side is a variable
// ---------------------------------------------------------------------------

TEST_F(UnifierTest, UnifyVarLhsAndFunctorRhsBindsVarToFunctor) {
    InSequence seq;
    EXPECT_CALL(*bm, whnf(&var0)).WillOnce(Return(&var0));
    EXPECT_CALL(*bm, whnf(&func)).WillOnce(Return(&func));
    EXPECT_CALL(*bm, whnf(&func)).WillOnce(Return(&func));  // occurs_check
    EXPECT_CALL(*bm, bind(0u, &func));

    EXPECT_TRUE(u->unify(&var0, &func, snk));
    EXPECT_EQ(snk, (std::unordered_set<uint32_t>{0}));
}

TEST_F(UnifierTest, UnifyFunctorLhsAndVarRhsBindsVarToFunctor) {
    InSequence seq;
    EXPECT_CALL(*bm, whnf(&func)).WillOnce(Return(&func));
    EXPECT_CALL(*bm, whnf(&var0)).WillOnce(Return(&var0));
    EXPECT_CALL(*bm, whnf(&func)).WillOnce(Return(&func));  // occurs_check
    EXPECT_CALL(*bm, bind(0u, &func));

    EXPECT_TRUE(u->unify(&func, &var0, snk));
    EXPECT_EQ(snk, (std::unordered_set<uint32_t>{0}));
}

TEST_F(UnifierTest, UnifyVarAndFunctorWithArgPassesOccursCheck) {
    InSequence seq;
    EXPECT_CALL(*bm, whnf(&var0)).WillOnce(Return(&var0));
    EXPECT_CALL(*bm, whnf(&f_var1)).WillOnce(Return(&f_var1));
    EXPECT_CALL(*bm, whnf(&f_var1)).WillOnce(Return(&f_var1));  // occurs_check outer
    EXPECT_CALL(*bm, whnf(&var1)).WillOnce(Return(&var1));      // occurs_check recurse arg
    EXPECT_CALL(*bm, bind(0u, &f_var1));

    EXPECT_TRUE(u->unify(&var0, &f_var1, snk));
    EXPECT_EQ(snk, (std::unordered_set<uint32_t>{0}));
}

// ---------------------------------------------------------------------------
// Occurs check passes through deep nesting
// ---------------------------------------------------------------------------

TEST_F(UnifierTest, UnifyVarAndDepth2FunctorNotContainingVarBinds) {
    InSequence seq;
    EXPECT_CALL(*bm, whnf(&var0)).WillOnce(Return(&var0));
    EXPECT_CALL(*bm, whnf(&f_f_var1)).WillOnce(Return(&f_f_var1));
    EXPECT_CALL(*bm, whnf(&f_f_var1)).WillOnce(Return(&f_f_var1));  // occurs_check depth-0
    EXPECT_CALL(*bm, whnf(&f_var1)).WillOnce(Return(&f_var1));      // occurs_check depth-1
    EXPECT_CALL(*bm, whnf(&var1)).WillOnce(Return(&var1));          // occurs_check depth-2
    EXPECT_CALL(*bm, bind(0u, &f_f_var1));

    EXPECT_TRUE(u->unify(&var0, &f_f_var1, snk));
    EXPECT_EQ(snk, (std::unordered_set<uint32_t>{0}));
}

TEST_F(UnifierTest, UnifyVarAndMixedFunctorNotContainingVarBinds) {
    InSequence seq;
    EXPECT_CALL(*bm, whnf(&var1)).WillOnce(Return(&var1));
    EXPECT_CALL(*bm, whnf(&g_f_var0)).WillOnce(Return(&g_f_var0));
    EXPECT_CALL(*bm, whnf(&g_f_var0)).WillOnce(Return(&g_f_var0));  // occurs_check depth-0
    EXPECT_CALL(*bm, whnf(&f_var0)).WillOnce(Return(&f_var0));      // occurs_check depth-1
    EXPECT_CALL(*bm, whnf(&var0)).WillOnce(Return(&var0));          // occurs_check depth-2
    EXPECT_CALL(*bm, bind(1u, &g_f_var0));

    EXPECT_TRUE(u->unify(&var1, &g_f_var0, snk));
    EXPECT_EQ(snk, (std::unordered_set<uint32_t>{1}));
}

// ---------------------------------------------------------------------------
// Occurs check failure
// ---------------------------------------------------------------------------

TEST_F(UnifierTest, UnifyVarToFunctorContainingSameVarFails) {
    EXPECT_CALL(*bm, bind).Times(0);
    InSequence seq;
    EXPECT_CALL(*bm, whnf(&var0)).WillOnce(Return(&var0));
    EXPECT_CALL(*bm, whnf(&f_var0)).WillOnce(Return(&f_var0));
    EXPECT_CALL(*bm, whnf(&f_var0)).WillOnce(Return(&f_var0));  // occurs_check outer
    EXPECT_CALL(*bm, whnf(&var0)).WillOnce(Return(&var0));      // occurs_check recurse arg

    EXPECT_FALSE(u->unify(&var0, &f_var0, snk));
    EXPECT_TRUE(snk.empty());
}

TEST_F(UnifierTest, UnifyVarToNestedFunctorContainingSameVarFails) {
    EXPECT_CALL(*bm, bind).Times(0);
    InSequence seq;
    EXPECT_CALL(*bm, whnf(&var1)).WillOnce(Return(&var1));
    EXPECT_CALL(*bm, whnf(&f_var1)).WillOnce(Return(&f_var1));
    EXPECT_CALL(*bm, whnf(&f_var1)).WillOnce(Return(&f_var1));  // occurs_check outer
    EXPECT_CALL(*bm, whnf(&var1)).WillOnce(Return(&var1));      // occurs_check recurse arg

    EXPECT_FALSE(u->unify(&var1, &f_var1, snk));
    EXPECT_TRUE(snk.empty());
}

TEST_F(UnifierTest, UnifyVarToDepth2FunctorContainingSameVarFails) {
    EXPECT_CALL(*bm, bind).Times(0);
    InSequence seq;
    EXPECT_CALL(*bm, whnf(&var0)).WillOnce(Return(&var0));
    EXPECT_CALL(*bm, whnf(&f_f_var0)).WillOnce(Return(&f_f_var0));
    EXPECT_CALL(*bm, whnf(&f_f_var0)).WillOnce(Return(&f_f_var0));  // occurs_check depth-0
    EXPECT_CALL(*bm, whnf(&f_var0)).WillOnce(Return(&f_var0));      // occurs_check depth-1
    EXPECT_CALL(*bm, whnf(&var0)).WillOnce(Return(&var0));          // occurs_check depth-2 (match!)

    EXPECT_FALSE(u->unify(&var0, &f_f_var0, snk));
    EXPECT_TRUE(snk.empty());
}

// ---------------------------------------------------------------------------
// Both sides are functors
// ---------------------------------------------------------------------------

TEST_F(UnifierTest, UnifyIdenticalNullaryFunctorsReturnsTrue) {
    EXPECT_CALL(*bm, bind).Times(0);
    InSequence seq;
    EXPECT_CALL(*bm, whnf(&func)).WillOnce(Return(&func));
    EXPECT_CALL(*bm, whnf(&func)).WillOnce(Return(&func));

    EXPECT_TRUE(u->unify(&func, &func, snk));
    EXPECT_TRUE(snk.empty());
}

TEST_F(UnifierTest, UnifyFunctorsWithDifferentNamesFails) {
    EXPECT_CALL(*bm, bind).Times(0);
    InSequence seq;
    EXPECT_CALL(*bm, whnf(&func)).WillOnce(Return(&func));
    EXPECT_CALL(*bm, whnf(&func2)).WillOnce(Return(&func2));

    EXPECT_FALSE(u->unify(&func, &func2, snk));
}

TEST_F(UnifierTest, UnifyFunctorsWithDifferentArityFails) {
    EXPECT_CALL(*bm, bind).Times(0);
    InSequence seq;
    EXPECT_CALL(*bm, whnf(&func)).WillOnce(Return(&func));
    EXPECT_CALL(*bm, whnf(&f_var0)).WillOnce(Return(&f_var0));

    EXPECT_FALSE(u->unify(&func, &f_var0, snk));
}

TEST_F(UnifierTest, UnifyFunctorsWithOneVarArgRecursivelyBindsArg) {
    InSequence seq;
    EXPECT_CALL(*bm, whnf(&f_var0)).WillOnce(Return(&f_var0));
    EXPECT_CALL(*bm, whnf(&f_func)).WillOnce(Return(&f_func));
    EXPECT_CALL(*bm, whnf(&var0)).WillOnce(Return(&var0));   // recursive unify
    EXPECT_CALL(*bm, whnf(&func)).WillOnce(Return(&func));
    EXPECT_CALL(*bm, whnf(&func)).WillOnce(Return(&func));   // occurs_check
    EXPECT_CALL(*bm, bind(0u, &func));

    EXPECT_TRUE(u->unify(&f_var0, &f_func, snk));
    EXPECT_EQ(snk, (std::unordered_set<uint32_t>{0}));
}

TEST_F(UnifierTest, UnifyFunctorsWithOneArgFailsIfArgsDiffer) {
    expr f_of_func2{expr::functor{"f", {&func2}}};

    EXPECT_CALL(*bm, bind).Times(0);
    InSequence seq;
    EXPECT_CALL(*bm, whnf(&f_func)).WillOnce(Return(&f_func));
    EXPECT_CALL(*bm, whnf(&f_of_func2)).WillOnce(Return(&f_of_func2));
    EXPECT_CALL(*bm, whnf(&func)).WillOnce(Return(&func));    // recursive unify arg
    EXPECT_CALL(*bm, whnf(&func2)).WillOnce(Return(&func2));

    EXPECT_FALSE(u->unify(&f_func, &f_of_func2, snk));
}

TEST_F(UnifierTest, UnifyBinaryFunctorsWithTwoVarArgsBindsBothAndPopulatesSnk) {
    InSequence seq;
    EXPECT_CALL(*bm, whnf(&f2_var0_var1)).WillOnce(Return(&f2_var0_var1));
    EXPECT_CALL(*bm, whnf(&f2_func_func2)).WillOnce(Return(&f2_func_func2));
    EXPECT_CALL(*bm, whnf(&var0)).WillOnce(Return(&var0));    // arg 0 unify
    EXPECT_CALL(*bm, whnf(&func)).WillOnce(Return(&func));
    EXPECT_CALL(*bm, whnf(&func)).WillOnce(Return(&func));    // arg 0 occurs_check
    EXPECT_CALL(*bm, bind(0u, &func));
    EXPECT_CALL(*bm, whnf(&var1)).WillOnce(Return(&var1));    // arg 1 unify
    EXPECT_CALL(*bm, whnf(&func2)).WillOnce(Return(&func2));
    EXPECT_CALL(*bm, whnf(&func2)).WillOnce(Return(&func2));  // arg 1 occurs_check
    EXPECT_CALL(*bm, bind(1u, &func2));

    EXPECT_TRUE(u->unify(&f2_var0_var1, &f2_func_func2, snk));
    EXPECT_EQ(snk, (std::unordered_set<uint32_t>{0, 1}));
}

TEST_F(UnifierTest, UnifyBinaryFunctorsWithMixedArgsOnlyBindsVarArg) {
    InSequence seq;
    EXPECT_CALL(*bm, whnf(&f2_func_var1)).WillOnce(Return(&f2_func_var1));
    EXPECT_CALL(*bm, whnf(&f2_func_func2)).WillOnce(Return(&f2_func_func2));
    EXPECT_CALL(*bm, whnf(&func)).WillOnce(Return(&func));    // arg 0 unify lhs
    EXPECT_CALL(*bm, whnf(&func)).WillOnce(Return(&func));    // arg 0 unify rhs
    EXPECT_CALL(*bm, whnf(&var1)).WillOnce(Return(&var1));    // arg 1 unify
    EXPECT_CALL(*bm, whnf(&func2)).WillOnce(Return(&func2));
    EXPECT_CALL(*bm, whnf(&func2)).WillOnce(Return(&func2));  // arg 1 occurs_check
    EXPECT_CALL(*bm, bind(1u, &func2));

    EXPECT_TRUE(u->unify(&f2_func_var1, &f2_func_func2, snk));
    EXPECT_EQ(snk, (std::unordered_set<uint32_t>{1}));
}

TEST_F(UnifierTest, UnifyBinaryFunctorsFirstArgFailsSecondNeverAttempted) {
    expr rhs{expr::functor{"f", {&func2, &func2}}};

    EXPECT_CALL(*bm, bind).Times(0);
    InSequence seq;
    EXPECT_CALL(*bm, whnf(&f2_func_var1)).WillOnce(Return(&f2_func_var1));
    EXPECT_CALL(*bm, whnf(&rhs)).WillOnce(Return(&rhs));
    EXPECT_CALL(*bm, whnf(&func)).WillOnce(Return(&func));    // arg 0 unify lhs
    EXPECT_CALL(*bm, whnf(&func2)).WillOnce(Return(&func2));  // arg 0 unify rhs (f≠g)

    EXPECT_FALSE(u->unify(&f2_func_var1, &rhs, snk));
    EXPECT_TRUE(snk.empty());
}

TEST_F(UnifierTest, UnifyBinaryFunctorsSecondArgFailsAfterFirstBinds) {
    expr lhs{expr::functor{"f", {&var0, &func}}};

    InSequence seq;
    EXPECT_CALL(*bm, whnf(&lhs)).WillOnce(Return(&lhs));
    EXPECT_CALL(*bm, whnf(&f2_func_func2)).WillOnce(Return(&f2_func_func2));
    EXPECT_CALL(*bm, whnf(&var0)).WillOnce(Return(&var0));    // arg 0 unify
    EXPECT_CALL(*bm, whnf(&func)).WillOnce(Return(&func));
    EXPECT_CALL(*bm, whnf(&func)).WillOnce(Return(&func));    // arg 0 occurs_check
    EXPECT_CALL(*bm, bind(0u, &func));
    EXPECT_CALL(*bm, whnf(&func)).WillOnce(Return(&func));    // arg 1 unify lhs (f())
    EXPECT_CALL(*bm, whnf(&func2)).WillOnce(Return(&func2));  // arg 1 unify rhs (g()), f≠g

    EXPECT_FALSE(u->unify(&lhs, &f2_func_func2, snk));
    EXPECT_EQ(snk, (std::unordered_set<uint32_t>{0}));
}

TEST_F(UnifierTest, UnifyTernaryFunctorsWithThreeVarArgsBindsAllThree) {
    InSequence seq;
    EXPECT_CALL(*bm, whnf(&f3_var0_var1_var2)).WillOnce(Return(&f3_var0_var1_var2));
    EXPECT_CALL(*bm, whnf(&f3_func_func2_func)).WillOnce(Return(&f3_func_func2_func));
    EXPECT_CALL(*bm, whnf(&var0)).WillOnce(Return(&var0));
    EXPECT_CALL(*bm, whnf(&func)).WillOnce(Return(&func));
    EXPECT_CALL(*bm, whnf(&func)).WillOnce(Return(&func));    // occurs_check
    EXPECT_CALL(*bm, bind(0u, &func));
    EXPECT_CALL(*bm, whnf(&var1)).WillOnce(Return(&var1));
    EXPECT_CALL(*bm, whnf(&func2)).WillOnce(Return(&func2));
    EXPECT_CALL(*bm, whnf(&func2)).WillOnce(Return(&func2));  // occurs_check
    EXPECT_CALL(*bm, bind(1u, &func2));
    EXPECT_CALL(*bm, whnf(&var2)).WillOnce(Return(&var2));
    EXPECT_CALL(*bm, whnf(&func)).WillOnce(Return(&func));
    EXPECT_CALL(*bm, whnf(&func)).WillOnce(Return(&func));    // occurs_check
    EXPECT_CALL(*bm, bind(2u, &func));

    EXPECT_TRUE(u->unify(&f3_var0_var1_var2, &f3_func_func2_func, snk));
    EXPECT_EQ(snk, (std::unordered_set<uint32_t>{0, 1, 2}));
}

TEST_F(UnifierTest, UnifyTernaryFunctorsThirdArgFailsAfterFirstTwoBind) {
    expr lhs{expr::functor{"f", {&var0, &var1, &func2}}};

    InSequence seq;
    EXPECT_CALL(*bm, whnf(&lhs)).WillOnce(Return(&lhs));
    EXPECT_CALL(*bm, whnf(&f3_func_func2_func)).WillOnce(Return(&f3_func_func2_func));
    EXPECT_CALL(*bm, whnf(&var0)).WillOnce(Return(&var0));
    EXPECT_CALL(*bm, whnf(&func)).WillOnce(Return(&func));
    EXPECT_CALL(*bm, whnf(&func)).WillOnce(Return(&func));    // occurs_check
    EXPECT_CALL(*bm, bind(0u, &func));
    EXPECT_CALL(*bm, whnf(&var1)).WillOnce(Return(&var1));
    EXPECT_CALL(*bm, whnf(&func2)).WillOnce(Return(&func2));
    EXPECT_CALL(*bm, whnf(&func2)).WillOnce(Return(&func2));  // occurs_check
    EXPECT_CALL(*bm, bind(1u, &func2));
    EXPECT_CALL(*bm, whnf(&func2)).WillOnce(Return(&func2));  // arg 2 lhs: g()
    EXPECT_CALL(*bm, whnf(&func)).WillOnce(Return(&func));    // arg 2 rhs: f() — g≠f

    EXPECT_FALSE(u->unify(&lhs, &f3_func_func2_func, snk));
    EXPECT_EQ(snk, (std::unordered_set<uint32_t>{0, 1}));
}

TEST_F(UnifierTest, UnifyDepth2FunctorsRecursivelyBindsInnerVar) {
    InSequence seq;
    EXPECT_CALL(*bm, whnf(&f_f_var0)).WillOnce(Return(&f_f_var0));
    EXPECT_CALL(*bm, whnf(&f_f_func)).WillOnce(Return(&f_f_func));
    EXPECT_CALL(*bm, whnf(&f_var0)).WillOnce(Return(&f_var0));   // recurse depth-1 lhs
    EXPECT_CALL(*bm, whnf(&f_func)).WillOnce(Return(&f_func));   // recurse depth-1 rhs
    EXPECT_CALL(*bm, whnf(&var0)).WillOnce(Return(&var0));       // recurse depth-2 lhs
    EXPECT_CALL(*bm, whnf(&func)).WillOnce(Return(&func));       // recurse depth-2 rhs
    EXPECT_CALL(*bm, whnf(&func)).WillOnce(Return(&func));       // occurs_check
    EXPECT_CALL(*bm, bind(0u, &func));

    EXPECT_TRUE(u->unify(&f_f_var0, &f_f_func, snk));
    EXPECT_EQ(snk, (std::unordered_set<uint32_t>{0}));
}

TEST_F(UnifierTest, UnifyDepth2FunctorsFailsOnInnerNameMismatch) {
    expr rhs{expr::functor{"f", {&g_var0}}};

    EXPECT_CALL(*bm, bind).Times(0);
    InSequence seq;
    EXPECT_CALL(*bm, whnf(&f_f_var0)).WillOnce(Return(&f_f_var0));
    EXPECT_CALL(*bm, whnf(&rhs)).WillOnce(Return(&rhs));
    EXPECT_CALL(*bm, whnf(&f_var0)).WillOnce(Return(&f_var0));   // recurse inner lhs
    EXPECT_CALL(*bm, whnf(&g_var0)).WillOnce(Return(&g_var0));   // recurse inner rhs (f≠g)

    EXPECT_FALSE(u->unify(&f_f_var0, &rhs, snk));
}

// ---------------------------------------------------------------------------
// whnf is called before branching — vars resolve through equivalence classes
// ---------------------------------------------------------------------------

TEST_F(UnifierTest, UnifyAfterWhnfBothResolveToSameFunctorReturnsTrue) {
    EXPECT_CALL(*bm, bind).Times(0);
    InSequence seq;
    EXPECT_CALL(*bm, whnf(&var0)).WillOnce(Return(&func));
    EXPECT_CALL(*bm, whnf(&var1)).WillOnce(Return(&func));

    EXPECT_TRUE(u->unify(&var0, &var1, snk));
    EXPECT_TRUE(snk.empty());
}

TEST_F(UnifierTest, UnifyAfterWhnfBothResolveToDifferentFunctorsFails) {
    EXPECT_CALL(*bm, bind).Times(0);
    InSequence seq;
    EXPECT_CALL(*bm, whnf(&var0)).WillOnce(Return(&func));
    EXPECT_CALL(*bm, whnf(&var1)).WillOnce(Return(&func2));

    EXPECT_FALSE(u->unify(&var0, &var1, snk));
}

TEST_F(UnifierTest, UnifyAfterWhnfBothResolveToVarsBindsRepresentatives) {
    // var0 is already in var2's eq class (whnf resolves it to var2)
    // var1 is its own representative; bind(2, &var1) merges the classes
    InSequence seq;
    EXPECT_CALL(*bm, whnf(&var0)).WillOnce(Return(&var2));
    EXPECT_CALL(*bm, whnf(&var1)).WillOnce(Return(&var1));
    EXPECT_CALL(*bm, whnf(&var1)).WillOnce(Return(&var1));  // occurs_check
    EXPECT_CALL(*bm, bind(2u, &var1));

    EXPECT_TRUE(u->unify(&var0, &var1, snk));
    EXPECT_EQ(snk, (std::unordered_set<uint32_t>{2}));
}

TEST_F(UnifierTest, UnifyAfterWhnfVarResolvesToVarFunctorBindsRepresentative) {
    // var0 is already in var2's eq class; var1 resolves to a functor
    // occurs_check and bind operate on var2 (the representative), not var0
    InSequence seq;
    EXPECT_CALL(*bm, whnf(&var0)).WillOnce(Return(&var2));
    EXPECT_CALL(*bm, whnf(&var1)).WillOnce(Return(&func));
    EXPECT_CALL(*bm, whnf(&func)).WillOnce(Return(&func));  // occurs_check
    EXPECT_CALL(*bm, bind(2u, &func));

    EXPECT_TRUE(u->unify(&var0, &var1, snk));
    EXPECT_EQ(snk, (std::unordered_set<uint32_t>{2}));
}

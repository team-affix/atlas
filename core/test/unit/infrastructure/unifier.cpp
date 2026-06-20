// unifier performs Robinson unification with occurs-check over expressions, delegating
// WHNF and bind to a bind_map. Unit tests mock the bind map and assert bind/vars_touched outcomes
// for variables, functors, failure paths, and WHNF-normalized representatives.

#include <unordered_set>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/globalizer.hpp"
#include "infrastructure/unifier.hpp"
#include "functor_fixture.hpp"

using ::testing::IsEmpty;
using ::testing::Return;
using ::testing::UnorderedElementsAre;

namespace {

struct MockBindMap {
    MOCK_METHOD(void,        bind, (uint32_t, framed_expr));
    MOCK_METHOD(framed_expr, whnf, (framed_expr));
};

using TestUnifier = unifier<MockBindMap>;

bool run_unify(TestUnifier& u, const expr* lhs, const expr* rhs,
               std::unordered_set<uint32_t>& vars_touched) {
    auto task = u.unify({lhs, 0}, {rhs, 0});
    while (!task.done()) {
        task.resume();
        if (task.has_yield())
            vars_touched.insert(task.consume_yield());
    }
    return task.result();
}

} // namespace

struct UnifierTest : public ::testing::Test {
    test_functors functors;
    MockBindMap bm;
    globalizer g;
    TestUnifier u{g, &bm};
    std::unordered_set<uint32_t> vars_touched;

    expr var0{expr::var{0}};
    expr var1{expr::var{1}};
    expr var2{expr::var{2}};
    expr func{expr::functor{functors.id("f"), {}}};
    expr func2{expr::functor{functors.id("g"), {}}};

    expr f_var0{expr::functor{functors.id("f"), {&var0}}};
    expr f_var1{expr::functor{functors.id("f"), {&var1}}};
    expr g_var0{expr::functor{functors.id("g"), {&var0}}};
    expr f_func{expr::functor{functors.id("f"), {&func}}};

    expr f_f_var0{expr::functor{functors.id("f"), {&f_var0}}};
    expr f_f_var1{expr::functor{functors.id("f"), {&f_var1}}};
    expr f_f_func{expr::functor{functors.id("f"), {&f_func}}};
    expr g_f_var0{expr::functor{functors.id("g"), {&f_var0}}};

    expr f2_var0_var1{expr::functor{functors.id("f"), {&var0, &var1}}};
    expr f2_func_func2{expr::functor{functors.id("f"), {&func, &func2}}};
    expr f2_func_var1{expr::functor{functors.id("f"), {&func, &var1}}};

    expr f3_var0_var1_var2{expr::functor{functors.id("f"), {&var0, &var1, &var2}}};
    expr f3_func_func2_func{expr::functor{functors.id("f"), {&func, &func2, &func}}};
};

// ---------------------------------------------------------------------------
// Both sides are variables
// ---------------------------------------------------------------------------

TEST_F(UnifierTest, UnifyIdenticalVarsBothSidesReturnsTrueNoBind) {
    EXPECT_CALL(bm, bind).Times(0);
    EXPECT_CALL(bm, whnf(framed_expr{&var0, 0})).WillRepeatedly(Return(framed_expr{&var0, 0}));

    EXPECT_TRUE(run_unify(u, &var0, &var0, vars_touched));
    EXPECT_THAT(vars_touched, IsEmpty());
}

TEST_F(UnifierTest, UnifyVarLhsVarRhsBindsHigherIndexToLower) {
    EXPECT_CALL(bm, whnf(framed_expr{&var0, 0})).WillRepeatedly(Return(framed_expr{&var0, 0}));
    EXPECT_CALL(bm, whnf(framed_expr{&var1, 0})).WillRepeatedly(Return(framed_expr{&var1, 0}));
    EXPECT_CALL(bm, bind(1u, framed_expr{&var0, 0}));

    EXPECT_TRUE(run_unify(u, &var0, &var1, vars_touched));
    EXPECT_THAT(vars_touched, UnorderedElementsAre(0u, 1u));
}

TEST_F(UnifierTest, UnifyVarRhsVarLhsBindsHigherIndexToLower) {
    EXPECT_CALL(bm, whnf(framed_expr{&var1, 0})).WillRepeatedly(Return(framed_expr{&var1, 0}));
    EXPECT_CALL(bm, whnf(framed_expr{&var0, 0})).WillRepeatedly(Return(framed_expr{&var0, 0}));
    EXPECT_CALL(bm, bind(1u, framed_expr{&var0, 0}));

    EXPECT_TRUE(run_unify(u, &var1, &var0, vars_touched));
    EXPECT_THAT(vars_touched, UnorderedElementsAre(0u, 1u));
}

// ---------------------------------------------------------------------------
// One side is a variable
// ---------------------------------------------------------------------------

TEST_F(UnifierTest, UnifyVarLhsAndFunctorRhsBindsVarToFunctor) {
    EXPECT_CALL(bm, whnf(framed_expr{&var0, 0})).WillRepeatedly(Return(framed_expr{&var0, 0}));
    EXPECT_CALL(bm, whnf(framed_expr{&func, 0})).WillRepeatedly(Return(framed_expr{&func, 0}));
    EXPECT_CALL(bm, bind(0u, framed_expr{&func, 0}));

    EXPECT_TRUE(run_unify(u, &var0, &func, vars_touched));
    EXPECT_THAT(vars_touched, UnorderedElementsAre(0u));
}

TEST_F(UnifierTest, UnifyFunctorLhsAndVarRhsBindsVarToFunctor) {
    EXPECT_CALL(bm, whnf(framed_expr{&func, 0})).WillRepeatedly(Return(framed_expr{&func, 0}));
    EXPECT_CALL(bm, whnf(framed_expr{&var0, 0})).WillRepeatedly(Return(framed_expr{&var0, 0}));
    EXPECT_CALL(bm, bind(0u, framed_expr{&func, 0}));

    EXPECT_TRUE(run_unify(u, &func, &var0, vars_touched));
    EXPECT_THAT(vars_touched, UnorderedElementsAre(0u));
}

TEST_F(UnifierTest, UnifyVarAndFunctorWithArgPassesOccursCheck) {
    EXPECT_CALL(bm, whnf(framed_expr{&var0, 0})).WillRepeatedly(Return(framed_expr{&var0, 0}));
    EXPECT_CALL(bm, whnf(framed_expr{&f_var1, 0})).WillRepeatedly(Return(framed_expr{&f_var1, 0}));
    EXPECT_CALL(bm, whnf(framed_expr{&var1, 0})).WillRepeatedly(Return(framed_expr{&var1, 0}));
    EXPECT_CALL(bm, bind(0u, framed_expr{&f_var1, 0}));

    EXPECT_TRUE(run_unify(u, &var0, &f_var1, vars_touched));
    EXPECT_THAT(vars_touched, UnorderedElementsAre(0u));
}

// ---------------------------------------------------------------------------
// Occurs check passes through deep nesting
// ---------------------------------------------------------------------------

TEST_F(UnifierTest, UnifyVarAndDepth2FunctorNotContainingVarBinds) {
    EXPECT_CALL(bm, whnf(framed_expr{&var0, 0})).WillRepeatedly(Return(framed_expr{&var0, 0}));
    EXPECT_CALL(bm, whnf(framed_expr{&f_f_var1, 0})).WillRepeatedly(Return(framed_expr{&f_f_var1, 0}));
    EXPECT_CALL(bm, whnf(framed_expr{&f_var1, 0})).WillRepeatedly(Return(framed_expr{&f_var1, 0}));
    EXPECT_CALL(bm, whnf(framed_expr{&var1, 0})).WillRepeatedly(Return(framed_expr{&var1, 0}));
    EXPECT_CALL(bm, bind(0u, framed_expr{&f_f_var1, 0}));

    EXPECT_TRUE(run_unify(u, &var0, &f_f_var1, vars_touched));
    EXPECT_THAT(vars_touched, UnorderedElementsAre(0u));
}

TEST_F(UnifierTest, UnifyVarAndMixedFunctorNotContainingVarBinds) {
    EXPECT_CALL(bm, whnf(framed_expr{&var1, 0})).WillRepeatedly(Return(framed_expr{&var1, 0}));
    EXPECT_CALL(bm, whnf(framed_expr{&g_f_var0, 0})).WillRepeatedly(Return(framed_expr{&g_f_var0, 0}));
    EXPECT_CALL(bm, whnf(framed_expr{&f_var0, 0})).WillRepeatedly(Return(framed_expr{&f_var0, 0}));
    EXPECT_CALL(bm, whnf(framed_expr{&var0, 0})).WillRepeatedly(Return(framed_expr{&var0, 0}));
    EXPECT_CALL(bm, bind(1u, framed_expr{&g_f_var0, 0}));

    EXPECT_TRUE(run_unify(u, &var1, &g_f_var0, vars_touched));
    EXPECT_THAT(vars_touched, UnorderedElementsAre(1u));
}

// ---------------------------------------------------------------------------
// Occurs check failure
// ---------------------------------------------------------------------------

TEST_F(UnifierTest, UnifyVarToFunctorContainingSameVarFails) {
    EXPECT_CALL(bm, bind).Times(0);
    EXPECT_CALL(bm, whnf(framed_expr{&var0, 0})).WillRepeatedly(Return(framed_expr{&var0, 0}));
    EXPECT_CALL(bm, whnf(framed_expr{&f_var0, 0})).WillRepeatedly(Return(framed_expr{&f_var0, 0}));

    EXPECT_FALSE(run_unify(u, &var0, &f_var0, vars_touched));
    EXPECT_THAT(vars_touched, IsEmpty());
}

TEST_F(UnifierTest, UnifyVarToNestedFunctorContainingSameVarFails) {
    EXPECT_CALL(bm, bind).Times(0);
    EXPECT_CALL(bm, whnf(framed_expr{&var1, 0})).WillRepeatedly(Return(framed_expr{&var1, 0}));
    EXPECT_CALL(bm, whnf(framed_expr{&f_var1, 0})).WillRepeatedly(Return(framed_expr{&f_var1, 0}));

    EXPECT_FALSE(run_unify(u, &var1, &f_var1, vars_touched));
    EXPECT_THAT(vars_touched, IsEmpty());
}

TEST_F(UnifierTest, UnifyVarToDepth2FunctorContainingSameVarFails) {
    EXPECT_CALL(bm, bind).Times(0);
    EXPECT_CALL(bm, whnf(framed_expr{&var0, 0})).WillRepeatedly(Return(framed_expr{&var0, 0}));
    EXPECT_CALL(bm, whnf(framed_expr{&f_f_var0, 0})).WillRepeatedly(Return(framed_expr{&f_f_var0, 0}));
    EXPECT_CALL(bm, whnf(framed_expr{&f_var0, 0})).WillRepeatedly(Return(framed_expr{&f_var0, 0}));

    EXPECT_FALSE(run_unify(u, &var0, &f_f_var0, vars_touched));
    EXPECT_THAT(vars_touched, IsEmpty());
}

// ---------------------------------------------------------------------------
// Both sides are functors
// ---------------------------------------------------------------------------

TEST_F(UnifierTest, UnifyIdenticalNullaryFunctorsReturnsTrue) {
    EXPECT_CALL(bm, bind).Times(0);
    EXPECT_CALL(bm, whnf(framed_expr{&func, 0})).WillRepeatedly(Return(framed_expr{&func, 0}));

    EXPECT_TRUE(run_unify(u, &func, &func, vars_touched));
    EXPECT_THAT(vars_touched, IsEmpty());
}

TEST_F(UnifierTest, UnifyFunctorsWithDifferentNamesFails) {
    EXPECT_CALL(bm, bind).Times(0);
    EXPECT_CALL(bm, whnf(framed_expr{&func, 0})).WillRepeatedly(Return(framed_expr{&func, 0}));
    EXPECT_CALL(bm, whnf(framed_expr{&func2, 0})).WillRepeatedly(Return(framed_expr{&func2, 0}));

    EXPECT_FALSE(run_unify(u, &func, &func2, vars_touched));
}

TEST_F(UnifierTest, UnifyFunctorsWithDifferentArityFails) {
    EXPECT_CALL(bm, bind).Times(0);
    EXPECT_CALL(bm, whnf(framed_expr{&func, 0})).WillRepeatedly(Return(framed_expr{&func, 0}));
    EXPECT_CALL(bm, whnf(framed_expr{&f_var0, 0})).WillRepeatedly(Return(framed_expr{&f_var0, 0}));

    EXPECT_FALSE(run_unify(u, &func, &f_var0, vars_touched));
}

TEST_F(UnifierTest, UnifyFunctorsWithOneVarArgRecursivelyBindsArg) {
    EXPECT_CALL(bm, whnf(framed_expr{&f_var0, 0})).WillRepeatedly(Return(framed_expr{&f_var0, 0}));
    EXPECT_CALL(bm, whnf(framed_expr{&f_func, 0})).WillRepeatedly(Return(framed_expr{&f_func, 0}));
    EXPECT_CALL(bm, whnf(framed_expr{&var0, 0})).WillRepeatedly(Return(framed_expr{&var0, 0}));
    EXPECT_CALL(bm, whnf(framed_expr{&func, 0})).WillRepeatedly(Return(framed_expr{&func, 0}));
    EXPECT_CALL(bm, bind(0u, framed_expr{&func, 0}));

    EXPECT_TRUE(run_unify(u, &f_var0, &f_func, vars_touched));
    EXPECT_THAT(vars_touched, UnorderedElementsAre(0u));
}

TEST_F(UnifierTest, UnifyFunctorsWithOneArgFailsIfArgsDiffer) {
    expr f_of_func2{expr::functor{functors.id("f"), {&func2}}};

    EXPECT_CALL(bm, bind).Times(0);
    EXPECT_CALL(bm, whnf(framed_expr{&f_func, 0})).WillRepeatedly(Return(framed_expr{&f_func, 0}));
    EXPECT_CALL(bm, whnf(framed_expr{&f_of_func2, 0})).WillRepeatedly(Return(framed_expr{&f_of_func2, 0}));
    EXPECT_CALL(bm, whnf(framed_expr{&func, 0})).WillRepeatedly(Return(framed_expr{&func, 0}));
    EXPECT_CALL(bm, whnf(framed_expr{&func2, 0})).WillRepeatedly(Return(framed_expr{&func2, 0}));

    EXPECT_FALSE(run_unify(u, &f_func, &f_of_func2, vars_touched));
}

TEST_F(UnifierTest, UnifyBinaryFunctorsWithTwoVarArgsBindsBothAndPopulatesSnk) {
    EXPECT_CALL(bm, whnf(framed_expr{&f2_var0_var1, 0})).WillRepeatedly(Return(framed_expr{&f2_var0_var1, 0}));
    EXPECT_CALL(bm, whnf(framed_expr{&f2_func_func2, 0})).WillRepeatedly(Return(framed_expr{&f2_func_func2, 0}));
    EXPECT_CALL(bm, whnf(framed_expr{&var0, 0})).WillRepeatedly(Return(framed_expr{&var0, 0}));
    EXPECT_CALL(bm, whnf(framed_expr{&var1, 0})).WillRepeatedly(Return(framed_expr{&var1, 0}));
    EXPECT_CALL(bm, whnf(framed_expr{&func, 0})).WillRepeatedly(Return(framed_expr{&func, 0}));
    EXPECT_CALL(bm, whnf(framed_expr{&func2, 0})).WillRepeatedly(Return(framed_expr{&func2, 0}));
    EXPECT_CALL(bm, bind(0u, framed_expr{&func, 0}));
    EXPECT_CALL(bm, bind(1u, framed_expr{&func2, 0}));

    EXPECT_TRUE(run_unify(u, &f2_var0_var1, &f2_func_func2, vars_touched));
    EXPECT_THAT(vars_touched, UnorderedElementsAre(0u, 1u));
}

TEST_F(UnifierTest, UnifyBinaryFunctorsWithMixedArgsOnlyBindsVarArg) {
    EXPECT_CALL(bm, whnf(framed_expr{&f2_func_var1, 0})).WillRepeatedly(Return(framed_expr{&f2_func_var1, 0}));
    EXPECT_CALL(bm, whnf(framed_expr{&f2_func_func2, 0})).WillRepeatedly(Return(framed_expr{&f2_func_func2, 0}));
    EXPECT_CALL(bm, whnf(framed_expr{&func, 0})).WillRepeatedly(Return(framed_expr{&func, 0}));
    EXPECT_CALL(bm, whnf(framed_expr{&var1, 0})).WillRepeatedly(Return(framed_expr{&var1, 0}));
    EXPECT_CALL(bm, whnf(framed_expr{&func2, 0})).WillRepeatedly(Return(framed_expr{&func2, 0}));
    EXPECT_CALL(bm, bind(1u, framed_expr{&func2, 0}));

    EXPECT_TRUE(run_unify(u, &f2_func_var1, &f2_func_func2, vars_touched));
    EXPECT_THAT(vars_touched, UnorderedElementsAre(1u));
}

TEST_F(UnifierTest, UnifyBinaryFunctorsFirstArgFailsSecondNeverAttempted) {
    expr rhs{expr::functor{functors.id("f"), {&func2, &func2}}};

    EXPECT_CALL(bm, bind).Times(0);
    EXPECT_CALL(bm, whnf(framed_expr{&f2_func_var1, 0})).WillRepeatedly(Return(framed_expr{&f2_func_var1, 0}));
    EXPECT_CALL(bm, whnf(framed_expr{&rhs, 0})).WillRepeatedly(Return(framed_expr{&rhs, 0}));
    EXPECT_CALL(bm, whnf(framed_expr{&func, 0})).WillRepeatedly(Return(framed_expr{&func, 0}));
    EXPECT_CALL(bm, whnf(framed_expr{&func2, 0})).WillRepeatedly(Return(framed_expr{&func2, 0}));

    EXPECT_FALSE(run_unify(u, &f2_func_var1, &rhs, vars_touched));
    EXPECT_THAT(vars_touched, IsEmpty());
}

TEST_F(UnifierTest, UnifyBinaryFunctorsSecondArgFailsAfterFirstBinds) {
    expr lhs{expr::functor{functors.id("f"), {&var0, &func}}};

    EXPECT_CALL(bm, whnf(framed_expr{&lhs, 0})).WillRepeatedly(Return(framed_expr{&lhs, 0}));
    EXPECT_CALL(bm, whnf(framed_expr{&f2_func_func2, 0})).WillRepeatedly(Return(framed_expr{&f2_func_func2, 0}));
    EXPECT_CALL(bm, whnf(framed_expr{&var0, 0})).WillRepeatedly(Return(framed_expr{&var0, 0}));
    EXPECT_CALL(bm, whnf(framed_expr{&func, 0})).WillRepeatedly(Return(framed_expr{&func, 0}));
    EXPECT_CALL(bm, whnf(framed_expr{&func2, 0})).WillRepeatedly(Return(framed_expr{&func2, 0}));
    EXPECT_CALL(bm, bind(0u, framed_expr{&func, 0}));

    EXPECT_FALSE(run_unify(u, &lhs, &f2_func_func2, vars_touched));
    EXPECT_THAT(vars_touched, UnorderedElementsAre(0u));
}

TEST_F(UnifierTest, UnifyTernaryFunctorsWithThreeVarArgsBindsAllThree) {
    EXPECT_CALL(bm, whnf(framed_expr{&f3_var0_var1_var2, 0})).WillRepeatedly(Return(framed_expr{&f3_var0_var1_var2, 0}));
    EXPECT_CALL(bm, whnf(framed_expr{&f3_func_func2_func, 0})).WillRepeatedly(Return(framed_expr{&f3_func_func2_func, 0}));
    EXPECT_CALL(bm, whnf(framed_expr{&var0, 0})).WillRepeatedly(Return(framed_expr{&var0, 0}));
    EXPECT_CALL(bm, whnf(framed_expr{&var1, 0})).WillRepeatedly(Return(framed_expr{&var1, 0}));
    EXPECT_CALL(bm, whnf(framed_expr{&var2, 0})).WillRepeatedly(Return(framed_expr{&var2, 0}));
    EXPECT_CALL(bm, whnf(framed_expr{&func, 0})).WillRepeatedly(Return(framed_expr{&func, 0}));
    EXPECT_CALL(bm, whnf(framed_expr{&func2, 0})).WillRepeatedly(Return(framed_expr{&func2, 0}));
    EXPECT_CALL(bm, bind(0u, framed_expr{&func, 0}));
    EXPECT_CALL(bm, bind(1u, framed_expr{&func2, 0}));
    EXPECT_CALL(bm, bind(2u, framed_expr{&func, 0}));

    EXPECT_TRUE(run_unify(u, &f3_var0_var1_var2, &f3_func_func2_func, vars_touched));
    EXPECT_THAT(vars_touched, UnorderedElementsAre(0u, 1u, 2u));
}

TEST_F(UnifierTest, UnifyTernaryFunctorsThirdArgFailsAfterFirstTwoBind) {
    expr lhs{expr::functor{functors.id("f"), {&var0, &var1, &func2}}};

    EXPECT_CALL(bm, whnf(framed_expr{&lhs, 0})).WillRepeatedly(Return(framed_expr{&lhs, 0}));
    EXPECT_CALL(bm, whnf(framed_expr{&f3_func_func2_func, 0})).WillRepeatedly(Return(framed_expr{&f3_func_func2_func, 0}));
    EXPECT_CALL(bm, whnf(framed_expr{&var0, 0})).WillRepeatedly(Return(framed_expr{&var0, 0}));
    EXPECT_CALL(bm, whnf(framed_expr{&var1, 0})).WillRepeatedly(Return(framed_expr{&var1, 0}));
    EXPECT_CALL(bm, whnf(framed_expr{&func, 0})).WillRepeatedly(Return(framed_expr{&func, 0}));
    EXPECT_CALL(bm, whnf(framed_expr{&func2, 0})).WillRepeatedly(Return(framed_expr{&func2, 0}));
    EXPECT_CALL(bm, bind(0u, framed_expr{&func, 0}));
    EXPECT_CALL(bm, bind(1u, framed_expr{&func2, 0}));

    EXPECT_FALSE(run_unify(u, &lhs, &f3_func_func2_func, vars_touched));
    EXPECT_THAT(vars_touched, UnorderedElementsAre(0u, 1u));
}

TEST_F(UnifierTest, UnifyDepth2FunctorsRecursivelyBindsInnerVar) {
    EXPECT_CALL(bm, whnf(framed_expr{&f_f_var0, 0})).WillRepeatedly(Return(framed_expr{&f_f_var0, 0}));
    EXPECT_CALL(bm, whnf(framed_expr{&f_f_func, 0})).WillRepeatedly(Return(framed_expr{&f_f_func, 0}));
    EXPECT_CALL(bm, whnf(framed_expr{&f_var0, 0})).WillRepeatedly(Return(framed_expr{&f_var0, 0}));
    EXPECT_CALL(bm, whnf(framed_expr{&f_func, 0})).WillRepeatedly(Return(framed_expr{&f_func, 0}));
    EXPECT_CALL(bm, whnf(framed_expr{&var0, 0})).WillRepeatedly(Return(framed_expr{&var0, 0}));
    EXPECT_CALL(bm, whnf(framed_expr{&func, 0})).WillRepeatedly(Return(framed_expr{&func, 0}));
    EXPECT_CALL(bm, bind(0u, framed_expr{&func, 0}));

    EXPECT_TRUE(run_unify(u, &f_f_var0, &f_f_func, vars_touched));
    EXPECT_THAT(vars_touched, UnorderedElementsAre(0u));
}

TEST_F(UnifierTest, UnifyDepth2FunctorsFailsOnInnerNameMismatch) {
    expr rhs{expr::functor{functors.id("f"), {&g_var0}}};

    EXPECT_CALL(bm, bind).Times(0);
    EXPECT_CALL(bm, whnf(framed_expr{&f_f_var0, 0})).WillRepeatedly(Return(framed_expr{&f_f_var0, 0}));
    EXPECT_CALL(bm, whnf(framed_expr{&rhs, 0})).WillRepeatedly(Return(framed_expr{&rhs, 0}));
    EXPECT_CALL(bm, whnf(framed_expr{&f_var0, 0})).WillRepeatedly(Return(framed_expr{&f_var0, 0}));
    EXPECT_CALL(bm, whnf(framed_expr{&g_var0, 0})).WillRepeatedly(Return(framed_expr{&g_var0, 0}));

    EXPECT_FALSE(run_unify(u, &f_f_var0, &rhs, vars_touched));
}

// ---------------------------------------------------------------------------
// whnf is called before branching — vars resolve through equivalence classes
// ---------------------------------------------------------------------------

TEST_F(UnifierTest, UnifyAfterWhnfBothResolveToSameFunctorReturnsTrue) {
    EXPECT_CALL(bm, bind).Times(0);
    EXPECT_CALL(bm, whnf(framed_expr{&var0, 0})).WillRepeatedly(Return(framed_expr{&func, 0}));
    EXPECT_CALL(bm, whnf(framed_expr{&var1, 0})).WillRepeatedly(Return(framed_expr{&func, 0}));

    EXPECT_TRUE(run_unify(u, &var0, &var1, vars_touched));
    EXPECT_THAT(vars_touched, IsEmpty());
}

TEST_F(UnifierTest, UnifyAfterWhnfBothResolveToDifferentFunctorsFails) {
    EXPECT_CALL(bm, bind).Times(0);
    EXPECT_CALL(bm, whnf(framed_expr{&var0, 0})).WillRepeatedly(Return(framed_expr{&func, 0}));
    EXPECT_CALL(bm, whnf(framed_expr{&var1, 0})).WillRepeatedly(Return(framed_expr{&func2, 0}));

    EXPECT_FALSE(run_unify(u, &var0, &var1, vars_touched));
}

TEST_F(UnifierTest, UnifyAfterWhnfBothResolveToVarsBindsRepresentatives) {
    EXPECT_CALL(bm, whnf(framed_expr{&var0, 0})).WillRepeatedly(Return(framed_expr{&var2, 0}));
    EXPECT_CALL(bm, whnf(framed_expr{&var1, 0})).WillRepeatedly(Return(framed_expr{&var1, 0}));
    EXPECT_CALL(bm, bind(2u, framed_expr{&var1, 0}));

    EXPECT_TRUE(run_unify(u, &var0, &var1, vars_touched));
    EXPECT_THAT(vars_touched, UnorderedElementsAre(1u, 2u));
}

TEST_F(UnifierTest, UnifyAfterWhnfVarResolvesToVarFunctorBindsRepresentative) {
    EXPECT_CALL(bm, whnf(framed_expr{&var0, 0})).WillRepeatedly(Return(framed_expr{&var2, 0}));
    EXPECT_CALL(bm, whnf(framed_expr{&var1, 0})).WillRepeatedly(Return(framed_expr{&func, 0}));
    EXPECT_CALL(bm, whnf(framed_expr{&func, 0})).WillRepeatedly(Return(framed_expr{&func, 0}));
    EXPECT_CALL(bm, bind(2u, framed_expr{&func, 0}));

    EXPECT_TRUE(run_unify(u, &var0, &var1, vars_touched));
    EXPECT_THAT(vars_touched, UnorderedElementsAre(2u));
}

TEST_F(UnifierTest, UnifyAfterWhnfBothResolveToSameVarReturnsTrueNoBind) {
    EXPECT_CALL(bm, bind).Times(0);
    EXPECT_CALL(bm, whnf(framed_expr{&var0, 0})).WillRepeatedly(Return(framed_expr{&var2, 0}));
    EXPECT_CALL(bm, whnf(framed_expr{&var1, 0})).WillRepeatedly(Return(framed_expr{&var2, 0}));

    EXPECT_TRUE(run_unify(u, &var0, &var1, vars_touched));
    EXPECT_THAT(vars_touched, IsEmpty());
}

// ---------------------------------------------------------------------------
// Occurs-check failure only visible after whnf on nested argument vars
// ---------------------------------------------------------------------------

TEST_F(UnifierTest, UnifyVarToFunctorFailsWhenWhnfRevealsArgAliasesVar) {
    EXPECT_CALL(bm, bind).Times(0);
    EXPECT_CALL(bm, whnf(framed_expr{&var0, 0})).WillRepeatedly(Return(framed_expr{&var0, 0}));
    EXPECT_CALL(bm, whnf(framed_expr{&f_var1, 0})).WillRepeatedly(Return(framed_expr{&f_var1, 0}));
    EXPECT_CALL(bm, whnf(framed_expr{&var1, 0})).WillRepeatedly(Return(framed_expr{&var0, 0}));

    EXPECT_FALSE(run_unify(u, &var0, &f_var1, vars_touched));
    EXPECT_THAT(vars_touched, IsEmpty());
}

TEST_F(UnifierTest, UnifyVarToDepth2FunctorFailsWhenWhnfRevealsInnerVarAlias) {
    EXPECT_CALL(bm, bind).Times(0);
    EXPECT_CALL(bm, whnf(framed_expr{&var0, 0})).WillRepeatedly(Return(framed_expr{&var0, 0}));
    EXPECT_CALL(bm, whnf(framed_expr{&f_f_var1, 0})).WillRepeatedly(Return(framed_expr{&f_f_var1, 0}));
    EXPECT_CALL(bm, whnf(framed_expr{&f_var1, 0})).WillRepeatedly(Return(framed_expr{&f_var1, 0}));
    EXPECT_CALL(bm, whnf(framed_expr{&var1, 0})).WillRepeatedly(Return(framed_expr{&var0, 0}));

    EXPECT_FALSE(run_unify(u, &var0, &f_f_var1, vars_touched));
    EXPECT_THAT(vars_touched, IsEmpty());
}

// ---------------------------------------------------------------------------
// Functor recursive unify — short-circuit and whnf-resolved args
// ---------------------------------------------------------------------------

TEST_F(UnifierTest, UnifyTernaryFunctorsFirstArgFailsSecondThirdNeverAttempted) {
    expr lhs{expr::functor{functors.id("f"), {&func, &var1, &var2}}};
    expr rhs{expr::functor{functors.id("f"), {&func2, &func, &func}}};

    EXPECT_CALL(bm, bind).Times(0);
    EXPECT_CALL(bm, whnf(framed_expr{&lhs, 0})).WillRepeatedly(Return(framed_expr{&lhs, 0}));
    EXPECT_CALL(bm, whnf(framed_expr{&rhs, 0})).WillRepeatedly(Return(framed_expr{&rhs, 0}));
    EXPECT_CALL(bm, whnf(framed_expr{&func, 0})).WillRepeatedly(Return(framed_expr{&func, 0}));
    EXPECT_CALL(bm, whnf(framed_expr{&func2, 0})).WillRepeatedly(Return(framed_expr{&func2, 0}));

    EXPECT_FALSE(run_unify(u, &lhs, &rhs, vars_touched));
    EXPECT_THAT(vars_touched, IsEmpty());
}

TEST_F(UnifierTest, UnifyBinaryFunctorsWhnfResolvesFirstArgSkipsItsBind) {
    EXPECT_CALL(bm, whnf(framed_expr{&f2_var0_var1, 0})).WillRepeatedly(Return(framed_expr{&f2_var0_var1, 0}));
    EXPECT_CALL(bm, whnf(framed_expr{&f2_func_func2, 0})).WillRepeatedly(Return(framed_expr{&f2_func_func2, 0}));
    EXPECT_CALL(bm, whnf(framed_expr{&var0, 0})).WillRepeatedly(Return(framed_expr{&func, 0}));
    EXPECT_CALL(bm, whnf(framed_expr{&var1, 0})).WillRepeatedly(Return(framed_expr{&var1, 0}));
    EXPECT_CALL(bm, whnf(framed_expr{&func, 0})).WillRepeatedly(Return(framed_expr{&func, 0}));
    EXPECT_CALL(bm, whnf(framed_expr{&func2, 0})).WillRepeatedly(Return(framed_expr{&func2, 0}));
    EXPECT_CALL(bm, bind(1u, framed_expr{&func2, 0}));

    EXPECT_TRUE(run_unify(u, &f2_var0_var1, &f2_func_func2, vars_touched));
    EXPECT_THAT(vars_touched, UnorderedElementsAre(1u));
}

TEST_F(UnifierTest, UnifyTernaryFunctorsSecondArgFailsThirdNeverAttempted) {
    expr lhs{expr::functor{functors.id("f"), {&func, &func, &var2}}};
    expr rhs{expr::functor{functors.id("f"), {&func, &func2, &var2}}};

    EXPECT_CALL(bm, bind).Times(0);
    EXPECT_CALL(bm, whnf(framed_expr{&lhs, 0})).WillRepeatedly(Return(framed_expr{&lhs, 0}));
    EXPECT_CALL(bm, whnf(framed_expr{&rhs, 0})).WillRepeatedly(Return(framed_expr{&rhs, 0}));
    EXPECT_CALL(bm, whnf(framed_expr{&func, 0})).WillRepeatedly(Return(framed_expr{&func, 0}));
    EXPECT_CALL(bm, whnf(framed_expr{&func2, 0})).WillRepeatedly(Return(framed_expr{&func2, 0}));
    EXPECT_CALL(bm, whnf(framed_expr{&var2, 0})).WillRepeatedly(Return(framed_expr{&var2, 0}));

    EXPECT_FALSE(run_unify(u, &lhs, &rhs, vars_touched));
    EXPECT_THAT(vars_touched, IsEmpty());
}

TEST_F(UnifierTest, UnifyBinaryFunctorsWhnfResolvesBothArgsSkipsAllBinds) {
    EXPECT_CALL(bm, bind).Times(0);
    EXPECT_CALL(bm, whnf(framed_expr{&f2_var0_var1, 0})).WillRepeatedly(Return(framed_expr{&f2_var0_var1, 0}));
    EXPECT_CALL(bm, whnf(framed_expr{&f2_func_func2, 0})).WillRepeatedly(Return(framed_expr{&f2_func_func2, 0}));
    EXPECT_CALL(bm, whnf(framed_expr{&var0, 0})).WillRepeatedly(Return(framed_expr{&func, 0}));
    EXPECT_CALL(bm, whnf(framed_expr{&var1, 0})).WillRepeatedly(Return(framed_expr{&func2, 0}));
    EXPECT_CALL(bm, whnf(framed_expr{&func, 0})).WillRepeatedly(Return(framed_expr{&func, 0}));
    EXPECT_CALL(bm, whnf(framed_expr{&func2, 0})).WillRepeatedly(Return(framed_expr{&func2, 0}));

    EXPECT_TRUE(run_unify(u, &f2_var0_var1, &f2_func_func2, vars_touched));
    EXPECT_THAT(vars_touched, IsEmpty());
}

// ---------------------------------------------------------------------------
// Entry whnf changes branch shape; symmetric occurs and inner recursive unify
// ---------------------------------------------------------------------------

TEST_F(UnifierTest, UnifyAfterWhnfLhsFunctorRhsVarBindsYoungerVar) {
    EXPECT_CALL(bm, whnf(framed_expr{&var0, 0})).WillRepeatedly(Return(framed_expr{&func, 0}));
    EXPECT_CALL(bm, whnf(framed_expr{&var1, 0})).WillRepeatedly(Return(framed_expr{&var1, 0}));
    EXPECT_CALL(bm, whnf(framed_expr{&func, 0})).WillRepeatedly(Return(framed_expr{&func, 0}));
    EXPECT_CALL(bm, bind(1u, framed_expr{&func, 0}));

    EXPECT_TRUE(run_unify(u, &var0, &var1, vars_touched));
    EXPECT_THAT(vars_touched, UnorderedElementsAre(1u));
}

TEST_F(UnifierTest, UnifyFunctorLhsVarRhsFailsWhenWhnfRevealsArgAliasesVar) {
    EXPECT_CALL(bm, bind).Times(0);
    EXPECT_CALL(bm, whnf(framed_expr{&f_var1, 0})).WillRepeatedly(Return(framed_expr{&f_var1, 0}));
    EXPECT_CALL(bm, whnf(framed_expr{&var0, 0})).WillRepeatedly(Return(framed_expr{&var0, 0}));
    EXPECT_CALL(bm, whnf(framed_expr{&var1, 0})).WillRepeatedly(Return(framed_expr{&var0, 0}));

    EXPECT_FALSE(run_unify(u, &f_var1, &var0, vars_touched));
    EXPECT_THAT(vars_touched, IsEmpty());
}

TEST_F(UnifierTest, UnifyFunctorArgsWhnfBothResolveToSameVarNoBind) {
    EXPECT_CALL(bm, bind).Times(0);
    EXPECT_CALL(bm, whnf(framed_expr{&f_var0, 0})).WillRepeatedly(Return(framed_expr{&f_var0, 0}));
    EXPECT_CALL(bm, whnf(framed_expr{&f_var1, 0})).WillRepeatedly(Return(framed_expr{&f_var1, 0}));
    EXPECT_CALL(bm, whnf(framed_expr{&var0, 0})).WillRepeatedly(Return(framed_expr{&var2, 0}));
    EXPECT_CALL(bm, whnf(framed_expr{&var1, 0})).WillRepeatedly(Return(framed_expr{&var2, 0}));

    EXPECT_TRUE(run_unify(u, &f_var0, &f_var1, vars_touched));
    EXPECT_THAT(vars_touched, IsEmpty());
}

TEST_F(UnifierTest, UnifyBinaryFunctorsWhnfResolvesSecondArgSkipsItsBind) {
    EXPECT_CALL(bm, whnf(framed_expr{&f2_var0_var1, 0})).WillRepeatedly(Return(framed_expr{&f2_var0_var1, 0}));
    EXPECT_CALL(bm, whnf(framed_expr{&f2_func_func2, 0})).WillRepeatedly(Return(framed_expr{&f2_func_func2, 0}));
    EXPECT_CALL(bm, whnf(framed_expr{&var0, 0})).WillRepeatedly(Return(framed_expr{&var0, 0}));
    EXPECT_CALL(bm, whnf(framed_expr{&var1, 0})).WillRepeatedly(Return(framed_expr{&func2, 0}));
    EXPECT_CALL(bm, whnf(framed_expr{&func, 0})).WillRepeatedly(Return(framed_expr{&func, 0}));
    EXPECT_CALL(bm, whnf(framed_expr{&func2, 0})).WillRepeatedly(Return(framed_expr{&func2, 0}));
    EXPECT_CALL(bm, bind(0u, framed_expr{&func, 0}));

    EXPECT_TRUE(run_unify(u, &f2_var0_var1, &f2_func_func2, vars_touched));
    EXPECT_THAT(vars_touched, UnorderedElementsAre(0u));
}

// ---------------------------------------------------------------------------
// Optional polish — partial ternary vars_touched, occurs WHNF on whole other_e, depth-2 WHNF
// ---------------------------------------------------------------------------

TEST_F(UnifierTest, UnifyTernaryFunctorsSecondArgFailsAfterFirstBinds) {
    expr lhs{expr::functor{functors.id("f"), {&var0, &func, &var2}}};
    expr rhs{expr::functor{functors.id("f"), {&func, &func2, &var2}}};

    EXPECT_CALL(bm, whnf(framed_expr{&lhs, 0})).WillRepeatedly(Return(framed_expr{&lhs, 0}));
    EXPECT_CALL(bm, whnf(framed_expr{&rhs, 0})).WillRepeatedly(Return(framed_expr{&rhs, 0}));
    EXPECT_CALL(bm, whnf(framed_expr{&var0, 0})).WillRepeatedly(Return(framed_expr{&var0, 0}));
    EXPECT_CALL(bm, whnf(framed_expr{&func, 0})).WillRepeatedly(Return(framed_expr{&func, 0}));
    EXPECT_CALL(bm, whnf(framed_expr{&func2, 0})).WillRepeatedly(Return(framed_expr{&func2, 0}));
    EXPECT_CALL(bm, whnf(framed_expr{&var2, 0})).WillRepeatedly(Return(framed_expr{&var2, 0}));
    EXPECT_CALL(bm, bind(0u, framed_expr{&func, 0}));

    EXPECT_FALSE(run_unify(u, &lhs, &rhs, vars_touched));
    EXPECT_THAT(vars_touched, UnorderedElementsAre(0u));
}

TEST_F(UnifierTest, UnifyVarToFunctorFailsWhenWhnfCollapsesOtherToSameVar) {
    EXPECT_CALL(bm, bind).Times(0);
    EXPECT_CALL(bm, whnf(framed_expr{&var0, 0})).WillRepeatedly(Return(framed_expr{&var0, 0}));
    EXPECT_CALL(bm, whnf(framed_expr{&f_var1, 0}))
        .WillOnce(Return(framed_expr{&f_var1, 0}))
        .WillRepeatedly(Return(framed_expr{&var0, 0}));

    EXPECT_FALSE(run_unify(u, &var0, &f_var1, vars_touched));
    EXPECT_THAT(vars_touched, IsEmpty());
}

TEST_F(UnifierTest, UnifyDepth2FunctorsInnerArgWhnfSkipsBind) {
    EXPECT_CALL(bm, bind).Times(0);
    EXPECT_CALL(bm, whnf(framed_expr{&f_f_var0, 0})).WillRepeatedly(Return(framed_expr{&f_f_var0, 0}));
    EXPECT_CALL(bm, whnf(framed_expr{&f_f_func, 0})).WillRepeatedly(Return(framed_expr{&f_f_func, 0}));
    EXPECT_CALL(bm, whnf(framed_expr{&f_var0, 0})).WillRepeatedly(Return(framed_expr{&f_var0, 0}));
    EXPECT_CALL(bm, whnf(framed_expr{&f_func, 0})).WillRepeatedly(Return(framed_expr{&f_func, 0}));
    EXPECT_CALL(bm, whnf(framed_expr{&var0, 0})).WillRepeatedly(Return(framed_expr{&func, 0}));
    EXPECT_CALL(bm, whnf(framed_expr{&func, 0})).WillRepeatedly(Return(framed_expr{&func, 0}));

    EXPECT_TRUE(run_unify(u, &f_f_var0, &f_f_func, vars_touched));
    EXPECT_THAT(vars_touched, IsEmpty());
}

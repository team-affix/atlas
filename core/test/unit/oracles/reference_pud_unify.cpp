#include <gtest/gtest.h>
#include "functor_fixture.hpp"
#include "pud_term_stack.hpp"
#include "reference_pud_unify.hpp"

// The reference unification other tests check the real unifier against, checked on hand-worked
// cases over the real bind map. A local variable at frame offset k is global variable k + index.
struct ReferencePudUnifyTest : public ::testing::Test {
    test_functors functors;
    pud_term_stack stack;

    const expr* var(uint32_t index) { return stack.pool.make_var(index); }
    const expr* atom(const char* name) { return stack.pool.make_functor(functors.id(name), {}); }
    const expr* functor(const char* name, std::vector<const expr*> args) {
        return stack.pool.make_functor(functors.id(name), std::move(args));
    }
};

TEST_F(ReferencePudUnifyTest, AVariableOccursInItself) {
    EXPECT_TRUE(reference_pud_occurs(2, var(2)));
}

TEST_F(ReferencePudUnifyTest, AVariableOccursDeepInsideATerm) {
    EXPECT_TRUE(reference_pud_occurs(2, functor("f", {atom("a"), functor("g", {var(2)})})));
}

TEST_F(ReferencePudUnifyTest, AVariableDoesNotOccurInATermWithoutIt) {
    EXPECT_FALSE(reference_pud_occurs(2, functor("f", {var(1), functor("g", {var(3)})})));
    EXPECT_FALSE(reference_pud_occurs(2, atom("a")));
}

TEST_F(ReferencePudUnifyTest, AnUnboundVariableDerefsToItsRepresentative) {
    const framed_expr dereffed = reference_pud_deref(stack, 4);
    EXPECT_EQ(dereffed.skeleton, var(4));
    EXPECT_EQ(dereffed.frame_offset, 0u);
}

TEST_F(ReferencePudUnifyTest, ABoundVariableDerefsToItsValue) {
    stack.bind_map.instantiate(stack.bind_map.find(4), framed_functor{std::get_if<expr::functor>(&(atom("a"))->content), 0});
    EXPECT_EQ(reference_pud_deref(stack, 4).skeleton, atom("a"));
}

TEST_F(ReferencePudUnifyTest, AVariableUnifiesWithATermByBindingIt) {
    EXPECT_TRUE(reference_pud_unify_terms(
        stack, framed_expr{functor("f", {var(0)}), 0}, framed_expr{functor("f", {atom("a")}), 0}));
    EXPECT_EQ(stack.normalizer.normalize(framed_expr{var(0), 0}), atom("a"));
}

TEST_F(ReferencePudUnifyTest, ALocalVariableIsBoundAtItsGlobalIndex) {
    EXPECT_TRUE(reference_pud_unify_terms(stack, framed_expr{var(1), 5}, framed_expr{atom("a"), 0}));
    EXPECT_EQ(stack.normalizer.normalize(framed_expr{var(6), 0}), atom("a"));
    EXPECT_EQ(stack.normalizer.normalize(framed_expr{var(1), 0}), var(1));
}

TEST_F(ReferencePudUnifyTest, DifferentFunctorsDoNotUnify) {
    EXPECT_FALSE(reference_pud_unify_terms(
        stack, framed_expr{functor("f", {atom("a")}), 0}, framed_expr{functor("g", {atom("a")}), 0}));
}

TEST_F(ReferencePudUnifyTest, DifferentAritiesDoNotUnify) {
    EXPECT_FALSE(reference_pud_unify_terms(
        stack, framed_expr{functor("f", {atom("a")}), 0}, framed_expr{functor("f", {atom("a"), atom("a")}), 0}));
}

TEST_F(ReferencePudUnifyTest, AVariableDoesNotUnifyWithATermContainingIt) {
    EXPECT_FALSE(reference_pud_unify_terms(stack, framed_expr{var(0), 0}, framed_expr{functor("f", {var(0)}), 0}));
    EXPECT_FALSE(reference_pud_unify_terms(stack, framed_expr{functor("f", {var(0)}), 0}, framed_expr{var(0), 0}));
}

TEST_F(ReferencePudUnifyTest, TwoVariablesUnifyIntoOneClass) {
    EXPECT_TRUE(reference_pud_unify_terms(stack, framed_expr{var(0), 0}, framed_expr{var(1), 0}));
    EXPECT_TRUE(reference_pud_unify_terms(stack, framed_expr{var(1), 0}, framed_expr{atom("a"), 0}));
    EXPECT_EQ(stack.normalizer.normalize(framed_expr{var(0), 0}), atom("a"));
}

TEST_F(ReferencePudUnifyTest, AVariableUnifiesWithItselfWithoutBinding) {
    EXPECT_TRUE(reference_pud_unify_terms(stack, framed_expr{var(3), 0}, framed_expr{var(3), 0}));
    EXPECT_FALSE(stack.bind_map.find(3).value.has_value());
}

TEST_F(ReferencePudUnifyTest, ABoundVariableUnifiesThroughItsValue) {
    EXPECT_TRUE(reference_pud_unify_terms(stack, framed_expr{var(0), 0}, framed_expr{atom("a"), 0}));
    EXPECT_TRUE(reference_pud_unify_terms(stack, framed_expr{var(0), 0}, framed_expr{atom("a"), 0}));
    EXPECT_FALSE(reference_pud_unify_terms(stack, framed_expr{var(0), 0}, framed_expr{atom("b"), 0}));
}

TEST_F(ReferencePudUnifyTest, ASharedVariableForcesBothArgumentsToAgree) {
    EXPECT_FALSE(reference_pud_unify_terms(
        stack,
        framed_expr{functor("f", {var(0), var(0)}), 0},
        framed_expr{functor("f", {atom("a"), atom("b")}), 0}));
}

#include <algorithm>
#include <gtest/gtest.h>
#include "functor_fixture.hpp"
#include "pud_term_stack.hpp"

struct PudNormalizerBindMapIntegrationTest : public ::testing::Test {
    test_functors functors;
    pud_term_stack stack;
};

TEST_F(PudNormalizerBindMapIntegrationTest, BoundFunctorNormalizesInnerVariables) {
    const expr* y = stack.pool.make_var(1u);
    stack.instantiate(1u, framed_expr{stack.pool.make_functor(functors.id("a"), {}), 0u});
    const expr* fx = stack.pool.make_functor(functors.id("wrap"), {y});
    const expr* expected = stack.pool.make_functor(
        functors.id("wrap"), {stack.pool.make_functor(functors.id("a"), {})});
    EXPECT_EQ(stack.normalizer.normalize(pud_fe(fx, 0)), expected);
}

TEST_F(PudNormalizerBindMapIntegrationTest, NormalizePerformsNoBindMapWrites) {
    const expr* term = stack.pool.make_functor(
        functors.id("pair"), {stack.pool.make_var(0u), stack.pool.make_var(1u)});
    const pud_ubs_state before = stack.snapshot();
    stack.normalizer.normalize(pud_fe(term, 0));
    EXPECT_TRUE(before == stack.snapshot());
}

TEST_F(PudNormalizerBindMapIntegrationTest, NormalizeAfterEquateUsesNewRepresentative) {
    stack.equate(2u, 5u);
    const expr* local = stack.pool.make_var(5u);
    const expr* rep = stack.resolve(2u);
    EXPECT_EQ(stack.normalizer.normalize(pud_fe(local, 0)), rep);
}

TEST_F(PudNormalizerBindMapIntegrationTest, CalleeEquatedWithCallerNormalizesToCallerIndex) {
    const uint32_t caller = stack.globalize.globalize(3u, 0u);
    const uint32_t callee = stack.globalize.globalize(0u, 2u);
    stack.equate(caller, callee);
    const expr* callee_local = stack.pool.make_var(2u);
    const expr* normalized = stack.normalizer.normalize(pud_fe(callee_local, 0));
    const expr* representative = stack.resolve(caller);
    EXPECT_EQ(normalized, representative);
    EXPECT_EQ(std::get<expr::var>(normalized->content).index, std::min(caller, callee));
}

TEST_F(PudNormalizerBindMapIntegrationTest, BindingAfterTermBuiltIsVisibleOnNormalize) {
    const expr* y = stack.pool.make_var(1u);
    const expr* fx = stack.pool.make_functor(functors.id("wrap"), {y});
    const expr* before = stack.normalizer.normalize(pud_fe(fx, 0));
    stack.instantiate(1u, framed_expr{stack.pool.make_functor(functors.id("a"), {}), 0u});
    const expr* after = stack.normalizer.normalize(pud_fe(fx, 0));
    EXPECT_NE(before, after);
    const expr* expected = stack.pool.make_functor(
        functors.id("wrap"), {stack.pool.make_functor(functors.id("a"), {})});
    EXPECT_EQ(after, expected);
}

TEST_F(PudNormalizerBindMapIntegrationTest, UnrelatedBindingDoesNotChangeFramedTermNormalize) {
    const expr* term = stack.pool.make_functor(functors.id("f"), {stack.pool.make_var(0u)});
    const expr* first = stack.normalizer.normalize(pud_fe(term, 0));
    stack.instantiate(9u, framed_expr{stack.pool.make_functor(functors.id("z"), {}), 0u});
    const expr* second = stack.normalizer.normalize(pud_fe(term, 0));
    EXPECT_EQ(first, second);
}

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
    stack.bind_map.instantiate(stack.bind_map.find(1u), framed_functor_from_framed_expr(framed_expr{stack.pool.make_functor(functors.id("a"), {}), 0u}));
    const expr* fx = stack.pool.make_functor(functors.id("wrap"), {y});
    const expr* expected = stack.pool.make_functor(
        functors.id("wrap"), {stack.pool.make_functor(functors.id("a"), {})});
    EXPECT_EQ(stack.normalizer.normalize(framed_expr{fx, 0u}), expected);
}

TEST_F(PudNormalizerBindMapIntegrationTest, NormalizePerformsNoBindMapWrites) {
    const expr* term = stack.pool.make_functor(
        functors.id("pair"), {stack.pool.make_var(0u), stack.pool.make_var(1u)});
    pud_ubs_state before = std::move(stack.bind_map).persist();
    stack.bind_map = ubs_bind_map{before};
    stack.normalizer.normalize(framed_expr{term, 0u});
    pud_ubs_state after = std::move(stack.bind_map).persist();
    stack.bind_map = ubs_bind_map{after};
    EXPECT_TRUE(before == after);
}

TEST_F(PudNormalizerBindMapIntegrationTest, NormalizeAfterEquateUsesNewRepresentative) {
    stack.bind_map.equate(stack.bind_map.find(2u), stack.bind_map.find(5u));
    const expr* local = stack.pool.make_var(5u);
    const expr* rep = stack.normalizer.normalize(framed_expr{stack.pool.make_var(2u), 0u});
    EXPECT_EQ(stack.normalizer.normalize(framed_expr{local, 0u}), rep);
}

TEST_F(PudNormalizerBindMapIntegrationTest, CalleeEquatedWithCallerNormalizesToCallerIndex) {
    const uint32_t caller = stack.globalize.globalize(3u, 0u);
    const uint32_t callee = stack.globalize.globalize(0u, 2u);
    stack.bind_map.equate(stack.bind_map.find(caller), stack.bind_map.find(callee));
    const expr* callee_local = stack.pool.make_var(2u);
    const expr* normalized = stack.normalizer.normalize(framed_expr{callee_local, 0u});
    const expr* representative = stack.normalizer.normalize(
        framed_expr{stack.pool.make_var(caller), 0u});
    EXPECT_EQ(normalized, representative);
    EXPECT_EQ(std::get<expr::var>(normalized->content).index, std::min(caller, callee));
}

TEST_F(PudNormalizerBindMapIntegrationTest, BindingAfterTermBuiltIsVisibleOnNormalize) {
    const expr* y = stack.pool.make_var(1u);
    const expr* fx = stack.pool.make_functor(functors.id("wrap"), {y});
    const expr* before = stack.normalizer.normalize(framed_expr{fx, 0u});
    stack.bind_map.instantiate(stack.bind_map.find(1u), framed_functor_from_framed_expr(framed_expr{stack.pool.make_functor(functors.id("a"), {}), 0u}));
    const expr* after = stack.normalizer.normalize(framed_expr{fx, 0u});
    EXPECT_NE(before, after);
    const expr* expected = stack.pool.make_functor(
        functors.id("wrap"), {stack.pool.make_functor(functors.id("a"), {})});
    EXPECT_EQ(after, expected);
}

TEST_F(PudNormalizerBindMapIntegrationTest, UnrelatedBindingDoesNotChangeFramedTermNormalize) {
    const expr* term = stack.pool.make_functor(functors.id("f"), {stack.pool.make_var(0u)});
    const expr* first = stack.normalizer.normalize(framed_expr{term, 0u});
    stack.bind_map.instantiate(stack.bind_map.find(9u), framed_functor_from_framed_expr(framed_expr{stack.pool.make_functor(functors.id("z"), {}), 0u}));
    const expr* second = stack.normalizer.normalize(framed_expr{term, 0u});
    EXPECT_EQ(first, second);
}

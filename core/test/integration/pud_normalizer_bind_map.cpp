#include <algorithm>
#include <gtest/gtest.h>
#include "functor_fixture.hpp"
#include "pud_unifier_bind_map_stack.hpp"
#include "ubs_bind_map_test_maps.hpp"

struct PudNormalizerBindMapIntegrationTest : public ::testing::Test {
    test_functors functors;
    pud_unifier_bind_map_stack stack;
};

TEST_F(PudNormalizerBindMapIntegrationTest, BoundFunctorNormalizesInnerVariables) {
    const expr* y = stack.bind_backing.pool.make_var(1u);
    stack.bind_map.instantiate(1u, framed_expr{stack.bind_backing.pool.make_functor(functors.id("a"), {}), 0u});
    const expr* fx = stack.bind_backing.pool.make_functor(functors.id("wrap"), {y});
    const expr* expected = stack.bind_backing.pool.make_functor(
        functors.id("wrap"), {stack.bind_backing.pool.make_functor(functors.id("a"), {})});
    EXPECT_EQ(stack.normalizer.normalize(pud_fe(fx, 0)), expected);
}

TEST_F(PudNormalizerBindMapIntegrationTest, NormalizePerformsNoBindMapWrites) {
    const expr* term = stack.bind_backing.pool.make_functor(
        functors.id("pair"), {stack.bind_backing.pool.make_var(0u), stack.bind_backing.pool.make_var(1u)});
    const ubs_map_snapshot before = snapshot_bind_backing(stack.bind_backing);
    stack.normalizer.normalize(pud_fe(term, 0));
    EXPECT_TRUE(ubs_snapshots_equal(before, snapshot_bind_backing(stack.bind_backing)));
}

TEST_F(PudNormalizerBindMapIntegrationTest, NormalizeAfterEquateUsesNewRepresentative) {
    stack.bind_map.equate(2u, 5u);
    const expr* local = stack.bind_backing.pool.make_var(5u);
    const expr* rep = stack.resolve(2u);
    EXPECT_EQ(stack.normalizer.normalize(pud_fe(local, 0)), rep);
}

TEST_F(PudNormalizerBindMapIntegrationTest, CalleeEquatedWithCallerNormalizesToCallerIndex) {
    const uint32_t caller = stack.globalize.globalize(3u, 0u);
    const uint32_t callee = stack.globalize.globalize(0u, 2u);
    stack.bind_map.equate(caller, callee);
    const expr* callee_local = stack.bind_backing.pool.make_var(2u);
    const expr* normalized = stack.normalizer.normalize(pud_fe(callee_local, 0));
    const expr* representative = stack.resolve(caller);
    EXPECT_EQ(normalized, representative);
    EXPECT_EQ(std::get<expr::var>(normalized->content).index, std::min(caller, callee));
}

TEST_F(PudNormalizerBindMapIntegrationTest, BindingAfterTermBuiltIsVisibleOnNormalize) {
    const expr* y = stack.bind_backing.pool.make_var(1u);
    const expr* fx = stack.bind_backing.pool.make_functor(functors.id("wrap"), {y});
    const expr* before = stack.normalizer.normalize(pud_fe(fx, 0));
    stack.bind_map.instantiate(1u, framed_expr{stack.bind_backing.pool.make_functor(functors.id("a"), {}), 0u});
    const expr* after = stack.normalizer.normalize(pud_fe(fx, 0));
    EXPECT_NE(before, after);
    const expr* expected = stack.bind_backing.pool.make_functor(
        functors.id("wrap"), {stack.bind_backing.pool.make_functor(functors.id("a"), {})});
    EXPECT_EQ(after, expected);
}

TEST_F(PudNormalizerBindMapIntegrationTest, UnrelatedBindingDoesNotChangeFramedTermNormalize) {
    const expr* term = stack.bind_backing.pool.make_functor(functors.id("f"), {stack.bind_backing.pool.make_var(0u)});
    const expr* first = stack.normalizer.normalize(pud_fe(term, 0));
    stack.bind_map.instantiate(9u, framed_expr{stack.bind_backing.pool.make_functor(functors.id("z"), {}), 0u});
    const expr* second = stack.normalizer.normalize(pud_fe(term, 0));
    EXPECT_EQ(first, second);
}

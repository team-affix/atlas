#include <gtest/gtest.h>
#include "functor_fixture.hpp"
#include "pud_unifier_bind_map_stack.hpp"

struct PudNormalizerBindMapIntegrationTest : public ::testing::Test {
    test_functors functors;
    pud_unifier_bind_map_stack stack;
};

TEST_F(PudNormalizerBindMapIntegrationTest, BoundFunctorNormalizesInnerVariables) {
    const expr* y = stack.bind_backing.pool.make_var(1u);
    stack.bind_map.instantiate(1u, stack.bind_backing.pool.make_functor(functors.id("a"), {}));
    const expr* fx = stack.bind_backing.pool.make_functor(functors.id("wrap"), {y});
    const expr* expected = stack.bind_backing.pool.make_functor(
        functors.id("wrap"), {stack.bind_backing.pool.make_functor(functors.id("a"), {})});
    EXPECT_EQ(stack.normalizer.normalize(pud_fe(fx, 0)), expected);
}

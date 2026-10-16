#include <gtest/gtest.h>
#include <immer/map.hpp>
#include <immer/map_transient.hpp>
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/ubs_bind_map.hpp"
#include "pud_unifier_bind_map_stack.hpp"
#include "ubs_bind_map_test_maps.hpp"

#include "infrastructure/pud_transient_ubs_state.hpp"

namespace {
} // namespace

struct UbsBindMapIntegrationTest : public ::testing::Test {
    ubs_bind_map_expr_pool_backing backing;
    ubs_bind_map_expr_pool_backing::bind_map_t store{backing.make_bind_map()};
};

TEST_F(UbsBindMapIntegrationTest, ResolveUsesExprPoolHashConsForIsolatedVar) {
    const expr* expected = backing.pool.make_var(7u);
    EXPECT_EQ(store.resolve(7u), expected);
    EXPECT_EQ(store.resolve(7u), backing.pool.make_var(7u));
}

TEST_F(UbsBindMapIntegrationTest, EquateAndInstantiateShareCanonicalPointers) {
    const expr* bound = backing.pool.make_functor(11u, {});
    store.equate(0u, 1u);
    store.instantiate(0u, 11u, {});
    EXPECT_EQ(store.resolve(1u), bound);
    EXPECT_EQ(store.resolve(0u), backing.pool.make_functor(11u, {}));
}

TEST(UbsBindMapImmerIntegrationTest, EquateAndInstantiateOnTransientMapsWithPool) {
    expr_pool pool;
    pud_transient_ubs_state transient_ubs_state;
    auto store = transient_ubs_state.make_bind_map(pool, pool);

    const expr* bound = pool.make_functor(11u, {});
    store.equate(0u, 1u);
    store.instantiate(0u, 11u, {});
    EXPECT_EQ(store.resolve(1u), bound);
}

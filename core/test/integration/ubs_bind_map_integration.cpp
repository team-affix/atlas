#include <gtest/gtest.h>
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/pud_transient_ubs_state.hpp"
#include "value_objects/framed_expr.hpp"
#include "value_objects/pud_ubs_state.hpp"

TEST(UbsBindMapIntegrationTest, PersistedStateReadsBackThroughAFreshTransient) {
    expr_pool pool;
    const expr* bound = pool.make_functor(11u, {pool.make_var(2u)});
    pud_transient_ubs_state writer{pud_ubs_state{}};
    auto written = writer.make_bind_map();
    written.equate(5u, 3u);
    written.instantiate(5u, framed_expr{bound, 4u});
    const pud_ubs_state persisted = writer.to_persistent_state();

    pud_transient_ubs_state reader{persisted};
    auto read = reader.make_bind_map();
    EXPECT_EQ(read.find_rep(5u), 3u);
    EXPECT_EQ(read.find_value(3u), (framed_expr{bound, 4u}));
    EXPECT_FALSE(read.find_value(9u).has_value());
}

TEST(UbsBindMapIntegrationTest, WritesThroughATransientNeverChangeTheStateItCameFrom) {
    pud_transient_ubs_state writer{pud_ubs_state{}};
    writer.make_bind_map().equate(1u, 2u);
    const pud_ubs_state base = writer.to_persistent_state();

    pud_transient_ubs_state branch{base};
    branch.make_bind_map().equate(2u, 0u);
    branch.to_persistent_state();

    pud_transient_ubs_state reader{base};
    EXPECT_EQ(reader.make_bind_map().find_rep(2u), 1u);
}

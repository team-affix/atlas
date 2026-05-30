#include <gtest/gtest.h>
#include "infrastructure/bind_map.hpp"
#include "infrastructure/bind_map_factory.hpp"

struct BindMapFactoryIntegrationTest : public ::testing::Test {
    bind_map_factory bmf;

    expr var0{expr::var{0}};
    expr func{expr::functor{"f", {}}};
};

TEST_F(BindMapFactoryIntegrationTest, FactoryBindMapWhnfBindsAndResolves) {
    auto bm = bmf.make();
    bm->bind(0, &func);
    EXPECT_EQ(bm->whnf(&var0), &func);
}

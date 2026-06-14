#include <gtest/gtest.h>
#include "infrastructure/bind_map.hpp"
#include "infrastructure/bind_map_factory.hpp"
#include "atom_fixture.hpp"

struct BindMapFactoryIntegrationTest : public ::testing::Test {
    
    test_atoms atoms;bind_map_factory bmf;

    expr var0{expr::var{0}};
    expr func{expr::functor{atoms.id("f"), {}}};
};

TEST_F(BindMapFactoryIntegrationTest, FactoryBindMapWhnfBindsAndResolves) {
    auto bm = bmf.make();
    bm->bind(0, &func);
    EXPECT_EQ(bm->whnf(&var0), &func);
}

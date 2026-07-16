// dbuct_bind_map_factory: make() returns independent framed bind maps.

#include <gtest/gtest.h>
#include "infrastructure/dbuct_bind_map_factory.hpp"
#include "infrastructure/globalizer.hpp"
#include "functor_fixture.hpp"

struct DbuctBindMapFactoryTest : public ::testing::Test {
    test_functors functors;
    globalizer g;
    dbuct_bind_map_factory<globalizer> factory{g};
    expr var0{expr::var{0}};
    expr func{expr::functor{functors.id("f"), {}}};
};

TEST_F(DbuctBindMapFactoryTest, MakeReturnsBindMapThatResolvesBindings) {
    auto bm = factory.make();
    bm.bind(0, {&func, 0});
    EXPECT_EQ(bm.whnf({&var0, 0}).skeleton, &func);
}

TEST_F(DbuctBindMapFactoryTest, MakeReturnsIndependentInstances) {
    auto bm0 = factory.make();
    auto bm1 = factory.make();
    bm0.bind(0, {&func, 0});
    EXPECT_EQ(bm0.whnf({&var0, 0}).skeleton, &func);
    EXPECT_EQ(bm1.whnf({&var0, 0}).skeleton, &var0);
}

// bind_map_factory: produces a bind_map that supports bind/whnf. The factory is the
// only production type under test; returned maps must resolve variable bindings.

#include <gtest/gtest.h>
#include "infrastructure/bind_map_factory.hpp"
#include "infrastructure/globalizer.hpp"
#include "functor_fixture.hpp"

struct BindMapFactoryTest : public ::testing::Test {
    test_functors functors;
    globalizer g;
    bind_map_factory factory{g};
    expr var0{expr::var{0}};
    expr func{expr::functor{functors.id("f"), {}}};
};

TEST_F(BindMapFactoryTest, MakeReturnsBindMapThatResolvesBindings) {
    bind_map bm = factory.make();
    bm.bind(0, {&func, 0});
    EXPECT_EQ(bm.whnf({&var0, 0}).skeleton, &func);
}

TEST_F(BindMapFactoryTest, MakeReturnsIndependentInstances) {
    bind_map bm0 = factory.make();
    bind_map bm1 = factory.make();
    bm0.bind(0, {&func, 0});
    EXPECT_EQ(bm0.whnf({&var0, 0}).skeleton, &func);
    EXPECT_EQ(bm1.whnf({&var0, 0}).skeleton, &var0);
}

TEST_F(BindMapFactoryTest, FreshMapHasEmptyBindings) {
    bind_map bm = factory.make();
    EXPECT_EQ(bm.whnf({&var0, 0}).skeleton, &var0);
}

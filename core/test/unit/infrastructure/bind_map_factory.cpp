// bind_map_factory: produces an i_bind_map that supports bind/whnf. The factory is the
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
    std::unique_ptr<i_bind_map> bm = factory.make();
    ASSERT_NE(bm, nullptr);
    bm->bind(0, {&func, 0});
    EXPECT_EQ(bm->whnf({&var0, 0}).skeleton, &func);
}

TEST_F(BindMapFactoryTest, MakeReturnsIndependentInstances) {
    std::unique_ptr<i_bind_map> bm0 = factory.make();
    std::unique_ptr<i_bind_map> bm1 = factory.make();
    ASSERT_NE(bm0, nullptr);
    ASSERT_NE(bm1, nullptr);
    bm0->bind(0, {&func, 0});
    EXPECT_EQ(bm0->whnf({&var0, 0}).skeleton, &func);
    EXPECT_EQ(bm1->whnf({&var0, 0}).skeleton, &var0);
}

TEST_F(BindMapFactoryTest, FreshMapHasEmptyBindings) {
    std::unique_ptr<i_bind_map> bm = factory.make();
    ASSERT_NE(bm, nullptr);
    EXPECT_EQ(bm->whnf({&var0, 0}).skeleton, &var0);
}

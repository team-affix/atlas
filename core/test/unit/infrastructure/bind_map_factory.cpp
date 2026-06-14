// bind_map_factory: produces an i_bind_map that supports bind/whnf. The factory is the
// only production type under test; returned maps must resolve variable bindings.

#include <gtest/gtest.h>
#include "infrastructure/bind_map_factory.hpp"
#include "atom_fixture.hpp"

struct BindMapFactoryTest : public ::testing::Test {
    
    test_atoms atoms;bind_map_factory factory;
    expr var0{expr::var{0}};
    expr func{expr::functor{atoms.id("f"), {}}};
};

TEST_F(BindMapFactoryTest, MakeReturnsBindMapThatResolvesBindings) {
    std::unique_ptr<i_bind_map> bm = factory.make();
    ASSERT_NE(bm, nullptr);
    bm->bind(0, &func);
    EXPECT_EQ(bm->whnf(&var0), &func);
}

TEST_F(BindMapFactoryTest, MakeReturnsIndependentInstances) {
    std::unique_ptr<i_bind_map> bm0 = factory.make();
    std::unique_ptr<i_bind_map> bm1 = factory.make();
    ASSERT_NE(bm0, nullptr);
    ASSERT_NE(bm1, nullptr);
    bm0->bind(0, &func);
    EXPECT_EQ(bm0->whnf(&var0), &func);
    EXPECT_EQ(bm1->whnf(&var0), &var0);
}

TEST_F(BindMapFactoryTest, FreshMapHasEmptyBindings) {
    std::unique_ptr<i_bind_map> bm = factory.make();
    ASSERT_NE(bm, nullptr);
    EXPECT_EQ(bm->whnf(&var0), &var0);
}

#include <gtest/gtest.h>
#include "infrastructure/bind_map.hpp"
#include "infrastructure/bind_map_factory.hpp"
#include "infrastructure/globalizer.hpp"
#include "functor_fixture.hpp"

struct BindMapFactoryIntegrationTest : public ::testing::Test {
    test_functors functors;
    globalizer g;
    bind_map_factory bmf{g};

    expr var0{expr::var{0}};
    expr func{expr::functor{functors.id("f"), {}}};
};

TEST_F(BindMapFactoryIntegrationTest, FactoryBindMapWhnfBindsAndResolves) {
    auto bm = bmf.make();
    bm.bind(0, {&func, 0});
    EXPECT_EQ(bm.whnf({&var0, 0}).skeleton, &func);
}

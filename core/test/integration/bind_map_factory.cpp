#include <gtest/gtest.h>
#include "../../../core/hpp/infrastructure/bind_map.hpp"
#include "../../../core/hpp/infrastructure/bind_map_factory.hpp"
#include "../../../core/hpp/infrastructure/overlay_bind_map_factory.hpp"

struct BindMapFactoryIntegrationTest : public ::testing::Test {
    bind_map_factory bmf;
    overlay_bind_map_factory obmf;

    expr var0{expr::var{0}};
    expr func{expr::functor{"f", {}}};
};

TEST_F(BindMapFactoryIntegrationTest, FactoryBindMapWhnfBindsAndResolves) {
    auto bm = bmf.make();
    bm->bind(0, &func);
    EXPECT_EQ(bm->whnf(&var0), &func);
}

TEST_F(BindMapFactoryIntegrationTest, OverlayFactoryLayersLocalOverRemote) {
    bind_map common;
    auto local = bmf.make();
    auto overlay = obmf.make(*local, common);

    overlay->bind(0, &func);
    EXPECT_EQ(overlay->whnf(&var0), &func);
    EXPECT_EQ(common.whnf(&var0), &var0);
}

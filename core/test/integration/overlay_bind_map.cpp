#include <gtest/gtest.h>
#include "../../../core/hpp/infrastructure/overlay_bind_map.hpp"
#include "../../../core/hpp/infrastructure/bind_map.hpp"

struct OverlayBindMapIntegrationTest : public ::testing::Test {
protected:
    bind_map local;
    bind_map remote;
    overlay_bind_map obm{local, remote};
    i_bind_map& sut{obm};

    expr var0{expr::var{0}};
    expr func{expr::functor{"f", {}}};
    expr func2{expr::functor{"g", {}}};
    expr func_of_var0{expr::functor{"f", {&var0}}};
};

TEST_F(OverlayBindMapIntegrationTest, BindWritesOnlyToLocalLayer) {
    sut.bind(0, &func);

    EXPECT_EQ(local.whnf(&var0), &func);
    EXPECT_EQ(remote.whnf(&var0), &var0);
}

TEST_F(OverlayBindMapIntegrationTest, WhnfUsesLocalBindingWhenPresent) {
    local.bind(0, &func);

    EXPECT_EQ(sut.whnf(&var0), &func);
    EXPECT_EQ(remote.whnf(&var0), &var0);
}

TEST_F(OverlayBindMapIntegrationTest, WhnfFallsThroughToRemoteWhenLocalUnbound) {
    remote.bind(0, &func);

    EXPECT_EQ(sut.whnf(&var0), &func);
}

TEST_F(OverlayBindMapIntegrationTest, WhnfLocalShadowsRemote) {
    local.bind(0, &func);
    remote.bind(0, &func2);

    EXPECT_EQ(sut.whnf(&var0), &func);
}

TEST_F(OverlayBindMapIntegrationTest, WhnfDoesNotDescendIntoFunctorArguments) {
    remote.bind(0, &func);

    EXPECT_EQ(sut.whnf(&func_of_var0), &func_of_var0);
}

TEST_F(OverlayBindMapIntegrationTest, TwoOverlaysShareRemoteButHaveIndependentLocals) {
    bind_map local2;
    overlay_bind_map obm2{local2, remote};

    obm.bind(0, &func);
    obm2.bind(0, &func2);

    EXPECT_EQ(local.whnf(&var0), &func);
    EXPECT_EQ(local2.whnf(&var0), &func2);
    EXPECT_EQ(remote.whnf(&var0), &var0);
}

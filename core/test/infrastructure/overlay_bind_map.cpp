#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "../../../core/hpp/infrastructure/overlay_bind_map.hpp"

using ::testing::Return;

class MockBindMap : public i_bind_map {
public:
    MOCK_METHOD(void,        bind, (uint32_t, const expr*), (override));
    MOCK_METHOD(const expr*, whnf, (const expr*),           (override));
};

class OverlayBindMapTest : public ::testing::Test {
protected:
    MockBindMap  local;
    MockBindMap  remote;
    expr var0{expr::var{0}};
    expr var1{expr::var{1}};
    expr func{expr::functor{"f", {}}};
    expr func2{expr::functor{"g", {}}};
    expr func_of_var0{expr::functor{"f", {&var0}}};
    expr func_of_func{expr::functor{"f", {&func2}}};
};

TEST_F(OverlayBindMapTest, BindIsForwardedToLocalOnlyRemoteReceivesNoBindCall) {
    overlay_bind_map obm(local, remote);

    EXPECT_CALL(local,  bind(0u, &func));
    EXPECT_CALL(remote, bind).Times(0);

    obm.bind(0, &func);
}

TEST_F(OverlayBindMapTest, WhnfLocalResolvesRemoteNeverCalled) {
    overlay_bind_map obm(local, remote);

    EXPECT_CALL(local,  whnf(&var0)).WillRepeatedly(Return(&func));
    EXPECT_CALL(remote, whnf).Times(0);

    EXPECT_EQ(obm.whnf(&var0), &func);
}

TEST_F(OverlayBindMapTest, WhnfFallsThroughToRemoteWhenLocalReturnsSelf) {
    overlay_bind_map obm(local, remote);

    EXPECT_CALL(local,  whnf(&var0)).WillRepeatedly(Return(&var0));
    EXPECT_CALL(remote, whnf(&var0)).WillRepeatedly(Return(&func));

    EXPECT_EQ(obm.whnf(&var0), &func);
}

TEST_F(OverlayBindMapTest, WhnfLocalShadowsRemoteRemoteNeverCalled) {
    overlay_bind_map obm(local, remote);

    EXPECT_CALL(local,  whnf(&var0)).WillRepeatedly(Return(&func));
    EXPECT_CALL(remote, whnf).Times(0);

    EXPECT_EQ(obm.whnf(&var0), &func);
}

TEST_F(OverlayBindMapTest, WhnfFunctorNotBoundLocallyFallsThroughToRemote) {
    overlay_bind_map obm(local, remote);

    EXPECT_CALL(local,  whnf(&func)).WillRepeatedly(Return(&func));
    EXPECT_CALL(remote, whnf(&func)).WillRepeatedly(Return(&func));

    EXPECT_EQ(obm.whnf(&func), &func);
}

TEST_F(OverlayBindMapTest, WhnfFunctorWithVarArgIsNotRecursed) {
    overlay_bind_map obm(local, remote);

    EXPECT_CALL(local,  whnf(&func_of_var0)).WillRepeatedly(Return(&func_of_var0));
    EXPECT_CALL(remote, whnf(&func_of_var0)).WillRepeatedly(Return(&func_of_var0));

    EXPECT_EQ(obm.whnf(&func_of_var0), &func_of_var0);
}

TEST_F(OverlayBindMapTest, WhnfVar0LocallyBoundToCompositeFunctorRemoteNotCalled) {
    overlay_bind_map obm(local, remote);

    EXPECT_CALL(local,  whnf(&var0)).WillRepeatedly(Return(&func_of_var0));
    EXPECT_CALL(remote, whnf).Times(0);

    EXPECT_EQ(obm.whnf(&var0), &func_of_var0);
}

TEST_F(OverlayBindMapTest, WhnfVar0RemoteResolvesToCompositeFunctor) {
    overlay_bind_map obm(local, remote);

    EXPECT_CALL(local,  whnf(&var0)).WillRepeatedly(Return(&var0));
    EXPECT_CALL(remote, whnf(&var0)).WillRepeatedly(Return(&func_of_func));

    EXPECT_EQ(obm.whnf(&var0), &func_of_func);
}

TEST_F(OverlayBindMapTest, TwoOverlaysHaveIndependentLocalsBindGoesToOwnLocal) {
    MockBindMap local2;
    overlay_bind_map obm(local,  remote);
    overlay_bind_map obm2(local2, remote);

    EXPECT_CALL(local,  bind(0u, &func));
    EXPECT_CALL(local2, bind(0u, &func2));
    EXPECT_CALL(remote, bind).Times(0);

    obm.bind(0, &func);
    obm2.bind(0, &func2);
}

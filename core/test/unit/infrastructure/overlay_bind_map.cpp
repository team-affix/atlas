// overlay_bind_map layers a local bind map over a remote one: binds stay local, WHNF
// tries local first then remote. Unit tests mock both i_bind_map sides and assert
// delegation and independence between overlay instances.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/overlay_bind_map.hpp"
#include "interfaces/i_bind_map.hpp"

using ::testing::Return;

struct MockBindMap : public i_bind_map {
    MOCK_METHOD(void,        bind, (uint32_t, const expr*), (override));
    MOCK_METHOD(const expr*, whnf, (const expr*),           (override));
};

struct OverlayBindMapTest : public ::testing::Test {
protected:
    MockBindMap  local;
    MockBindMap  remote;
    overlay_bind_map obm{local, remote};
    i_bind_map& sut{obm};

    expr var0{expr::var{0}};
    expr var1{expr::var{1}};
    expr func{expr::functor{"f", {}}};
    expr func2{expr::functor{"g", {}}};
    expr func_of_var0{expr::functor{"f", {&var0}}};
    expr func_of_func{expr::functor{"f", {&func2}}};
    expr ternary{expr::functor{"h", {&var0, &var1, &func}}};
    expr ternary_of_var0{expr::functor{"h", {&var0, &var1, &var0}}};
};

TEST_F(OverlayBindMapTest, BindIsForwardedToLocalOnlyRemoteReceivesNoBindCall) {
    EXPECT_CALL(local,  bind(0u, &func));
    EXPECT_CALL(remote, bind).Times(0);

    sut.bind(0, &func);
}

TEST_F(OverlayBindMapTest, WhnfLocalResolvesRemoteNeverCalled) {
    EXPECT_CALL(local,  whnf(&var0)).WillRepeatedly(Return(&func));
    EXPECT_CALL(remote, whnf).Times(0);

    EXPECT_EQ(sut.whnf(&var0), &func);
}

TEST_F(OverlayBindMapTest, WhnfFallsThroughToRemoteWhenLocalReturnsSelf) {
    EXPECT_CALL(local,  whnf(&var0)).WillRepeatedly(Return(&var0));
    EXPECT_CALL(remote, whnf(&var0)).WillRepeatedly(Return(&func));

    EXPECT_EQ(sut.whnf(&var0), &func);
}

TEST_F(OverlayBindMapTest, WhnfLocalShadowsRemoteRemoteNeverCalled) {
    EXPECT_CALL(local,  whnf(&var0)).WillRepeatedly(Return(&func));
    EXPECT_CALL(remote, whnf).Times(0);

    EXPECT_EQ(sut.whnf(&var0), &func);
}

TEST_F(OverlayBindMapTest, WhnfFunctorNotBoundLocallyFallsThroughToRemote) {
    EXPECT_CALL(local,  whnf(&func)).WillRepeatedly(Return(&func));
    EXPECT_CALL(remote, whnf(&func)).WillRepeatedly(Return(&func));

    EXPECT_EQ(sut.whnf(&func), &func);
}

TEST_F(OverlayBindMapTest, WhnfFunctorWithVarArgIsNotRecursed) {
    EXPECT_CALL(local,  whnf(&func_of_var0)).WillRepeatedly(Return(&func_of_var0));
    EXPECT_CALL(remote, whnf(&func_of_var0)).WillRepeatedly(Return(&func_of_var0));

    EXPECT_EQ(sut.whnf(&func_of_var0), &func_of_var0);
}

TEST_F(OverlayBindMapTest, WhnfVar0LocallyBoundToCompositeFunctorRemoteNotCalled) {
    EXPECT_CALL(local,  whnf(&var0)).WillRepeatedly(Return(&func_of_var0));
    EXPECT_CALL(remote, whnf).Times(0);

    EXPECT_EQ(sut.whnf(&var0), &func_of_var0);
}

TEST_F(OverlayBindMapTest, WhnfVar0RemoteResolvesToCompositeFunctor) {
    EXPECT_CALL(local,  whnf(&var0)).WillRepeatedly(Return(&var0));
    EXPECT_CALL(remote, whnf(&var0)).WillRepeatedly(Return(&func_of_func));

    EXPECT_EQ(sut.whnf(&var0), &func_of_func);
}

TEST_F(OverlayBindMapTest, WhnfTernaryFunctorDelegatesToRemote) {
    EXPECT_CALL(local, whnf(&ternary)).WillRepeatedly(Return(&ternary));
    EXPECT_CALL(remote, whnf(&ternary)).WillRepeatedly(Return(&ternary));

    EXPECT_EQ(sut.whnf(&ternary), &ternary);
}

TEST_F(OverlayBindMapTest, WhnfVarBoundToTernaryFunctorLocally) {
    EXPECT_CALL(local, whnf(&var0)).WillRepeatedly(Return(&ternary_of_var0));
    EXPECT_CALL(remote, whnf).Times(0);

    EXPECT_EQ(sut.whnf(&var0), &ternary_of_var0);
}

TEST_F(OverlayBindMapTest, TwoOverlaysHaveIndependentLocalsBindGoesToOwnLocal) {
    MockBindMap local2;
    overlay_bind_map obm2{local2, remote};

    EXPECT_CALL(local,  bind(0u, &func));
    EXPECT_CALL(local2, bind(0u, &func2));
    EXPECT_CALL(remote, bind).Times(0);

    sut.bind(0, &func);
    obm2.bind(0, &func2);
}

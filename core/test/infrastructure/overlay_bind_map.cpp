#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "../../../core/hpp/infrastructure/overlay_bind_map.hpp"
#include "../../../core/hpp/bootstrap/bindings.hpp"
#include "../../../core/hpp/bootstrap/locator.hpp"
#include "../../../core/hpp/domain/interfaces/i_factory.hpp"

using ::testing::Return;

class MockBindMap : public i_bind_map {
public:
    MOCK_METHOD(void,        bind, (uint32_t, const expr*), (override));
    MOCK_METHOD(const expr*, whnf, (const expr*),           (override));
};

class MockFactory : public i_factory<i_bind_map> {
public:
    MOCK_METHOD(std::unique_ptr<i_bind_map>, make, (), (const, override));
};

class OverlayBindMapTest : public ::testing::Test {
protected:
    void SetUp() override {
        b.bind<i_factory<i_bind_map>>(factory);
        locator::register_bindings(&b);
    }
    void TearDown() override { locator::register_bindings(nullptr); }

    MockBindMap  remote;
    MockFactory  factory;
    bindings     b;
    expr var0{expr::var{0}};
    expr var1{expr::var{1}};
    expr func{expr::functor{"f", {}}};
    expr func2{expr::functor{"g", {}}};
    expr func_of_var0{expr::functor{"f", {&var0}}};
    expr func_of_func{expr::functor{"f", {&func2}}};
};

TEST_F(OverlayBindMapTest, BindIsForwardedToLocalOnlyRemoteReceivesNoBindCall) {
    auto local_uptr = std::make_unique<MockBindMap>();
    auto* local = local_uptr.get();
    EXPECT_CALL(factory, make()).WillOnce(Return(std::move(local_uptr)));
    overlay_bind_map obm(remote);

    EXPECT_CALL(*local, bind(0u, &func));
    EXPECT_CALL(remote, bind).Times(0);

    obm.bind(0, &func);
}

TEST_F(OverlayBindMapTest, WhnfLocalResolvesRemoteNeverCalled) {
    auto local_uptr = std::make_unique<MockBindMap>();
    auto* local = local_uptr.get();
    EXPECT_CALL(factory, make()).WillOnce(Return(std::move(local_uptr)));
    overlay_bind_map obm(remote);

    EXPECT_CALL(*local, whnf(&var0)).WillRepeatedly(Return(&func));
    EXPECT_CALL(remote, whnf).Times(0);

    EXPECT_EQ(obm.whnf(&var0), &func);
}

TEST_F(OverlayBindMapTest, WhnfFallsThroughToRemoteWhenLocalReturnsSelf) {
    auto local_uptr = std::make_unique<MockBindMap>();
    auto* local = local_uptr.get();
    EXPECT_CALL(factory, make()).WillOnce(Return(std::move(local_uptr)));
    overlay_bind_map obm(remote);

    EXPECT_CALL(*local, whnf(&var0)).WillRepeatedly(Return(&var0));
    EXPECT_CALL(remote, whnf(&var0)).WillRepeatedly(Return(&func));

    EXPECT_EQ(obm.whnf(&var0), &func);
}

TEST_F(OverlayBindMapTest, WhnfLocalShadowsRemoteRemoteNeverCalled) {
    auto local_uptr = std::make_unique<MockBindMap>();
    auto* local = local_uptr.get();
    EXPECT_CALL(factory, make()).WillOnce(Return(std::move(local_uptr)));
    overlay_bind_map obm(remote);

    EXPECT_CALL(*local, whnf(&var0)).WillRepeatedly(Return(&func));
    EXPECT_CALL(remote, whnf).Times(0);

    EXPECT_EQ(obm.whnf(&var0), &func);
}

TEST_F(OverlayBindMapTest, WhnfFunctorNotBoundLocallyFallsThroughToRemote) {
    auto local_uptr = std::make_unique<MockBindMap>();
    auto* local = local_uptr.get();
    EXPECT_CALL(factory, make()).WillOnce(Return(std::move(local_uptr)));
    overlay_bind_map obm(remote);

    EXPECT_CALL(*local, whnf(&func)).WillRepeatedly(Return(&func));
    EXPECT_CALL(remote, whnf(&func)).WillRepeatedly(Return(&func));

    EXPECT_EQ(obm.whnf(&func), &func);
}

TEST_F(OverlayBindMapTest, WhnfFunctorWithVarArgIsNotRecursed) {
    auto local_uptr = std::make_unique<MockBindMap>();
    auto* local = local_uptr.get();
    EXPECT_CALL(factory, make()).WillOnce(Return(std::move(local_uptr)));
    overlay_bind_map obm(remote);

    EXPECT_CALL(*local, whnf(&func_of_var0)).WillRepeatedly(Return(&func_of_var0));
    EXPECT_CALL(remote, whnf(&func_of_var0)).WillRepeatedly(Return(&func_of_var0));

    EXPECT_EQ(obm.whnf(&func_of_var0), &func_of_var0);
}

TEST_F(OverlayBindMapTest, WhnfVar0LocallyBoundToCompositeFunctorRemoteNotCalled) {
    auto local_uptr = std::make_unique<MockBindMap>();
    auto* local = local_uptr.get();
    EXPECT_CALL(factory, make()).WillOnce(Return(std::move(local_uptr)));
    overlay_bind_map obm(remote);

    EXPECT_CALL(*local, whnf(&var0)).WillRepeatedly(Return(&func_of_var0));
    EXPECT_CALL(remote, whnf).Times(0);

    EXPECT_EQ(obm.whnf(&var0), &func_of_var0);
}

TEST_F(OverlayBindMapTest, WhnfVar0RemoteResolvesToCompositeFunctor) {
    auto local_uptr = std::make_unique<MockBindMap>();
    auto* local = local_uptr.get();
    EXPECT_CALL(factory, make()).WillOnce(Return(std::move(local_uptr)));
    overlay_bind_map obm(remote);

    EXPECT_CALL(*local, whnf(&var0)).WillRepeatedly(Return(&var0));
    EXPECT_CALL(remote, whnf(&var0)).WillRepeatedly(Return(&func_of_func));

    EXPECT_EQ(obm.whnf(&var0), &func_of_func);
}

TEST_F(OverlayBindMapTest, TwoOverlaysHaveIndependentLocalsBindGoesToOwnLocal) {
    auto local1_uptr = std::make_unique<MockBindMap>();
    auto local2_uptr = std::make_unique<MockBindMap>();
    auto* local1 = local1_uptr.get();
    auto* local2 = local2_uptr.get();

    EXPECT_CALL(factory, make())
        .WillOnce(Return(std::move(local1_uptr)))
        .WillOnce(Return(std::move(local2_uptr)));

    overlay_bind_map obm(remote);
    overlay_bind_map obm2(remote);

    EXPECT_CALL(*local1, bind(0u, &func));
    EXPECT_CALL(*local2, bind(0u, &func2));
    EXPECT_CALL(remote, bind).Times(0);

    obm.bind(0, &func);
    obm2.bind(0, &func2);
}

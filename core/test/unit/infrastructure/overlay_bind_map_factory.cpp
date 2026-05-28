// overlay_bind_map_factory: builds overlay over local/remote bind maps. Local binds
// must be visible through the overlay; remote must not see local-only bindings.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "infrastructure/overlay_bind_map_factory.hpp"
#include "interfaces/i_bind_map.hpp"

using ::testing::Return;

struct MockBindMap : public i_bind_map {
    MOCK_METHOD(void, bind, (uint32_t, const expr*), (override));
    MOCK_METHOD(const expr*, whnf, (const expr*), (override));
};

struct OverlayBindMapFactoryTest : public ::testing::Test {
    overlay_bind_map_factory factory;
    MockBindMap local;
    MockBindMap remote;
    expr var0{expr::var{0}};
    expr func{expr::functor{"f", {}}};
};

TEST_F(OverlayBindMapFactoryTest, OverlayDelegatesWhnfToLocalThenRemote) {
    EXPECT_CALL(local, whnf(&var0)).WillOnce(Return(&func));
    auto overlay = factory.make(local, remote);
    ASSERT_NE(overlay, nullptr);
    EXPECT_EQ(overlay->whnf(&var0), &func);
}

TEST_F(OverlayBindMapFactoryTest, OverlayBindWritesLocal) {
    auto overlay = factory.make(local, remote);
    EXPECT_CALL(local, bind(0, &func)).Times(1);
    overlay->bind(0, &func);
}

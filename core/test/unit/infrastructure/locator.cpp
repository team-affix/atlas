// locator is the DI registry: bind_as registers concrete and interface type entries;
// locate returns the bound instance. Tests assert registration identity and DEBUG_ASSERT
// failures on duplicate bind or unregistered lookup (debug builds only).

#include <gtest/gtest.h>
#include "infrastructure/locator.hpp"

struct dummy_concrete {
    int value = 42;
};

struct dummy_iface_a {
    virtual ~dummy_iface_a() = default;
    virtual int a() const { return 1; }
};

struct dummy_iface_b {
    virtual ~dummy_iface_b() = default;
    virtual int b() const { return 2; }
};

struct dual_service : dummy_iface_a, dummy_iface_b {
    int a() const override { return 10; }
    int b() const override { return 20; }
};

struct LocatorTest : public ::testing::Test {
    locator loc;
};

TEST_F(LocatorTest, BindAsRegistersConcreteType) {
    dummy_concrete concrete;
    loc.bind_as(concrete);
    EXPECT_EQ(&loc.locate<dummy_concrete>(), &concrete);
    EXPECT_EQ(loc.locate<dummy_concrete>().value, 42);
}

TEST_F(LocatorTest, BindAsRegistersEachInterface) {
    dual_service service;
    loc.bind_as<dummy_iface_a, dummy_iface_b>(service);
    EXPECT_EQ(&loc.locate<dual_service>(), &service);
    EXPECT_EQ(&loc.locate<dummy_iface_a>(), static_cast<dummy_iface_a*>(&service));
    EXPECT_EQ(&loc.locate<dummy_iface_b>(), static_cast<dummy_iface_b*>(&service));
    EXPECT_EQ(loc.locate<dummy_iface_a>().a(), 10);
    EXPECT_EQ(loc.locate<dummy_iface_b>().b(), 20);
}

TEST_F(LocatorTest, DuplicateBindThrows) {
    dummy_concrete concrete;
    loc.bind_as(concrete);
    EXPECT_THROW(loc.bind_as(concrete), std::logic_error);
}

TEST_F(LocatorTest, LocateUnregisteredThrows) {
    EXPECT_THROW(loc.locate<dummy_concrete>(), std::logic_error);
}

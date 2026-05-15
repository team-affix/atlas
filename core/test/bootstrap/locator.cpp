#include <gtest/gtest.h>
#include "../../../core/hpp/bootstrap/locator.hpp"

class LocatorTest : public ::testing::Test {
protected:
    bindings b;
    void SetUp() override { locator::register_bindings(&b); }
    void TearDown() override { locator::register_bindings(nullptr); }
};

TEST_F(LocatorTest, LocateReturnsCorrectObjectAfterRegisterBindings) {
    int x = 7;
    b.bind(x);
    EXPECT_EQ(&locator::locate<int>(), &x);
    EXPECT_EQ(locator::locate<int>(), 7);
}

TEST_F(LocatorTest, MultipleTypesLocateThroughLocator) {
    int x = 1;
    double d = 2.5;
    b.bind(x);
    b.bind(d);
    EXPECT_EQ(&locator::locate<int>(), &x);
    EXPECT_EQ(&locator::locate<double>(), &d);
}

#include <gtest/gtest.h>
#include "../../../core/hpp/bootstrap/bindings.hpp"
#include <stdexcept>

class BindingsTest : public ::testing::Test {
protected:
    bindings b;
    int x = 42;
    void SetUp() override { b.bind(x); }
};

TEST_F(BindingsTest, BindAndResolveReturnsSameObjectByReference) {
    EXPECT_EQ(&b.resolve<int>(), &x);
    EXPECT_EQ(b.resolve<int>(), 42);
}

TEST_F(BindingsTest, MultipleDistinctTypesAllResolveCorrectly) {
    double d = 3.14;
    b.bind(d);
    EXPECT_EQ(&b.resolve<int>(), &x);
    EXPECT_EQ(&b.resolve<double>(), &d);
}

TEST_F(BindingsTest, ResolveWithUnboundTypeThrows) {
    EXPECT_THROW(b.resolve<double>(), std::out_of_range);
}

TEST_F(BindingsTest, BindDuplicateTypeThrows) {
    int y = 99;
    EXPECT_THROW(b.bind(y), std::logic_error);
}

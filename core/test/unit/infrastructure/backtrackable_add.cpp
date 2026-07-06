// backtrackable_add adds a fixed amount to a scalar with undo subtraction. Unit
// tests assert the forward add and its exact reversal.

#include <gtest/gtest.h>
#include <cstdint>
#include "infrastructure/backtrackable_add.hpp"

struct BacktrackableAddTest : public ::testing::Test {
protected:
    uint32_t x = 10;
    backtrackable_add<uint32_t> m{7};
    void SetUp() override { m.capture(x); }
};

TEST_F(BacktrackableAddTest, InvokeAddsAmount) {
    m.invoke();
    EXPECT_EQ(x, 17u);
}

TEST_F(BacktrackableAddTest, InvokeAndBacktrackRestoresValue) {
    m.invoke();
    m.backtrack();
    EXPECT_EQ(x, 10u);
}
